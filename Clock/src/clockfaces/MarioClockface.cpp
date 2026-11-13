#include "MarioClockface.h"
#include "ClockfaceCommon.h"
#include "../core/Tile.h"
#include "../core/Object.h"
#include "../core/Locator.h"
#include "../core/Game.h"
#include "../assets/assets.h"
#include "../fonts/Super_Mario_Bros__24pt7b.h"
#include "../sprites/mario.h"
#include "../sprites/block.h"

namespace {
  // Clockface (seconds)
  Tile ground(GROUND, 8, 8);
  Object bush(BUSH, 21, 9);
  Object cloud1(CLOUD1, 13, 12);
  Object cloud2(CLOUD2, 13, 12);
  Object cloud3(CLOUD3, 17, 6);
  Object hill(HILL, 20, 22);

  Mario mario(21, 40);
  Block hourBlock(3, 8);
  Block minuteBlock(22, 8);
  Block secondBlock(41, 8);
  int lastSecond = -1;

  // Clockface3 (HH:MM)
  Tile ground3(GROUND, 8, 8);
  Object bush3(BUSH, 21, 9);
  Object cloud13(CLOUD1, 13, 12);
  Object cloud23(CLOUD2, 13, 12);
  Object hill3(HILL, 20, 22);

  Mario mario3(23, 40);
  Block hourBlock3(13, 8);
  Block minuteBlock3(32, 8);
  unsigned long lastMillis3 = 0;
  unsigned long lastMillis3Walk = 0;
}

Clockface::Clockface(Adafruit_GFX* display)
    : _display(display), _dateTime(nullptr)
{
  Locator::provide(display);
  Locator::provide(&eventBus);
}

void Clockface::setup(CWDateTime *dateTime)
{
  _dateTime = dateTime;

  Locator::getDisplay()->setTextSize(1);
  Locator::getDisplay()->setFont(&Super_Mario_Bros__24pt7b);
  Locator::getDisplay()->fillRect(0, 0, 64, 64, SKY_COLOR);

  ground.fillRow(DISPLAY_HEIGHT - ground._height);

  bush.draw(43, 47);
  hill.draw(0, 34);
  cloud1.draw(0, 25);
  cloud2.draw(51, 30);
  cloud3.draw(30, 0);

  updateTime();

  hourBlock.init();
  minuteBlock.init();
  secondBlock.init();
  mario.init();
}

void Clockface::update()
{
  if (!_dateTime) return;

  hourBlock.update();
  minuteBlock.update();
  secondBlock.update();
  mario.update();

  int updatedSecond = _dateTime->getSecond();
  if (lastSecond != updatedSecond) {
    mario.jump();
    updateTime();
    lastSecond = updatedSecond;
  }
}

void Clockface::externalEvent(int type)
{
  if (type == 0) {
    mario.jump();
    updateTime();
  }
}

void Clockface::updateTime()
{
  hourBlock.setText(String(_dateTime->getHour(FORMAT_TWO_DIGITS)));
  minuteBlock.setText(String(_dateTime->getMinute(FORMAT_TWO_DIGITS)));
  secondBlock.setText(String(_dateTime->getSecond(FORMAT_TWO_DIGITS)));
}

//**********************************************************************************************

Clockface3::Clockface3(Adafruit_GFX* display)
    : _display(display), _dateTime(nullptr)
{
  Locator::provide(display);
  Locator::provide(&eventBus);
}

void Clockface3::setup(CWDateTime *dateTime)
{
  _dateTime = dateTime;

  Locator::getDisplay()->setTextSize(1);
  Locator::getDisplay()->setFont(&Super_Mario_Bros__24pt7b);
  Locator::getDisplay()->fillRect(0, 0, 64, 64, SKY_COLOR);

  ground3.fillRow(DISPLAY_HEIGHT - ground3._height);

  bush3.draw(43, 47);
  hill3.draw(0, 34);
  cloud13.draw(0, 21);
  cloud23.draw(51, 7);

  updateTime3();

  hourBlock3.init();
  minuteBlock3.init();
  mario3.init();
}

void Clockface3::update()
{
  if (!_dateTime) return;

  hourBlock3.update();
  minuteBlock3.update();
  mario3.update();

  int second = _dateTime->getSecond();
  if (second == 0 && millis() - lastMillis3 > 1000) {
    mario3.jump();
    updateTime3();
    minuteBlock3.init();
    lastMillis3 = millis();
  }

  if (second == 30 && millis() - lastMillis3Walk > 1000) {
    mario3.walk(RIGHT);
    lastMillis3Walk = millis();
  }
}

void Clockface3::updateTime3()
{
  minuteBlock3.setText(String(_dateTime->getMinute(FORMAT_TWO_DIGITS)));
  hourBlock3.setText(String(_dateTime->getHour(FORMAT_TWO_DIGITS)));
  minuteBlock3.setText(String(_dateTime->getMinute(FORMAT_TWO_DIGITS)));
}

void Clockface3::externalEvent3(int type)
{
  if (type == 0) {
    mario3.jump();
    updateTime3();
  }
}

