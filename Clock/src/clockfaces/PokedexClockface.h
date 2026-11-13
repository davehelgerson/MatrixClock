#ifndef PokedexClockface_h
#define PokedexClockface_h

#include <Adafruit_GFX.h>
#include "../core/IClockface.h"
#include "../core/CWDateTime.h"

class Clockface1: public IClockface {
  private:
    Adafruit_GFX* _display;
    CWDateTime* _dateTime;
    unsigned long _lastMillis = 0;

  public:
    Clockface1(Adafruit_GFX* display);
    void setup(CWDateTime *dateTime) override;
    void update() override;
    void refreshDate(uint8_t weekday, uint16_t color);
    void refreshTime();
    void updatePokemon();
    void updateLoadingBar(uint8_t seconds);
};

#endif

