#ifndef MatrixRainClockface_h
#define MatrixRainClockface_h

#include <Adafruit_GFX.h>
#include "../core/IClockface.h"
#include "../core/CWDateTime.h"

/**
 * 64x64 Matrix digital rain animation with randomized column positions,
 * speeds, trail lengths, and glyph flicker.
 */
class MatrixRainClockface : public IClockface {
  private:
    static const int NUM_DROPS = 26;
    static const unsigned long FRAME_MS = 35;

    struct Drop {
      int8_t x;
      int16_t headY;
      int8_t speed;     // 1..4 (lower tick threshold = faster)
      int8_t length;
      uint8_t tick;
    };

    Adafruit_GFX *_display;
    Drop _drops[NUM_DROPS];
    unsigned long _lastFrameMillis = 0;

    void initDrop(int idx, bool randomStart);

  public:
    MatrixRainClockface(Adafruit_GFX *display);
    void setup(CWDateTime *dateTime) override;
    void update() override;
    void teardown() override;
};

#endif
