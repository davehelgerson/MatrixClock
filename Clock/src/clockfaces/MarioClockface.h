#ifndef MarioClockface_h
#define MarioClockface_h

#include <Adafruit_GFX.h>
#include "../core/IClockface.h"
#include "../core/CWDateTime.h"

class Clockface: public IClockface {
  private:
    Adafruit_GFX* _display;
    CWDateTime* _dateTime;
    void updateTime();

  public:
    Clockface(Adafruit_GFX* display);
    void setup(CWDateTime *dateTime) override;
    void update() override;
    void externalEvent(int type);
};

class Clockface3: public IClockface {
  private:
    Adafruit_GFX* _display;
    CWDateTime* _dateTime;
    void updateTime3();

  public:
    Clockface3(Adafruit_GFX* display);
    void setup(CWDateTime *dateTime) override;
    void update() override;
    void externalEvent3(int type);
};

#endif

