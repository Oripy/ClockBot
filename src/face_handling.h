#include "Face.h"

class FaceHandler {
    public:
        void init();
        void update();
        void setActive();
        void setAsleep();
        void setIdle(); // Alias for setActive, as idle is the default active state
    private:
        void updateLookPosition(float x, float y);
        Face *face;
        bool isAsleep;
        enum Direction { UP, DOWN, LEFT, RIGHT };
        Direction moveDirection = Direction::DOWN;
        float lastLookX = 0.5; // Default to center
        float lastLookY = 0.5; // Default to center
};