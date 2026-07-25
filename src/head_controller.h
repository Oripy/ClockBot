#define DISABLE_COMPLEX_FUNCTIONS
#ifndef SERVOS_EASING_H
#include <ServoEasing.h>
#endif

class HeadController {
public:
    void init(int pPin, int tPin);
    // void update();
    void setTarget(float p, float t);

private:
    ServoEasing panServo, tiltServo;
    // float currPan, currTilt;
    // float targetPan, targetTilt;
    // float lerpSpeed;
};