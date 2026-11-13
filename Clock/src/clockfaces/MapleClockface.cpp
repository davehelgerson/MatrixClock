#include "MapleClockface.h"
#include "ClockfaceCommon.h"
#include "../core/Locator.h"

Clockface4::Clockface4(Adafruit_GFX* display)
    : _display(display), _dateTime(nullptr), _maple(4, MAPLE_CAT_GROUND_Y),
      _lastMinMillis(0), _nextWalkDir(RIGHT)
{
  Locator::provide(display);
  Locator::provide(&eventBus);
}

void Clockface4::setup(CWDateTime *dateTime)
{
  _dateTime = dateTime;
  drawBackground();
  updateClock();
  _maple.init();
  _lastMinMillis = millis();
}

void Clockface4::drawBackground()
{
  auto* d = Locator::getDisplay();

  d->fillRect(0, 0, 64, 64, MAPLE_BG);
  d->fillRect(0, MAPLE_FLOOR_Y, 64, 64 - MAPLE_FLOOR_Y, MAPLE_FLOOR);
  d->fillCircle(3, 12, 3, MAPLE_MOON);

  d->drawPixel(5,  2,  MAPLE_TIME);
  d->drawPixel(1,  6,  MAPLE_TIME);
  d->drawPixel(62, 4,  MAPLE_TIME);
  d->drawPixel(60, 10, MAPLE_TIME);
  d->drawPixel(14, 29, MAPLE_TIME);
  d->drawPixel(28, 28, MAPLE_TIME);
  d->drawPixel(42, 30, MAPLE_TIME);
}

void Clockface4::updateClock()
{
  auto* d = Locator::getDisplay();

  int h24 = _dateTime->getHour();
  int h12 = h24 % 12;
  if (h12 == 0) h12 = 12;
  bool am = _dateTime->isAM();

  d->fillRect(7, 4, 52, 23, MAPLE_BG);

  drawCompactClockTime(d, h12, _dateTime->getMinute(), 7, 4, MAPLE_TIME, 3);
  if (h12 < 10) {
    d->fillRect(7, 4, 12, 15, MAPLE_BG);
  }

  drawAmPm(d, am, 45, 22, MAPLE_TIME);
}

void Clockface4::update()
{
  if (!_dateTime) return;
  _maple.update();

  if (_dateTime->getSecond() == 0 && millis() - _lastMinMillis > 1000) {
    _lastMinMillis = millis();
    updateClock();

    if (_dateTime->getMinute() == 0) {
      _maple.jump();
    } else {
      _maple.walk(_nextWalkDir);
      _nextWalkDir = (_nextWalkDir == RIGHT) ? LEFT : RIGHT;
    }
  }
}

