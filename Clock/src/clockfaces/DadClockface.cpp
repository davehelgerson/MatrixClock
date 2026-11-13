#include "DadClockface.h"
#include "ClockfaceCommon.h"
#include "../core/Locator.h"

namespace {
  const uint16_t DAD_BG            = 0x0000;
  const uint16_t DAD_CLOCK_COLOR   = 0xFFFF;
  const uint16_t DAD_LABEL_COLOR   = 0xFED6; // Warm cream-gold
  const uint16_t DAD_COLORS[]      = {
    0x07FF, // Cyan
    0xFFE0, // Gold
    0xFD20, // Orange
    0xF81F, // Pink
    0x07E0, // Green
    0x5DFF  // Sky blue
  };
  const uint8_t DAD_COLOR_COUNT    = sizeof(DAD_COLORS) / sizeof(DAD_COLORS[0]);
  const char* const DAD_MESSAGES[] = {
    "I love you!",
    "You are the best son!",
    "You are amazing!",
    "I love your creativity!",
    "I'm proud of you!",
    "You're my favorite human!",
    "You are the son I always wanted!",
    "You make me happy every day!",
    "You're braver than you know!",
    "I'm so happy that you are my son!",
    "I'm here for you!",
    "I'm lucky to be your dad!",
    "I love adventures with you!",
    "Your smile brightens my day!",
    "Remember to always be kind!",
    "I'm always cheering for you!",
    "Thank you for being you!",
    "You are couragous and strong!"
  };
  const uint8_t  DAD_MSG_COUNT     = sizeof(DAD_MESSAGES) / sizeof(DAD_MESSAGES[0]);
  const uint8_t  DAD_TEXT_SCALE    = 1;
  const int16_t  DAD_CHAR_W        = 6 * DAD_TEXT_SCALE;
  const int16_t  DAD_CHAR_H        = 8 * DAD_TEXT_SCALE;
  const int16_t  DAD_SCROLL_Y      = 28;
  const uint16_t DAD_SCROLL_MS     = 35;
  const uint16_t DAD_CLOCK_POLL_MS = 500;
}

Clockface5::Clockface5(Adafruit_GFX* display)
    : _display(display), _dateTime(nullptr), _msgIndex(-1),
      _scrollX(64), _msgWidth(0), _msgColor(DAD_COLORS[0]),
      _lastScrollMillis(0), _lastClockMillis(0),
      _lastHour(-1), _lastMinute(-1), _lastIsAM(true)
{
  Locator::provide(display);
  Locator::provide(&eventBus);
}

void Clockface5::setup(CWDateTime *dateTime)
{
  _dateTime = dateTime;
  randomSeed(esp_random());

  auto* d = Locator::getDisplay();
  d->fillRect(0, 0, 64, 64, DAD_BG);

  _lastHour = -1;
  _lastMinute = -1;
  drawLabels();
  updateClock();

  _msgIndex = -1;
  pickNextMessage();

  _lastScrollMillis = millis();
  _lastClockMillis = millis();
}

void Clockface5::drawLabels()
{
  auto* d = Locator::getDisplay();
  d->setFont(nullptr);
  d->setTextWrap(false);
  d->setTextSize(1);
  d->setTextColor(DAD_LABEL_COLOR, DAD_BG);

  // Upper-left header in crisp 5x7 font
  d->setCursor(2, 2);
  d->print("To Logan:");

  // Lower-right "- Dad" in crisp 5x7 font with tight 2px gap after dash
  d->fillRect(39, 58, 4, 1, DAD_LABEL_COLOR);
  d->setCursor(45, 55);
  d->print("Dad");
}

void Clockface5::pickNextMessage()
{
  if (_msgIndex < 0) {
    _msgIndex = random(DAD_MSG_COUNT);
  } else {
    int8_t nextIdx = random(DAD_MSG_COUNT - 1);
    if (nextIdx >= _msgIndex) {
      nextIdx++;
    }
    _msgIndex = nextIdx;
  }

  uint8_t nextColorIdx = random(DAD_COLOR_COUNT);
  if (DAD_COLOR_COUNT > 1 && DAD_COLORS[nextColorIdx] == _msgColor) {
    nextColorIdx = (nextColorIdx + 1) % DAD_COLOR_COUNT;
  }
  _msgColor = DAD_COLORS[nextColorIdx];
  _msgWidth = (int16_t)strlen(DAD_MESSAGES[_msgIndex]) * DAD_CHAR_W;
  _scrollX = 64;
}

void Clockface5::updateClock()
{
  if (!_dateTime) return;

  int h24 = _dateTime->getHour();
  int minute = _dateTime->getMinute();
  bool am = _dateTime->isAM();
  int h12 = h24 % 12;
  if (h12 == 0) h12 = 12;

  if (h12 == _lastHour && minute == _lastMinute && am == _lastIsAM) {
    return;
  }
  _lastHour = h12;
  _lastMinute = minute;
  _lastIsAM = am;

  auto* d = Locator::getDisplay();
  d->fillRect(0, 55, 32, 9, DAD_BG);

  int16_t x = 2;
  const int16_t y = 57;

  if (h12 >= 10) {
    drawCompactClockDigit(d, h12 / 10, x, y, DAD_CLOCK_COLOR, 1);
    x += 4;
  }
  drawCompactClockDigit(d, h12 % 10, x, y, DAD_CLOCK_COLOR, 1);
  x += 4;

  d->drawPixel(x, y + 1, DAD_CLOCK_COLOR);
  d->drawPixel(x, y + 3, DAD_CLOCK_COLOR);
  x += 2;

  drawCompactClockDigit(d, minute / 10, x, y, DAD_CLOCK_COLOR, 1);
  x += 4;
  drawCompactClockDigit(d, minute % 10, x, y, DAD_CLOCK_COLOR, 1);
  x += 5;

  drawAmPm(d, am, x, y, DAD_CLOCK_COLOR);
}

void Clockface5::drawScrollingMessage()
{
  auto* d = Locator::getDisplay();
  d->setFont(nullptr);
  d->setTextWrap(false);
  d->setTextSize(1);

  const char* msg = DAD_MESSAGES[_msgIndex];
  size_t len = strlen(msg);

  for (size_t i = 0; i < len; i++) {
    int16_t charX = _scrollX + (int16_t)i * DAD_CHAR_W;
    if (charX + DAD_CHAR_W <= 0) continue;
    if (charX >= 64) break;
    d->drawChar(charX, DAD_SCROLL_Y, (unsigned char)msg[i], _msgColor, DAD_BG, DAD_TEXT_SCALE, DAD_TEXT_SCALE);
  }

  int16_t trailX = _scrollX + _msgWidth;
  if (trailX >= 0 && trailX < 64) {
    d->drawFastVLine(trailX, DAD_SCROLL_Y, DAD_CHAR_H, DAD_BG);
  }
}

void Clockface5::update()
{
  if (!_dateTime) return;

  unsigned long now = millis();
  if ((unsigned long)(now - _lastClockMillis) >= DAD_CLOCK_POLL_MS) {
    _lastClockMillis = now;
    updateClock();
  }

  if ((unsigned long)(now - _lastScrollMillis) >= DAD_SCROLL_MS) {
    _lastScrollMillis = now;
    _scrollX--;
    if (_scrollX < -_msgWidth) {
      pickNextMessage();
    } else {
      drawScrollingMessage();
    }
  }
}

