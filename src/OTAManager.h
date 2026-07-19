#ifndef OTAMANAGER_H
#define OTAMANAGER_H

#include <Arduino.h>
#include <ArduinoOTA.h>

class OtaManager {
public:
    void begin();
    void handle();
};

#endif
