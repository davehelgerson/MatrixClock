#ifndef Maple_h
#define Maple_h

#include "../core/Game.h"
#include "../core/Locator.h"

// Night scene palette
const uint16_t MAPLE_BG    = 0x080F;  // dark navy night sky
const uint16_t MAPLE_CAT   = 0x0000;  // black cat (off-pixels)
const uint16_t MAPLE_EYE   = 0x07E0;  // bright green eyes
const uint16_t MAPLE_MOON  = 0xFFE0;  // yellow moon
const uint16_t MAPLE_FLOOR = 0x2082;  // dark brownish-gray floor
const uint16_t MAPLE_TIME  = 0xFFFF;  // white time display

const uint8_t  MAPLE_CAT_W       = 10;
const uint8_t  MAPLE_CAT_H       = 13;
const uint8_t  MAPLE_JUMP_HEIGHT = 12;

const int16_t MAPLE_CAT_GROUND_Y = 43;  // paws land at y+13 = 56 (floor)
const int16_t MAPLE_FLOOR_Y      = 56;

const uint16_t MAPLE_WALK_SPEED = 100;
const uint16_t MAPLE_JUMP_SPEED = 55;

class MapleCat {
  public:
    enum State { SITTING, WALKING, JUMPING };

    MapleCat(int16_t x, int16_t y);
    void init();
    void walk(Direction dir);
    void jump();
    void update();
    State getState() const { return _state; }

  private:
    int16_t _x, _y;
    int16_t _lastX, _lastY;
    State _state;
    Direction _walkDir;
    Direction _jumpDir;
    int16_t _jumpBaseY;
    int16_t _targetX;
    bool _walkFrame;
    unsigned long _lastMillis;

    void erase();
    void drawAt(int16_t x, int16_t y);
};

#endif
