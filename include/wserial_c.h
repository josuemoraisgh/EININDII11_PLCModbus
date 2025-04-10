#ifndef __WSERIAL_H
#define __WSERIAL_H

#include <Arduino.h>
#include "ESPTelnet.h"

#define BAUD_RATE 115200
class WSerial_c : public ESPTelnet
{
protected: 
  //uint16_t server_port = 0;
  void update(void);
  bool start(uint16_t port, unsigned long baudrate = BAUD_RATE);

public:
  WSerial_c() : ESPTelnet() {};
  template <typename T>
  void plot(const char *varName, T x, T y, const char *unit = NULL);
  template <typename T>
  void plot(const char *varName, T y, const char *unit = NULL);
  template <typename T>
  void print(const T &data);
  template <typename T>
  void print(const T &data, int base);
  void println();
  template <typename T>
  void println(const T &data);
  template <typename T>
  void println(const T &data, int base);

  
  friend inline bool startWSerial(WSerial_c *ws,uint16_t port, unsigned long baudrate = BAUD_RATE);
  friend inline void updateWSerial(WSerial_c *ws); 
  uint16_t serverPort() { return (this->server_port); }
  bool isConnected();
};

inline bool WSerial_c::isConnected()
{
  return ((ESPTelnet *) this)->isConnected();
}

inline bool startWSerial(WSerial_c *ws,uint16_t port, unsigned long baudrate){return(ws->start(port, baudrate));}
bool WSerial_c::start(uint16_t port, unsigned long baudrate)
{
  if(isConnected()) ((ESPTelnet *) this)->stop();
  server_port = port;
  onDisconnect([](String ip)
               {
        Serial.print("- Telnet: ");
        Serial.print(ip);
        Serial.println(" disconnected"); });
  onConnectionAttempt([](String ip)
                      {
        Serial.print("- Telnet: ");
        Serial.print(ip);
        Serial.println(" tried to connected"); });
  onReconnect([](String ip)
              {
        Serial.print("- Telnet: ");
        Serial.print(ip);
        Serial.println(" reconnected"); });

  return (((ESPTelnet *) this)->begin(server_port));
}

inline void updateWSerial(WSerial_c *ws) {ws->update();}
void WSerial_c::update(void)
{
  if (isConnected())
  {
    if (Serial.available() && on_input != NULL)
    {
      on_input(Serial.readStringUntil('\n'));
    }
  }
  ((ESPTelnet *) this)->loop();
}

template <typename T>
void WSerial_c::plot(const char *varName, T y, const char *unit)
{
  plot(varName, (T)millis(), y, unit);
}
template <typename T>
void WSerial_c::plot(const char *varName, T x, T y, const char *unit)
{
  print(">"); // Inicio de envio de dados para um gráfico.
  print(varName);
  print(":");
  print(x);
  print(":");
  print(y);
  if (unit != NULL)
  {
    print("§"); // Unidade na sequência
    print(unit);
  }
  println("|g"); // Modo Grafico
}

template <typename T>
void WSerial_c::print(const T &data)
{
  if (((ESPTelnet *) this)->isConnected())
    ((ESPTelnet *) this)->print(data);  
  else
    Serial.print(data);  
}

template <typename T>
void WSerial_c::print(const T &data, int base)
{
  if (((ESPTelnet *) this)->isConnected())
    ((ESPTelnet *) this)->print(data, base);  
  else
    Serial.print(data, base);  
}

template <typename T>
void WSerial_c::println(const T &data)
{
  if (((ESPTelnet *) this)->isConnected())
    ((ESPTelnet *) this)->println(data);  
  else
    Serial.println(data);  
}

template <typename T>
void WSerial_c::println(const T &data, int base)
{
  if (((ESPTelnet *) this)->isConnected())
    ((ESPTelnet *) this)->println(data, base);  
  else
    Serial.println(data, base);  
}

void WSerial_c::println()
{
  if (((ESPTelnet *) this)->isConnected())
    ((ESPTelnet *) this)->println();  
  else
    Serial.println();
}
#endif