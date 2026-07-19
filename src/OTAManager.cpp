#include "OTAManager.h"

void OtaManager::begin() {
    Serial.println("Starting OTA...");
    ArduinoOTA.setHostname("ClockBot");
    ArduinoOTA.onStart([]() { Serial.println("OTA Start"); });
    ArduinoOTA.onEnd([]() { Serial.println("\nOTA End"); });
    ArduinoOTA.begin();
}

void OtaManager::handle() {
    ArduinoOTA.handle();
}
