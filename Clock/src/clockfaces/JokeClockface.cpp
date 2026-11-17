#include "JokeClockface.h"
#include "ClockfaceCommon.h"
#include "../core/WifiConfig.h"
#include "../fonts/picopixel.h"

namespace {
const uint16_t COLOR_BLACK  = 0x0000;
const uint16_t COLOR_GOLD   = 0xFE40;
const uint16_t COLOR_WHITE  = 0xFFFF;
const uint16_t JOKE_COLORS[] = {
  0x07FF, // Cyan
  0xFFE0, // Yellow
  0x07E0, // Green
  0xF81F, // Pink
  0x5DFF  // Sky blue
};
const uint8_t JOKE_COLOR_COUNT = sizeof(JOKE_COLORS) / sizeof(JOKE_COLORS[0]);
const int16_t CHAR_W = 6;
const int16_t CHAR_H = 8;
const int16_t SCROLL_Y = 28;
}  // namespace

JokeClockface::JokeClockface(Adafruit_GFX *display)
  : _display(display)
{
}

void JokeClockface::setup(CWDateTime *dateTime)
{
  _dateTime = dateTime;
  randomSeed(esp_random());

  if (_display) {
    _display->fillRect(0, 0, 64, 64, COLOR_BLACK);
  }

  _lastHour = -1;
  _lastMinute = -1;
  _loadedDay = -1;

  drawHeader();
  loadOrFetchTodaysJoke();
  updateClock();

  _lastScrollMillis = millis();
  _lastClockMillis = millis();
}

void JokeClockface::teardown()
{
  if (_display) {
    _display->fillRect(0, 0, 64, 64, COLOR_BLACK);
  }
}

void JokeClockface::loadOrFetchTodaysJoke()
{
  if (!_dateTime) return;

  int y = _dateTime->getYear();
  int m = _dateTime->getMonth();
  int d = _dateTime->getDay();
  _loadedYear = y;
  _loadedMonth = m;
  _loadedDay = d;

  char dateBuf[16];
  snprintf(dateBuf, sizeof(dateBuf), "%04d-%02d-%02d", y, m, d);
  String today(dateBuf);

  String cachedDate, cachedJoke;
  if (!WifiConfig::loadDailyJoke(cachedDate, cachedJoke) || cachedDate != today) {
    WifiConfig::fetchDailyJokeIfNeeded(y, m, d, true);
    WifiConfig::loadDailyJoke(cachedDate, cachedJoke);
  }

  _jokeText = (cachedJoke.length() > 0) ? cachedJoke : WifiConfig::getFallbackDailyJoke(y, m, d);
  _jokeColor = JOKE_COLORS[random(JOKE_COLOR_COUNT)];
  _msgWidth = static_cast<int16_t>(_jokeText.length()) * CHAR_W;
  _scrollX = 64;

  if (_display) {
    _display->fillRect(0, SCROLL_Y, 64, CHAR_H, COLOR_BLACK);
  }
}

void JokeClockface::drawHeader()
{
  if (!_display) return;

  _display->setFont(&Picopixel);
  _display->setTextSize(1);
  _display->setTextColor(COLOR_GOLD);
  _display->setCursor(3, 7);
  _display->print("DAILY JOKE");
}

void JokeClockface::updateClock()
{
  if (!_dateTime || !_display) return;

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

  _display->fillRect(0, 54, 64, 10, COLOR_BLACK);

  int16_t x = 3;
  const int16_t y = 56;
  if (h12 >= 10) {
    drawCompactClockDigit(_display, h12 / 10, x, y, COLOR_WHITE, 1);
    x += 4;
  }
  drawCompactClockDigit(_display, h12 % 10, x, y, COLOR_WHITE, 1);
  x += 4;
  _display->drawPixel(x, y + 1, COLOR_WHITE);
  _display->drawPixel(x, y + 3, COLOR_WHITE);
  x += 2;
  drawCompactClockDigit(_display, minute / 10, x, y, COLOR_WHITE, 1);
  x += 4;
  drawCompactClockDigit(_display, minute % 10, x, y, COLOR_WHITE, 1);
  x += 5;
  drawAmPm(_display, am, x, y, COLOR_GOLD);
}

void JokeClockface::drawScrollingJoke()
{
  if (!_display) return;
  _display->setFont(nullptr);
  _display->setTextWrap(false);
  _display->setTextSize(1);

  size_t len = _jokeText.length();
  for (size_t i = 0; i < len; i++) {
    int16_t charX = _scrollX + static_cast<int16_t>(i) * CHAR_W;
    if (charX + CHAR_W <= 0) continue;
    if (charX >= 64) break;
    _display->drawChar(charX, SCROLL_Y, static_cast<unsigned char>(_jokeText[i]),
                       _jokeColor, COLOR_BLACK, 1, 1);
  }

  int16_t trailX = _scrollX + _msgWidth;
  if (trailX >= 0 && trailX < 64) {
    _display->drawFastVLine(trailX, SCROLL_Y, CHAR_H, COLOR_BLACK);
  }
}

void JokeClockface::update()
{
  if (!_dateTime || !_display) return;

  unsigned long now = millis();
  if ((now - _lastClockMillis) >= CLOCK_POLL_MS) {
    _lastClockMillis = now;
    if (_dateTime->getDay() != _loadedDay ||
        _dateTime->getMonth() != _loadedMonth ||
        _dateTime->getYear() != _loadedYear) {
      loadOrFetchTodaysJoke();
    }
    updateClock();
  }

  if ((now - _lastScrollMillis) >= SCROLL_MS) {
    _lastScrollMillis = now;
    _scrollX--;
    if (_scrollX < -_msgWidth) {
      _scrollX = 64;
      _jokeColor = JOKE_COLORS[random(JOKE_COLOR_COUNT)];
    } else {
      drawScrollingJoke();
    }
  }
}
