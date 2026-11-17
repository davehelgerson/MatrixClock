#ifndef AnalogClockface_h
#define AnalogClockface_h

#include <Adafruit_GFX.h>
#include "../core/IClockface.h"
#include "../core/CWDateTime.h"

/**
 * Classic 64x64 analog watch clockface with hour, minute, and second hands.
 */
class AnalogClockface : public IClockface {
  private:
    static const int16_t CX = 31;
    static const int16_t CY = 31;
    static const int16_t NUM_RADIUS = 27;

    static const int16_t HOUR_LEN = 15;
    static const int16_t MIN_LEN  = 21;
    static const int16_t SEC_LEN  = 23;
    static const int16_t SEC_TAIL = 5;

    Adafruit_GFX *_display;
    CWDateTime *_dateTime = nullptr;
    int _lastSecond = -1;

    int16_t _prevHourX = CX;
    int16_t _prevHourY = CY;
    int16_t _prevMinX  = CX;
    int16_t _prevMinY  = CY;
    int16_t _prevSecX  = CX;
    int16_t _prevSecY  = CY;
    int16_t _prevTailX = CX;
    int16_t _prevTailY = CY;
    bool _hasPrevHands = false;

    void drawDial();
    void drawHands(int hour, int minute, int second);
    void erasePrevHands();
    void drawThickHand(int16_t x0, int16_t y0, int16_t x1, int16_t y1, uint16_t color);

  public:
    AnalogClockface(Adafruit_GFX *display);
    void setup(CWDateTime *dateTime) override;
    void update() override;
    void teardown() override;
};

#endif
