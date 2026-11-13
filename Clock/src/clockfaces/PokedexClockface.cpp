#include "PokedexClockface.h"
#include "ClockfaceCommon.h"
#include "../core/Locator.h"
#include "../assets/assets.h"
#include "../fonts/PKMN_RBYGSC4pt7b.h"

#define LIGHT_GREEN 0x754d
#define DARK_GREEN 0x0264
#define DARK_BLUE 0x016D
#define LIGHT_BLUE 0x24fe
#define LIGHT_BLACK 0X10c4

namespace {
  const uint16_t* const pokemons[] PROGMEM = {
    pokemon1, pokemon2, pokemon3, pokemon4, pokemon5, pokemon6, pokemon7
  };
}

Clockface1::Clockface1(Adafruit_GFX* display)
    : _display(display), _dateTime(nullptr)
{
  Locator::provide(display);
  Locator::provide(&eventBus);
}

void Clockface1::setup(CWDateTime *dateTime)
{
  _dateTime = dateTime;

  Locator::getDisplay()->fillRect(0, 0, 64, 64, 0x0000);
  Locator::getDisplay()->drawRGBBitmap(0, 0, POKEDEX_BG, 64, 64);
  Locator::getDisplay()->setTextSize(1);
  Locator::getDisplay()->setFont(&PKMN_RBYGSC4pt7b);

  refreshTime();
  refreshDate(_dateTime->getWeekday(), DARK_BLUE);
  updatePokemon();
  _lastMillis = millis();
}

void Clockface1::update()
{
  if (!_dateTime) return;

  if (millis() - _lastMillis >= 1000) {
    uint8_t seconds = _dateTime->getSecond();

    refreshTime();
    updatePokemon();
    if (_dateTime->getMinute() == 0 && seconds == 0) {
      uint8_t wd = _dateTime->getWeekday();
      refreshDate((wd == 0 ? 6 : wd - 1), LIGHT_BLUE);
      refreshDate(wd, DARK_BLUE);
    }

    updateLoadingBar(seconds);

    uint16_t color = random(LONG_MAX);
    Locator::getDisplay()->fillRect(5, 4, 2, 4, color);
    Locator::getDisplay()->fillRect(4, 5, 4, 2, color);

    _lastMillis = millis();
  }
}

void Clockface1::updatePokemon()
{
  Locator::getDisplay()->drawRGBBitmap(8, 21, pokemons[random(sizeof(pokemons) / sizeof(pokemons[0]))], 16, 16);
}

void Clockface1::refreshTime()
{
  Locator::getDisplay()->fillRect(35, 17, 26, 14, LIGHT_BLACK);
  drawClockNumber(Locator::getDisplay(), _dateTime->getHour(), 36, 17, 0xffff);
  drawClockNumber(Locator::getDisplay(), _dateTime->getMinute(), 49, 24, 0xffff);
}

void Clockface1::refreshDate(uint8_t weekday, uint16_t color)
{
  uint8_t x = 36 + ((weekday > 3 ? (weekday - 4) : weekday) * 6);
  uint8_t y = 35 + (weekday > 3 ? 5 : 0);

  Locator::getDisplay()->fillRect(x, y, 5, 4, color);
}

void Clockface1::updateLoadingBar(uint8_t seconds)
{
  if (seconds == 0) {
    Locator::getDisplay()->fillRect(9, 53, 11, 5, LIGHT_GREEN);
  } else {
    Locator::getDisplay()->fillRect(9, 53, ((10 * seconds) / 59) + 1, 5, DARK_GREEN);
  }
}

