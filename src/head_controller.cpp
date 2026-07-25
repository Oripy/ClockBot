#include "head_controller.h"

#define START_DEGREE_VALUE 0

void HeadController::init(int pPin, int tPin) {
  // lerpSpeed = 0.03; // Smoothness factor
  panServo.attach(pPin, START_DEGREE_VALUE, DEFAULT_MICROSECONDS_FOR_0_DEGREE, DEFAULT_MICROSECONDS_FOR_180_DEGREE, -90, 90);
  tiltServo.attach(tPin, START_DEGREE_VALUE, DEFAULT_MICROSECONDS_FOR_0_DEGREE, DEFAULT_MICROSECONDS_FOR_180_DEGREE, -90, 90);
  
  panServo.setSpeed(10); // Set speed for pan servo
  tiltServo.setSpeed(10); // Set speed for tilt servo

  panServo.setEasingType(EASE_CUBIC_IN_OUT);
  tiltServo.setEasingType(EASE_CUBIC_IN_OUT);
  
  // delay(DELAY_BETWEEN_ACTIONS_MILLIS);
}

void HeadController::setTarget(float p, float t) {
  panServo.startEaseTo(constrain(p, 0, 180));
  tiltServo.startEaseTo(constrain(t, 0, 180));
}

// void HeadController::update() {
//   // Linear Interpolation (LERP)
//   currPan += (targetPan - currPan) * lerpSpeed;
//   currTilt += (targetTilt - currTilt) * lerpSpeed;
  
//   panServo.write((int)currPan);
//   tiltServo.write((int)currTilt);
// }