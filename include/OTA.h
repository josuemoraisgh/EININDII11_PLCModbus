#ifndef OTA_H
#define OTA_H

#include <Arduino.h>
#include <ArduinoOTA.h>
#include <utility>
#include <vector>
#include <numeric>

static std::function<void()> disableInterruptsCallback = nullptr;

class OTA {
public:
    static void start(const char *hostname, const char *password = "", unsigned int port = 3232, unsigned int interval = 1000) {
        ArduinoOTA.setPort(port);
        ArduinoOTA.setHostname(hostname);
        ArduinoOTA.setPassword(password);

        ArduinoOTA.onStart([]() {
            if (disableInterruptsCallback == nullptr) {
                Serial.println("\nDisabling all pins");
                disableInterruptsAllPins();
            } else {
                Serial.println("\nRunning custom callback to disable interrupts");
                disableInterruptsCallback();
            }
            String type;
            if (ArduinoOTA.getCommand() == U_FLASH)
                type = "sketch";
            else // U_SPIFFS
                type = "filesystem";

            // NOTE: if updating SPIFFS this would be the place to unmount SPIFFS using SPIFFS.end()
            Serial.println("Start updating " + type);
        });
        ArduinoOTA.onEnd([]() {
            Serial.println("\nEnd");
        });
        ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
            Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
        });
        ArduinoOTA.onError([](ota_error_t error) {
            Serial.printf("Error[%u]: ", error);
            if (error == OTA_AUTH_ERROR) Serial.println("Auth Failed");
            else if (error == OTA_BEGIN_ERROR) Serial.println("Begin Failed");
            else if (error == OTA_CONNECT_ERROR) Serial.println("Connect Failed");
            else if (error == OTA_RECEIVE_ERROR) Serial.println("Receive Failed");
            else if (error == OTA_END_ERROR) Serial.println("End Failed");

            Serial.println("Rebooting...");
            delay(100);
            ESP.restart();
        });
        ArduinoOTA.begin();

        Serial.println("Ready");
        Serial.print("IP address: ");
        Serial.println(WiFi.localIP());

        // auto handle = [](TimerHandle_t xTimer) { OTA::handle(); };
        // timer = xTimerCreate(NULL, pdMS_TO_TICKS(interval), true, NULL, handle);
        // xTimerStart(timer, 0);
    }

    static void setDisableInterruptsCallback(std::function<void()> _disableInterruptsCallback) {
        disableInterruptsCallback = std::move(_disableInterruptsCallback);
    }

    static void setDisableInterruptsCallback(const std::vector<int> &_pins) {
        disableInterruptsCallback = std::bind(disableInterruptsDefaultCallback, _pins);
    }

    static void handle() {
        ArduinoOTA.handle();
    }

private:
    // static TimerHandle_t timer;
    static void disableInterruptsAllPins() {
        std::vector<int> pins(40);
        std::iota(pins.begin(), pins.end(), 0);
        disableInterruptsDefaultCallback(pins);
    }

    static void disableInterruptsDefaultCallback(const std::vector<int> &pins) {
        for (int pin: pins) {
            detachInterrupt(pin);
        }
    }
};

#endif //OTA_H