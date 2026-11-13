#ifndef StarsClockface_h
#define StarsClockface_h

#include <Adafruit_GFX.h>
#include "../core/IClockface.h"
#include "../core/CWDateTime.h"

/**
 * Animated starfield clockface that continuously spawns colored LED pixels
 * darting across the 64x64 matrix in random directions and speeds.
 */
class StarsClockface : public IClockface {
  private:
    static const uint8_t MAX_STARS = 28;
    static const unsigned long FRAME_INTERVAL_MS = 30;
    static const unsigned long SPAWN_INTERVAL_MS = 65;

    struct Star {
      float x;
      float y;
      float vx;
      float vy;
      uint16_t color;
      uint16_t tailColor;
      int8_t prevX;
      int8_t prevY;
      int8_t tailX;
      int8_t tailY;
      bool hasPrev;
      bool hasTail;
      bool active;
    };

    Adafruit_GFX *_display;
    Star _stars[MAX_STARS];
    unsigned long _lastFrameMillis = 0;
    unsigned long _lastSpawnMillis = 0;

    void spawnStar(Star &star);
    static uint16_t randomStarColor();
    static uint16_t dimRgb565(uint16_t color);

  public:
    StarsClockface(Adafruit_GFX *display);
    void setup(CWDateTime *dateTime) override;
    void update() override;
    void teardown() override;
};

#endif

