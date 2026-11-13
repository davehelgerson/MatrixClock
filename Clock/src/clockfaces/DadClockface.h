#ifndef DadClockface_h
#define DadClockface_h

#include <Adafruit_GFX.h>
#include "../core/IClockface.h"
#include "../core/CWDateTime.h"

class Clockface5: public IClockface {
  private:
    Adafruit_GFX* _display;
    CWDateTime* _dateTime;
    int8_t _msgIndex;
    int16_t _scrollX;
    int16_t _msgWidth;
    uint16_t _msgColor;
    unsigned long _lastScrollMillis;
    unsigned long _lastClockMillis;
    int8_t _lastHour;
    int8_t _lastMinute;
    bool _lastIsAM;

    void pickNextMessage();
    void updateClock();
    void drawLabels();
    void drawScrollingMessage();

  public:
    Clockface5(Adafruit_GFX* display);
    void setup(CWDateTime *dateTime) override;
    void update() override;
};

#endif

