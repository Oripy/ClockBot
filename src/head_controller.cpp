#include "head_controller.h"

void HeadController::init(int pPin, int tPin) {
  lerpSpeed = 0.03; // Smoothness factor
  panServo.setPeriodHertz(50);
  panServo.attach(pPin, 500, 2400);
  tiltServo.setPeriodHertz(50);
  tiltServo.attach(tPin, 500, 2400);
  currPan = targetPan = 90.0;
  currTilt = targetTilt = 90.0;
  panServo.write((int)currPan);
  tiltServo.write((int)currTilt);
}

void HeadController::setTarget(float p, float t) {
  targetPan = constrain(p, 0, 180);
  targetTilt = constrain(t, 0, 180);
}

void HeadController::update() {
  // Linear Interpolation (LERP)
  currPan += (targetPan - currPan) * lerpSpeed;
  currTilt += (targetTilt - currTilt) * lerpSpeed;
  
  panServo.write((int)currPan);
  tiltServo.write((int)currTilt);
}