#include <ESP32Servo.h>

class HeadController {
public:
    void init(int pPin, int tPin);
    void update();
    void setTarget(float p, float t);

private:
    Servo panServo, tiltServo;
    float currPan, currTilt;
    float targetPan, targetTilt;
    float lerpSpeed;
};
