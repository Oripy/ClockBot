#include <Arduino.h>
#include <Wire.h>
#include <WiFiManager.h>
#include "OTAManager.h"
#include <OneButton.h>
#include "face_handling.h"
#include <RTClib.h>
#include <TM1637Display.h>
#include <ServoEasing.hpp>
#include "head_controller.h"
#include <FastLED.h>
#include <Preferences.h>

#define DEBUG false

Preferences preferences;

// Pin Definitions
#define TM1637_CLK D0
#define TM1637_DIO D1
#define TILT_SERVO_PIN D2
#define PAN_SERVO_PIN D3
#define SDA_PIN D4
#define SCL_PIN D5
#define LEFT_BUTTON_PIN D6
#define RIGHT_BUTTON_PIN D7
#define LED_PIN D9

#define LONG_PRESS_DURATION 1500 // Duration in milliseconds for long press detection
#define NUM_LEDS 3
CRGB leds[NUM_LEDS];

enum Modes {
  MODE_SETUP,
  MODE_IDLE,
  MODE_NIGHT,
  MODE_ACTIVE,
  MODE_GAME
};

// Initialize display and RTC
TM1637Display display(TM1637_CLK, TM1637_DIO);
RTC_DS3231 rtc;

// Initialize OneButton on LEFT_BUTTON_PIN (Active LOW, pullup enabled)
OneButton btn_left(LEFT_BUTTON_PIN, true, true);
OneButton btn_right(RIGHT_BUTTON_PIN, true, true);

// State Machine Variables
enum ClockState { IDLE_MODE, NIGHT_MODE, ACTIVE_MODE, SET_HOURS, SET_MINUTES };
ClockState currentState = IDLE_MODE;

// Temporary variables used during editing
int editHour = 0;
int editMinute = 0;
int lastFrameUpdate = 0;
int lastInteractionTime = 0;

// Blinking effect variables (used in setup mode)
unsigned long lastBlinkTime = 0;
bool blinkState = true;
const int BLINK_INTERVAL = 300; // Blink every 300ms

int LEDcolor = 0;

WiFiManager wm;
int night_mode_time = 1200; // Default 20:00
int day_mode_time = 480; // Default to 8:00
bool portalClosed = false;
unsigned long portalStartTime = 0;

OtaManager ota;
FaceHandler faceHandler;
HeadController head;

String minutesToTimeStr(int totalMinutes) {
  int hours = totalMinutes / 60;
  int minutes = totalMinutes % 60;
  char buf[6];
  snprintf(buf, sizeof(buf), "%02d:%02d", hours, minutes);
  return String(buf);
}

int timeStrToMinutes(String timeStr) {
  int firstColon = timeStr.indexOf(':');
  if (firstColon == -1) return 0;
  
  int hours = timeStr.substring(0, firstColon).toInt();
  int minutes = timeStr.substring(firstColon + 1).toInt();
  
  // Bound check
  if (hours < 0) hours = 0;
  if (hours > 23) hours = 23;
  if (minutes < 0) minutes = 0;
  if (minutes > 59) minutes = 59;
  
  return (hours * 60) + minutes;
}

void saveConfigCallback() {
    #if DEBUG
    Serial.println("Parsing and saving new time configuration...");
    #endif
    
    // Read the "HH:MM" strings submitted via the portal
    String valStr1 = wm.getParameters()[0]->getValue();
    String valStr2 = wm.getParameters()[1]->getValue();

    // Convert strings to integer minutes with built-in validation checks
    if (valStr1.length() > 0) {
        night_mode_time = timeStrToMinutes(valStr1);
    }
    if (valStr2.length() > 0) {
        day_mode_time = timeStrToMinutes(valStr2);
    }

    // Save the integer values to ESP32 Preferences (Flash)
    preferences.begin("config", false);
    preferences.putInt("time1", night_mode_time);
    preferences.putInt("time2", day_mode_time);
    preferences.end();

    #if DEBUG
    Serial.printf("Saved new times -> Night Start: %s (%d mins), Night End: %s (%d mins)\n", 
                minutesToTimeStr(night_mode_time).c_str(), night_mode_time,
                minutesToTimeStr(day_mode_time).c_str(), day_mode_time);
    #endif
}

// ==========================================
// BUTTON EVENT CALLBACK FUNCTIONS
// ==========================================

void handleLeftClick() {
    lastInteractionTime = millis(); // Reset idle timer on any interaction
    switch (currentState) {
        case SET_HOURS:
            editHour = (editHour + 1) % 24; // Increment hour, wrap around at 24
            Serial.print("Hours edited to: "); Serial.println(editHour);
            break;
        case SET_MINUTES:
            editMinute = (editMinute + 1) % 60; // Increment minute, wrap around at 60
            Serial.print("Minutes edited to: "); Serial.println(editMinute);
            break;
        case IDLE_MODE:
            currentState = ACTIVE_MODE;
            break;
        default:
            // In NORMAL_MODE, left click does nothing
            break;
    }
}

void handleRightClick() {
    lastInteractionTime = millis(); // Reset idle timer on any interaction
    switch (currentState) {
        case SET_HOURS:
            editHour = (editHour + 23) % 24; // Decrement hour by 1 (while making sure it doesn't go below 0), wrap around at 24
            Serial.print("Hours edited to: "); Serial.println(editHour);
            break;
        case SET_MINUTES:
            editMinute = (editMinute + 59) % 60; // Decrement minute, wrap around at 60
            Serial.print("Minutes edited to: "); Serial.println(editMinute);
            break;
        case IDLE_MODE:
            currentState = ACTIVE_MODE;
            break;
        default:
            // In NORMAL_MODE, right click does nothing
            break;
    }
}

// Triggers once the button has been held down for the duration of the long-press threshold
void handleLeftLongPressStart() {
    lastInteractionTime = millis(); // Reset idle timer on any interaction
    switch (currentState) {
        case NIGHT_MODE: // A COMMENTER SUR LA VERSION FINALE
        case IDLE_MODE: {
            // Enter setup: Fetch current time first to start editing from there
            DateTime now = rtc.now();  
            editHour = now.hour();
            editMinute = now.minute();
            currentState = SET_HOURS;
            Serial.println("Entered Setup: Setting HOURS");
            break;
        }
        case SET_HOURS: {
            // Move to setting minutes
            currentState = SET_MINUTES;
            Serial.println("Setting MINUTES");
            break;
        }
        case SET_MINUTES: {
            // Save new time to the physical RTC module
            DateTime now_after_setup = rtc.now();
            rtc.adjust(DateTime(now_after_setup.year(), now_after_setup.month(), now_after_setup.day(), editHour, editMinute, 0));
            
            currentState = IDLE_MODE;
            Serial.println("Time Saved! Exited Setup.");
            break;
        }
    }
}

void handleRightLongPressStart() {
    lastInteractionTime = millis(); // Reset idle timer on any interaction
    switch (currentState) {
        default:
            // In NORMAL_MODE or any other state, right long press does nothing
            break;
    }
}

// ==========================================
// STATE EXECUTION METHODS
// ==========================================

void updateClock() {
    DateTime now = rtc.now();
    int displayTime = (now.hour() * 100) + now.minute();
    
    // Blink the center colon based on seconds tick
    bool showColon = (now.second() % 2 == 0);
    display.showNumberDecEx(displayTime, showColon ? 0b01000000 : 0, true);

    int now_minutes = now.hour()*60 + now.minute();
    if (now_minutes >= night_mode_time || now_minutes < day_mode_time) {
        currentState = NIGHT_MODE;
    } else if (currentState == NIGHT_MODE) {
        currentState = IDLE_MODE;
    }
}

void runSetupMode() {
    // Non-blocking timer to toggle blinking state
    if (millis() - lastBlinkTime >= BLINK_INTERVAL) {
        blinkState = !blinkState;
        lastBlinkTime = millis();
    }

    uint8_t data[4] = {0, 0, 0, 0};

    // Convert editing values to raw segment arrays
    int h1 = editHour / 10;
    int h2 = editHour % 10;
    int m1 = editMinute / 10;
    int m2 = editMinute % 10;

    // Encode digits to TM1637 raw data format
    data[0] = display.encodeDigit(h1);
    data[1] = display.encodeDigit(h2);
    data[2] = display.encodeDigit(m1);
    data[3] = display.encodeDigit(m2);

    // Apply Blink effect based on current configuration state
    if (currentState == SET_HOURS) {
        if (!blinkState) {
        data[0] = 0x00; // Turn off first digit
        data[1] = 0x00; // Turn off second digit
        }
    } 
    else if (currentState == SET_MINUTES) {
        if (!blinkState) {
        data[2] = 0x00; // Turn off third digit
        data[3] = 0x00; // Turn off fourth digit
        }
    }

    // Draw the custom raw segment array to the display
    // Or, with the colon turned on solidly (0x80) to signify edit mode
    data[1] |= 0b10000000; 
    display.setSegments(data);
}

// ==========================================
// SETUP & MAIN LOOP
// ==========================================

void setup() {
    #if DEBUG
    Serial.begin(115200);
    delay(2000); // Allow time for Serial Monitor to initialize
    Serial.println("ClockBot Starting...");
    #endif

    preferences.begin("config", false);
    night_mode_time = preferences.getInt("time1", 1200);  // Default 20:00
    day_mode_time = preferences.getInt("time2", 480); // Default 8:00
    preferences.end();

    #if DEBUG
    Serial.printf("Night Mode (Minutes): %d (%s)\n", night_mode_time, minutesToTimeStr(night_mode_time).c_str());
    Serial.printf("Day Mode (Minutes): %d (%s)\n", day_mode_time, minutesToTimeStr(day_mode_time).c_str());
    #endif

    String night_mode_time_str = minutesToTimeStr(night_mode_time);
    String day_mode_time_str = minutesToTimeStr(day_mode_time);

    wm.setConfigPortalBlocking(false);
    wm.setConfigPortalTimeout(600);
    wm.setSaveParamsCallback(saveConfigCallback);

    WiFiManagerParameter night_mode_time_field("time1", "Night Start Time (HH:MM)", night_mode_time_str.c_str(), 6, "type=\"time\"");
    WiFiManagerParameter day_mode_time_field("time2", "Night End Time (HH:MM)", day_mode_time_str.c_str(), 6, "type=\"time\"");

    wm.addParameter(&night_mode_time_field);
    wm.addParameter(&day_mode_time_field);

    #if DEBUG
    Serial.println("Starting Non-Blocking Config Portal...");
    #endif
    wm.startConfigPortal("ClockBot", "ClockBot");
    portalClosed = false;
    portalStartTime = millis();

    display.setBrightness(3);

    // FastLED.addLeds<WS2812, D9>(leds, NUM_LEDS);
    FastLED.addLeds<WS2812B, LED_PIN, GRB>(leds, NUM_LEDS);
    FastLED.setBrightness(20);
    FastLED.clear();

    Wire.begin();

    faceHandler.init();

    head.init(PAN_SERVO_PIN, TILT_SERVO_PIN);

    #if DEBUG
    if (!rtc.begin()) {
        Serial.println("Couldn't find RTC module!");
    }
    #endif

    // Configure OneButton events and timing settings
    btn_left.setPressMs(LONG_PRESS_DURATION);
    btn_left.attachClick(handleLeftClick);
    btn_left.attachLongPressStart(handleLeftLongPressStart);
    btn_right.setPressMs(LONG_PRESS_DURATION);
    btn_right.attachClick(handleRightClick);
    btn_right.attachLongPressStart(handleRightLongPressStart);
}

ClockState lastState = IDLE_MODE;

void loop() {
    if (WiFi.status() == WL_CONNECTED) {
        #if DEBUG
        Serial.println("OTA...");
        #endif
        ota.handle();
    }

    if (!portalClosed) {
        if (millis() - portalStartTime >= 60000) {
            #if DEBUG
            Serial.println("Portal timeout reached. Closing config portal...");
            #endif
            
            // Stop the configuration portal server/DNS gracefully
            wm.stopConfigPortal();
            portalClosed = true;
            
            #if DEBUG
            Serial.println("Portal closed");
            #endif

            if (wm.autoConnect("ClockBot", "ClockBot")) {
                #if DEBUG
                Serial.println("Connected to Wi-Fi successfully!");
                #endif
                delay(1000);
                ota.begin();
            } else {
                #if DEBUG
                Serial.println("Failed to connect to Wi-Fi, OTA impossible");
                #endif
            }
        } else {
            // Keep the portal responsive
            wm.process();
        }
    }
    
    // Keep monitoring the physical button state
    btn_left.tick();
    btn_right.tick();

    leds[1] = CHSV(LEDcolor, 255, 255);
    leds[2] = CHSV(LEDcolor, 255, 255);
    // leds[1] = CRGB(255, 0, 0);
    LEDcolor = (LEDcolor + 1) % 256; // Increment hue for next frame
    FastLED.show();
    
    faceHandler.update(); // Update the Face object (handles eye movement, blinking, etc.)
    if (millis() % 5000 < 50) { // Every 5 seconds
        head.setTarget(random(0, 180), random(0, 180));
    }
    if (currentState != lastState) {
        #if DEBUG
        Serial.print("State changed to: ");
        switch (currentState) {
            case IDLE_MODE: Serial.println("IDLE_MODE"); break;
            case NIGHT_MODE: Serial.println("NIGHT_MODE"); break;
            case ACTIVE_MODE: Serial.println("ACTIVE_MODE"); break;
            case SET_HOURS: Serial.println("SET_HOURS"); break;
            case SET_MINUTES: Serial.println("SET_MINUTES"); break;
        }
        #endif
        lastState = currentState;
    }

    // Run the display routines
    if (millis() - lastFrameUpdate >= 30) { // only update every 10ms (100FPS)
        // if (millis() - lastInteractionTime > 30000 && currentState != NIGHT_MODE) { // 30 seconds of inactivity
        if (millis() - lastInteractionTime > 10000 && currentState != NIGHT_MODE && currentState != SET_HOURS && currentState != SET_MINUTES) { // 10 seconds of inactivity
            currentState = IDLE_MODE; // Return to idle mode
        }
        switch (currentState) {
            case IDLE_MODE:
                display.setBrightness(3); // Normal brightness for idle mode
                updateClock();
                faceHandler.setIdle();
                break;
            case ACTIVE_MODE:
                display.setBrightness(3); // Normal brightness for idle mode
                updateClock();
                faceHandler.setActive();
                break;
            case NIGHT_MODE:
                display.setBrightness(1); // Dim display for night mode
                updateClock();
                faceHandler.setAsleep();
                break;
            case SET_HOURS:
            case SET_MINUTES:
                runSetupMode();
                break;
        }
        lastFrameUpdate = millis(); // Update the clock update time
    }
}