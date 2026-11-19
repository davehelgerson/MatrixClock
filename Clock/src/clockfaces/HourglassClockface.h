#ifndef HourglassClockface_h
#define HourglassClockface_h

#include <Adafruit_GFX.h>
#include "../core/IClockface.h"
#include "../core/CWDateTime.h"

/**
 * 64x64 Hourglass clockface displaying three hourglasses whose sand empties
 * over the day, hour, and minute, with the current time in the lower left.
 */
class HourglassClockface : public IClockface {
  private:
    static const int16_t MATRIX_SIZE = 64;
    static const int16_t BULB_HEIGHT = 20;
    static const int16_t MAX_BULB_PIXELS = 216;
    static const int16_t GLASS_CY    = 30;
    static const uint8_t HOURGLASS_COUNT = 3;
    static const unsigned long FRAME_INTERVAL_MS = 60;

    struct SandCell {
      int8_t dx;
      uint8_t dist;
    };

    Adafruit_GFX *_display;
    CWDateTime *_dateTime = nullptr;

    unsigned long _lastFrameMillis = 0;
    unsigned long _secStartMillis = 0;
    int _lastSecond = -1;
    int _lastHour12 = -1;
    int _lastMinute = -1;
    bool _lastIsAM = true;
    uint32_t _animTick = 0;

    static bool _tablesReady;
    static int _bulbPixels[HOURGLASS_COUNT];
    static SandCell _topOrder[HOURGLASS_COUNT][MAX_BULB_PIXELS];
    static SandCell _bottomOrder[HOURGLASS_COUNT][MAX_BULB_PIXELS];

    static void initTables();
    void drawLabel(char ch, int16_t cx, int16_t y, uint16_t color);
    void drawHourglassFrame(uint8_t idx, int16_t cx);
    void drawHourglassSand(uint8_t idx, int16_t cx, float progress);
    void drawTime(int hour24, int minute, bool isAM, bool force);

  public:
    HourglassClockface(Adafruit_GFX *display);
    void setup(CWDateTime *dateTime) override;
    void update() override;
    void teardown() override;
};

#endif
