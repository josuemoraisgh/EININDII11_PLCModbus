// coil status:       [0] def_pin_D1,   [1] def_pin_D2,   [2] def_pin_D3,             [3] def_pin_D4,             [4] def_pin_RELE, [5]
// Input status:      [0] def_pin_RTN1, [1] def_pin_RTN2, [2] def_pin_PUSH1,          [3] def_pin_PUSH2,          [4]               [5] 
// Input Registers:   [0] POT1,         [1] POT2,         [2] Leitura 4-20mA canal 1, [3] Leitura 4-20mA canal 2, [4] ADC1,         [5] ADC2,
// Holding Registers: [0] DAC,          [1] Write 4-20mA, [2] PWM                     [3]                         [4]               [5]  

#include <Arduino.h>
#include <WiFi.h>
#include <ModbusServerWiFi.h>
#include <ESPmDNS.h>
#include <EEPROM.h>
#include "OTA.h"
#include "WSerial_c.h"
#include "display_c.h"
#include "wifimanager_c.h"
#include "ads1115_c.h"

/********** Definições dos GPIO ***********/
#define def_pin_ADC1      36    
#define def_pin_ADC2      39    
#define def_pin_RTN2      35    
#define def_pin_PUSH1     34    
#define def_pin_PWM       33    
#define def_pin_PUSH2     32    
#define def_pin_RELE      27    
#define def_pin_W4a20_1   26    
#define def_pin_DAC1      25    
#define def_pin_D1        23    
#define def_pin_SCL       22    
#define def_pin_SDA       21    
#define def_pin_D2        19    
#define def_pin_D3        18    
#define def_pin_D4         4    
#define def_pin_RTN1       2    

// Variáveis e objetos do projeto:
char DDNSName[15] = "inindkit";
ADS1115_c ads; 
WifiManager_c wm;
Display_c disp;
WSerial_c WSerial;

// Instância do servidor Modbus (eModbus)
ModbusServerWiFi modbusServer;


#define COILSIZE 5
#define STATUSSIZE 4
#define HRSIZE 3
#define IRSIZE 6


volatile uint16_t holdingRegisters[HRSIZE] = { 0, 0, 0 }; //[0]: DAC, [1]: Wriete 4-20mA, [2]: PWM
volatile bool coils[COILSIZE] = { false, false, false, false, false }; //[0]: D1, [1]: D2, [2]: D3, [3]: D4, [4]: RELÊ


// ========================================================
// CALLBACKS MODBUS – agora realizando leituras de sensores/entradas diretamente
// ========================================================

// Leitura das Coils (FC 01)
// Continua usando o vetor, pois são alteradas pelos comandos de escrita.
ModbusMessage readCoils(ModbusMessage request) {
  ModbusMessage response;
  uint16_t addr = 0, count = 0;
  request.get(2, addr);
  request.get(4, count);

  if ((addr + count) > COILSIZE) {
    response.setError(request.getServerID(), request.getFunctionCode(), ILLEGAL_DATA_ADDRESS);
  } else {
    uint8_t byteCount = (count + 7) / 8;
    response.add(request.getServerID(), request.getFunctionCode(), byteCount);
    for (uint8_t byteIndex = 0; byteIndex < byteCount; byteIndex++) {
      uint8_t coilByte = 0;
      for (uint8_t bit = 0; bit < 8; bit++) {
        uint16_t coilIndex = addr + byteIndex * 8 + bit;
        if (coilIndex < (addr + count) && coils[coilIndex]) {
          coilByte |= (1 << bit);
        }
      }
      response.add(coilByte);
    }
  }
  return response;
}

// Leitura das Discrete Inputs (FC 02)
// Realiza a leitura direta dos pinos, dispensando vetor intermediário:
ModbusMessage readDiscreteInputs(ModbusMessage request) {
  ModbusMessage response;
  uint16_t addr = 0, count = 0;
  request.get(2, addr);
  request.get(4, count);

  if ((addr + count) > STATUSSIZE) {  // Apenas 4 entradas digitais disponíveis
    response.setError(request.getServerID(), request.getFunctionCode(), ILLEGAL_DATA_ADDRESS);
  } else {
    uint8_t byteCount = (count + 7) / 8;
    response.add(request.getServerID(), request.getFunctionCode(), byteCount);
    for (uint8_t byteIndex = 0; byteIndex < byteCount; byteIndex++) {
      uint8_t inputByte = 0;
      for (uint8_t bit = 0; bit < 8; bit++) {
        uint16_t inputIndex = addr + byteIndex * 8 + bit;
        if (inputIndex < (addr + count)) {
          bool pinState = false;
          switch (inputIndex) {
            case 0: pinState = digitalRead(def_pin_RTN1); break;
            case 1: pinState = digitalRead(def_pin_RTN2); break;
            case 2: pinState = digitalRead(def_pin_PUSH1); break;
            case 3: pinState = digitalRead(def_pin_PUSH2); break;
            default: break;
          }
          if (pinState) inputByte |= (1 << bit);
        }
      }
      response.add(inputByte);
    }
  }
  return response;
}

// Leitura dos Holding Registers (FC 03)
// (Permanece usando o vetor holdingRegisters, pois normalmente os comandos de escrita
// alteram esses valores e o mestre pode lê‑los.)
ModbusMessage readHoldingRegisters(ModbusMessage request) {
  ModbusMessage response;
  uint16_t addr = 0, words = 0;
  request.get(2, addr);
  request.get(4, words);

  if (words > HRSIZE) {  // Apenas HRSIZE registradores disponíveis
    response.setError(request.getServerID(), request.getFunctionCode(), ILLEGAL_DATA_ADDRESS);
  } else {
    response.add(request.getServerID(), request.getFunctionCode(), (uint8_t)(words * 2));
    for (uint16_t i = 0; i < words; i++) {
      response.add( holdingRegisters[addr + i] );
    }
  }
  return response;
}

// Leitura dos Input Registers (FC 04)
// Em vez de usar um vetor intermediário, realiza a leitura diretamente dos sensores:
ModbusMessage readInputRegisters(ModbusMessage request) {
  ModbusMessage response;
  uint16_t addr = 0, words = 0;
  request.get(2, addr);
  request.get(4, words);

  if ((addr + words) > IRSIZE) {  // Temos 5 entradas analógicas
    response.setError(request.getServerID(), request.getFunctionCode(), ILLEGAL_DATA_ADDRESS);
  } else {
    response.add(request.getServerID(), request.getFunctionCode(), (uint8_t)(words * 2));
    for (uint16_t i = 0; i < words; i++) {
      switch (addr + i) {
        case 0:
          response.add(ads.analogRead(1));  // POT1
          break;
        case 1:
          response.add(ads.analogRead(0));  // POT2
          break;
        case 2:
          response.add(ads.analogRead(3));  // Leitura 4-20mA canal 1
          break;
        case 3:
          response.add(ads.analogRead(2));  // Leitura 4-20mA canal 2
          break;
        case 4:
          response.add(analogRead(def_pin_ADC1)); // ADC1
          break;
        case 5:
          response.add(analogRead(def_pin_ADC2)); // ADC1
          break;          
        default:
          break;
      }
    }
  }
  return response;
}

// Escrita de uma Coil (FC 05)
// Atualiza o vetor e imediatamente configura a saída digital correspondente
ModbusMessage writeSingleCoil(ModbusMessage request) {
  ModbusMessage response;
  uint16_t addr = 0, value = 0;
  request.get(2, addr);
  request.get(4, value);
  
  // Verifica se o endereço solicitado está dentro do range
  if (addr >= COILSIZE) {
    response.setError(request.getServerID(), request.getFunctionCode(), ILLEGAL_DATA_ADDRESS);
    return response;
  }
  
  // Valida o valor recebido; apenas 0xFF00 (true) ou 0x0000 (false) são permitidos
  if ((value != 0xFF00) && (value != 0x0000)) {
    response.setError(request.getServerID(), request.getFunctionCode(), ILLEGAL_DATA_VALUE);
    return response;
  }
  
  // Converte o valor para o estado booleano
  bool state = (value == 0xFF00);
  coils[addr] = state;
  
  // Atualiza imediatamente a saída digital correspondente:
  switch (addr) {
    case 0: digitalWrite(def_pin_D1, state ? HIGH : LOW); break;
    case 1: digitalWrite(def_pin_D2, state ? HIGH : LOW); break;
    case 2: digitalWrite(def_pin_D3, state ? HIGH : LOW); break;
    case 3: digitalWrite(def_pin_D4, state ? HIGH : LOW); break;
    case 4: digitalWrite(def_pin_RELE, state ? HIGH : LOW); break;
    default: break;
  }
  
  // Construção manual da resposta:
  response.add(request.getServerID());      // ID do servidor
  response.add(request.getFunctionCode());  // Código da função (05)
  response.add((addr >> 8) & 0xFF);         // Byte alto do endereço
  response.add(addr & 0xFF);                // Byte baixo do endereço
  response.add((value >> 8) & 0xFF);        // Byte alto do valor
  response.add(value & 0xFF);               // Byte baixo do valor

  return response;
}


// Escrita de um Holding Register (FC 06)
// Atualiza o vetor e imediatamente configura a saída correspondente
ModbusMessage writeSingleHoldingRegister(ModbusMessage request) {
  ModbusMessage response;
  uint16_t addr = 0, value = 0;
  uint8_t valueAux = 0;
  request.get(2, addr);
  request.get(4, value);
  
  // Verifica se o endereço está dentro do range definido para holding registers
  if (addr >= HRSIZE) {
    response.setError(request.getServerID(), request.getFunctionCode(), ILLEGAL_DATA_ADDRESS);
    return response;
  }
  
  // Atualiza imediatamente a saída correspondente:
  switch (addr) {
    case 0:
      valueAux = map(value,0,65535,0,255); 
      dacWrite(def_pin_DAC1, valueAux);      // Saída DAC (valor de 0 a 255)
      holdingRegisters[addr] = valueAux;      
      break;
    case 1: 
      valueAux = map(value,0,65535,0,1024); 
      analogWrite(def_pin_W4a20_1, valueAux);  // Saída para 4-20mA (via analogWrite)
      holdingRegisters[addr] = valueAux;
      break;
    case 2:
      valueAux = map(value,0,65535,0,1024);     
      analogWrite(def_pin_PWM, valueAux);      // Saída PWM (via analogWrite)
      holdingRegisters[addr] = valueAux;
      break;
    default:
      break;
  }
  
  // Construção manual da resposta:
  response.add(request.getServerID());                    // ID do servidor
  response.add(request.getFunctionCode());                // Código da função (06)
  response.add((addr >> 8) & 0xFF);                         // Byte alto do endereço
  response.add(addr & 0xFF);                                // Byte baixo do endereço
  response.add((value >> 8) & 0xFF);                        // Byte alto do valor
  response.add(value & 0xFF);                               // Byte baixo do valor

  return response;
}


void errorMsg(String error, bool restart) {
    WSerial.println(error);
    if (restart) {
        WSerial.println("Rebooting now...");
        delay(2000);
        ESP.restart();
        delay(2000);
    }
}

// ========================================================
// setup() – inicialização do sistema
// ========================================================
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
    pinMode(def_pin_ADC1, INPUT);
    pinMode(def_pin_DAC1, OUTPUT);
    pinMode(def_pin_D1, OUTPUT);
    pinMode(def_pin_D2, OUTPUT);
    pinMode(def_pin_D3, OUTPUT);
    pinMode(def_pin_D4, OUTPUT);
    pinMode(def_pin_RELE, OUTPUT);
    pinMode(def_pin_W4a20_1, OUTPUT);
    pinMode(def_pin_PWM, OUTPUT);
    pinMode(def_pin_RTN1, INPUT_PULLDOWN);
    pinMode(def_pin_RTN2, INPUT_PULLDOWN);
    pinMode(def_pin_PUSH1, INPUT_PULLDOWN);
    pinMode(def_pin_PUSH2, INPUT_PULLDOWN);
    
    // Configura os estados iniciais das saídas:
    digitalWrite(def_pin_D1, LOW);
    digitalWrite(def_pin_D2, LOW);
    digitalWrite(def_pin_D3, LOW);
    digitalWrite(def_pin_D4, LOW);
    digitalWrite(def_pin_RELE, LOW);
    analogWrite(def_pin_PWM, 0);
    dacWrite(def_pin_DAC1, 0);
    analogWrite(def_pin_W4a20_1, 0);
    ads.begin();

    // Inicializa o servidor Modbus (porta 502, ID 1, timeout 2000ms)
    if (!modbusServer.start(502, 1, 2000)) {
        WSerial.println("Erro ao iniciar o servidor Modbus TCP (eModbus)!");
        while (1);
    }
    WSerial.println("Servidor Modbus TCP (eModbus) iniciado com sucesso!");

    // Registra os callbacks para os respectivos códigos de função    
    modbusServer.registerWorker(1, READ_COIL,           &readCoils);
    modbusServer.registerWorker(1, READ_DISCR_INPUT,    &readDiscreteInputs);
    modbusServer.registerWorker(1, READ_HOLD_REGISTER,  &readHoldingRegisters);
    modbusServer.registerWorker(1, READ_INPUT_REGISTER, &readInputRegisters);
    modbusServer.registerWorker(1, WRITE_COIL,          &writeSingleCoil);
    modbusServer.registerWorker(1, WRITE_HOLD_REGISTER, &writeSingleHoldingRegister);
}

// ========================================================
// loop() – como as leituras e atualizações são feitas nos callbacks,
// o loop pode conter apenas o gerenciamento de OTA, display, serial, etc.
// ========================================================
void loop() {
  OTA::handle();
  updateWSerial(&WSerial);
  updateDisplay(&disp);
  if (wm.getPortalRunning()) wm.process();
}