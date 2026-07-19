#include "face_handling.h"

void FaceHandler::init() {
    face = new Face(128, 64, 42); // Initialize the Face object with width, height, and eye size

    face->Behavior.SetEmotion(eEmotions::Happy, 1.0);
    face->Behavior.SetEmotion(eEmotions::Angry, 1.0);
    face->Behavior.SetEmotion(eEmotions::Sleepy, 1.0);
    face->Behavior.SetEmotion(eEmotions::Normal, 1.0);
    face->Behavior.SetEmotion(eEmotions::Scared, 1.0);
    face->Behavior.SetEmotion(eEmotions::Awe, 1.0);
    face->Behavior.SetEmotion(eEmotions::Surprised, 1.0);

    isAsleep = false;
    updateLookPosition(lastLookX, lastLookY); // Center the eyes initially
    setActive(); // Set the face to active mode by default
}

void FaceHandler::update() {
    if (face) {
        face->Update(); // Call the Update method of the Face object
    }
}

void FaceHandler::setIdle() {
    if (!face) {
        return;
    }

    // Configure behaviors
    face->RandomBehavior = true;
    face->Behavior.Timer.SetIntervalMillis(300000);
    face->RandomBlink = true;
    face->Blink.Timer.SetIntervalMillis(7000); // Blink roughly every 7s
    face->RandomLook = false;

    isAsleep = false;
}

void FaceHandler::setActive() {
    if (!face) {
        return;
    }

    // Configure behaviors
    face->RandomBehavior = true;
    face->Behavior.Timer.SetIntervalMillis(5000);
    face->RandomBlink = true;
    // face->Blink.Timer.SetIntervalMillis(3500); // Blink roughly every 3.5s
    face->Blink.Timer.SetIntervalMillis(500); // Blink roughly every 3.5s
    face->RandomLook = true;

    isAsleep = false;
}

void FaceHandler::setAsleep() {
    if (!face) {
        return;
    }

    if (isAsleep) {
        if (moveDirection == Direction::DOWN) {
            float newy = lastLookY + 0.001; // Move the eyes down slightly
            if (newy > 1.0) {
                newy = 1.0;
                moveDirection = Direction::UP; // Change direction to up
            }
            updateLookPosition(lastLookX, newy);
        } else {
            float newy = lastLookY - 0.001; // Move the eyes up slightly
            if (newy < 0.0) {
                newy = 0.0;
                moveDirection = Direction::DOWN; // Change direction to down
            }
            updateLookPosition(lastLookX, newy);
        }
        return;
    } else { 
        // Configure behaviors
        face->RandomBehavior = false;
        face->RandomBlink = false;
        face->RandomLook = false;
        face->Behavior.GoToEmotion(eEmotions::Sleepy);
        updateLookPosition(0.5, 0.5); // Center the eyes
        isAsleep = true;
    }
}

void FaceHandler::updateLookPosition(float x, float y) {
    if (!face) {
        return;
    }
    face->Look.LookAt(x, y);
    lastLookX = x;
    lastLookY = y;
    // Serial.print("Updated look position to: (");
    // Serial.print(lastLookX);
    // Serial.print(", ");
    // Serial.print(lastLookY);
    // Serial.println(")");
}