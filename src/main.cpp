#include <HardwareSerial.h>
#include <ESPmDNS.h>
#include <EEPROM.h>
#include <WiFi.h>
#include <ArduinoModbus.h>   // Biblioteca para Modbus TCP

#include "OTA.h"
#include "WSerial_c.h"
#include "display_c.h"
#include "wifimanager_c.h"

/********** POTENTIOMETERS GPIO define *****/
#define def_pin_POT1      36 // GPIO36
#define def_pin_POT2      39 // GPIO39

/***************** Read 4@20 mA ***********/
#define def_pin_R4a20_1   35 // GPIO35
#define def_pin_R4a20_2   34 // GPIO34

/********************* ADC ****************/
#define def_pin_ADC1      32 // GPIO32

/******************** Digitais **************/
#define def_pin_D1        13 // GPIO13 - somente digital
#define def_pin_D2        14 // GPIO14 - somente digital
#define def_pin_D3        27 // GPIO27 - somente digital
#define def_pin_D4        33 // GPIO33 - somente digital

/********************* DAC ****************/
#define def_pin_DAC1      25 // GPIO25

/***************** Write 4@20 mA **********/
#define def_pin_W4a20_1   26 // GPIO26

/********************* RELÊ ***************/
#define def_pin_RELE      23 // GPIO23

/***************** OLED Display ************/
#define def_pin_SDA       21 // GPIO21
#define def_pin_SCL       5  // GPIO5

/********************* PWM ****************/
#define def_pin_PWM       12 // GPIO12

/************* BUTTONS GPIO define *********/
#define def_pin_RTN1      15 // GPIO15 Botão Retentivo 1
#define def_pin_RTN2      2  // GPIO2  Botão Retentivo 2
#define def_pin_PUSH1     16 // GPIO16 Botão Push Button 1
#define def_pin_PUSH2     17 // GPIO17 Botão Push Button 2

// Variáveis globais do projeto:
char DDNSName[15] = "inindkit";
WifiManager_c wm;
Display_c disp;
WSerial_c WSerial;

// Declaração da instância do servidor Modbus TCP:
ModbusTCPServer modbusTCPServer;

void errorMsg(String error, bool restart) {
    WSerial.println(error);
    if (restart) {
        WSerial.println("Rebooting now...");
        delay(2000);
        ESP.restart();
        delay(2000);
    }
}

void setup() {
    WSerial.println("Booting");
    // Inicializa o Display:
    if (startDisplay(&disp, def_pin_SDA, def_pin_SCL)) {
        disp.setText(1, "Inicializando...");
        WSerial.println("Display running");
    } else {
        errorMsg("Display error.", false);
    }
    delay(50);
    
    // EEPROM para ler a identificação do kit:
    EEPROM.begin(1);
    char idKit[2] = "0";
    idKit[0] = (char)EEPROM.read(0); // id do kit
    strcat(DDNSName, idKit);

    // Inicia conexão WiFi:
    WiFi.mode(WIFI_STA);
    wm.start(&WSerial);
    wm.setApName(DDNSName);
    disp.setFuncMode(true);
    disp.setText(1, "Mode: Acces Point", true);
    disp.setText(2, "SSID: AutoConnectAP", true);
    disp.setText(3, "PSWD: ", true);
    if (wm.autoConnect("AutoConnectAP")) {
        WSerial.print("\nWifi running - IP:");
        WSerial.println(WiFi.localIP());
        disp.setFuncMode(false);
        disp.setText(1, (WiFi.localIP().toString() + " ID:" + String(idKit[0])).c_str());
        disp.setText(2, DDNSName);
        disp.setText(3, "UFU Mode");
        delay(50);
    } else {
        errorMsg("Wifi error.\nAP MODE...", false);
    }

    // Inicia OTA e Telnet:
    OTA::start(DDNSName);  // Deve ocorrer após a conexão WiFi
    startWSerial(&WSerial, 4000 + String(idKit[0]).toInt());

    // Configuração dos pinos:
    // Entradas Analógicas:
    pinMode(def_pin_POT1, ANALOG);
    pinMode(def_pin_POT2, ANALOG);
    pinMode(def_pin_R4a20_1, ANALOG);
    pinMode(def_pin_R4a20_2, ANALOG);
    pinMode(def_pin_ADC1, ANALOG);
    
    // Saídas Analógicas:
    pinMode(def_pin_DAC1, ANALOG);       // DAC para saída analógica
    // Para W4a20_1 e PWM usamos funções de saída analógica (analogWrite ou dacWrite conforme o caso)
    
    // Saídas Digitais:
    pinMode(def_pin_D1, OUTPUT);
    pinMode(def_pin_D2, OUTPUT);
    pinMode(def_pin_D3, OUTPUT);
    pinMode(def_pin_D4, OUTPUT);
    pinMode(def_pin_RELE, OUTPUT);
    
    // Saídas para 4-20 mA escrita:
    pinMode(def_pin_W4a20_1, OUTPUT);
    
    // PWM:
    pinMode(def_pin_PWM, OUTPUT);
    
    // Botões (Entradas Digitais):
    pinMode(def_pin_RTN1, INPUT_PULLDOWN);
    pinMode(def_pin_RTN2, INPUT_PULLDOWN);
    pinMode(def_pin_PUSH1, INPUT_PULLDOWN);
    pinMode(def_pin_PUSH2, INPUT_PULLDOWN);
    
    // Outros setups de hardware:
    digitalWrite(def_pin_D1, LOW);
    digitalWrite(def_pin_D2, LOW);
    digitalWrite(def_pin_D3, LOW);
    digitalWrite(def_pin_D4, LOW);
    digitalWrite(def_pin_RELE, LOW);
    analogWrite(def_pin_PWM, 0);
    // Inicializa o DAC e a saída de 4–20 mA com zero
    dacWrite(def_pin_DAC1, 0);
    analogWrite(def_pin_W4a20_1, 0);
    
    /************ INTEGRAÇÃO COM MODBUS TCP ************/
    // Inicia o servidor Modbus TCP na porta 502:
    if (!modbusTCPServer.begin(502)) {
        WSerial.println("Falha ao iniciar o servidor Modbus TCP!");
        while (1);  // Trava caso não inicie
    }
    WSerial.println("Servidor Modbus TCP iniciado com sucesso!");
    
    // Configura os canais Modbus:
    // Holding Registers: 3 registros para controlar as saídas analógicas:
    modbusTCPServer.configureHoldingRegisters(0, 3); // [0]: DAC, [1]: Write 4-20mA, [2]: PWM
    // Input Registers: 5 registros para as entradas analógicas:
    modbusTCPServer.configureInputRegisters(0, 5);     // [0]: POT1, [1]: POT2, [2]: R4a20_1, [3]: R4a20_2, [4]: ADC1
    // Coils: 5 coils para as saídas digitais:
    modbusTCPServer.configureCoils(0, 5);              // [0]: D1, [1]: D2, [2]: D3, [3]: D4, [4]: RELÊ
    // Discrete Inputs: 4 entradas digitais para os botões:
    modbusTCPServer.configureDiscreteInputs(0, 4);     // [0]: RTN1, [1]: RTN2, [2]: PUSH1, [3]: PUSH2
}

void loop() {
    OTA::handle();
    updateWSerial(&WSerial);
    updateDisplay(&disp);
    if (wm.getPortalRunning()) wm.process();
    
    /************ PROCESSO DO MODBUS TCP ************/
    // Processa requisições do mestre Modbus TCP:
    modbusTCPServer.poll();
    
    // --- Atualiza os canais baseados nos I/Os ---
    // 1. Saídas Analógicas (Holding Registers):
    // Registro 0: DAC
    uint16_t dacVal = modbusTCPServer.holdingRegisterRead(0);
    dacWrite(def_pin_DAC1, dacVal);
    
    // Registro 1: Write 4–20mA (usando analogWrite – ajuste a escala conforme necessário)
    uint16_t w4a20Val = modbusTCPServer.holdingRegisterRead(1);
    analogWrite(def_pin_W4a20_1, w4a20Val);
    
    // Registro 2: PWM
    uint16_t pwmVal = modbusTCPServer.holdingRegisterRead(2);
    analogWrite(def_pin_PWM, pwmVal);
    
    // 2. Saídas Digitais (Coils):
    // Coil 0: D1
    digitalWrite(def_pin_D1, modbusTCPServer.coilRead(0) ? HIGH : LOW);
    // Coil 1: D2
    digitalWrite(def_pin_D2, modbusTCPServer.coilRead(1) ? HIGH : LOW);
    // Coil 2: D3
    digitalWrite(def_pin_D3, modbusTCPServer.coilRead(2) ? HIGH : LOW);
    // Coil 3: D4
    digitalWrite(def_pin_D4, modbusTCPServer.coilRead(3) ? HIGH : LOW);
    // Coil 4: RELÊ
    digitalWrite(def_pin_RELE, modbusTCPServer.coilRead(4) ? HIGH : LOW);
    
    // 3. Entradas Analógicas (Input Registers):
    modbusTCPServer.inputRegisterWrite(0, analogRead(def_pin_POT1));
    modbusTCPServer.inputRegisterWrite(1, analogRead(def_pin_POT2));
    modbusTCPServer.inputRegisterWrite(2, analogRead(def_pin_R4a20_1));
    modbusTCPServer.inputRegisterWrite(3, analogRead(def_pin_R4a20_2));
    modbusTCPServer.inputRegisterWrite(4, analogRead(def_pin_ADC1));
    
    // 4. Entradas Digitais (Discrete Inputs):
    modbusTCPServer.discreteInputWrite(0, digitalRead(def_pin_RTN1));
    modbusTCPServer.discreteInputWrite(1, digitalRead(def_pin_RTN2));
    modbusTCPServer.discreteInputWrite(2, digitalRead(def_pin_PUSH1));
    modbusTCPServer.discreteInputWrite(3, digitalRead(def_pin_PUSH2));
}