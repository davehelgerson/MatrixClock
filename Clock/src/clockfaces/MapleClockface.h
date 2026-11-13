#ifndef MapleClockface_h
#define MapleClockface_h

#include <Adafruit_GFX.h>
#include "../core/IClockface.h"
#include "../core/CWDateTime.h"
#include "../core/Game.h"
#include "../sprites/maple.h"

class Clockface4: public IClockface {
  private:
    Adafruit_GFX* _display;
    CWDateTime* _dateTime;
    MapleCat _maple;
    unsigned long _lastMinMillis;
    Direction _nextWalkDir;

    void drawBackground();
    void updateClock();

  public:
    Clockface4(Adafruit_GFX* display);
    void setup(CWDateTime *dateTime) override;
    void update() override;
};

#endif

