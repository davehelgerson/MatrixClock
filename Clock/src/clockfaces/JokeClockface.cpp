#include "JokeClockface.h"
#include "ClockfaceCommon.h"
#include "../core/WifiConfig.h"
#include "../fonts/picopixel.h"
#include <new>

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

struct JokeFetchRequest {
  int year;
  int month;
  int day;
};

volatile bool gJokeFetchInProgress = false;
volatile uint32_t gJokeFetchGeneration = 0;

void jokeFetchTask(void *pvParameters)
{
  JokeFetchRequest *req = static_cast<JokeFetchRequest *>(pvParameters);
  if (req) {
    WifiConfig::fetchDailyJokeIfNeeded(req->year, req->month, req->day, true);
    delete req;
  }
  __atomic_add_fetch(&gJokeFetchGeneration, 1U, __ATOMIC_RELEASE);
  __atomic_store_n(&gJokeFetchInProgress, false, __ATOMIC_RELEASE);
  vTaskDelete(nullptr);
}

void startBackgroundJokeFetch(int year, int month, int day)
{
  if (__atomic_exchange_n(&gJokeFetchInProgress, true, __ATOMIC_ACQ_REL)) {
    return;
  }
  JokeFetchRequest *req = new (std::nothrow) JokeFetchRequest{ year, month, day };
  if (!req) {
    __atomic_store_n(&gJokeFetchInProgress, false, __ATOMIC_RELEASE);
    return;
  }
  BaseType_t ok = xTaskCreatePinnedToCore(
    jokeFetchTask, "Task_JokeFetch", 12288, req, 1, nullptr, 0);
  if (ok != pdPASS) {
    delete req;
    __atomic_store_n(&gJokeFetchInProgress, false, __ATOMIC_RELEASE);
  }
}
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
  _seenFetchGeneration = __atomic_load_n(&gJokeFetchGeneration, __ATOMIC_ACQUIRE);

  drawHeader();
  if (_dateTime) {
    TimeSnapshot snap = _dateTime->snapshot();
    loadOrFetchTodaysJoke(snap, true);
    updateClock(snap);
  }

  _lastScrollMillis = millis();
  _lastClockMillis = millis();
}

void JokeClockface::teardown()
{
  if (_display) {
    _display->fillRect(0, 0, 64, 64, COLOR_BLACK);
  }
}

void JokeClockface::loadOrFetchTodaysJoke(const TimeSnapshot &snap, bool allowBackgroundFetch)
{
  int y = snap.year;
  int m = snap.month;
  int d = snap.day;
  _loadedYear = y;
  _loadedMonth = m;
  _loadedDay = d;

  char dateBuf[16];
  snprintf(dateBuf, sizeof(dateBuf), "%04d-%02d-%02d", y, m, d);
  String today(dateBuf);

  String cachedDate, cachedJoke;
  bool hasTodayJoke = WifiConfig::loadDailyJoke(cachedDate, cachedJoke) && (cachedDate == today);
  if (!hasTodayJoke && allowBackgroundFetch) {
    // Fetch asynchronously on Core 0 so the LED matrix and encoder stay responsive.
    startBackgroundJokeFetch(y, m, d);
  }

  String nextText = (hasTodayJoke && cachedJoke.length() > 0)
                    ? cachedJoke
                    : WifiConfig::getFallbackDailyJoke(y, m, d);
  if (nextText != _jokeText) {
    _jokeText = nextText;
    _jokeColor = JOKE_COLORS[random(JOKE_COLOR_COUNT)];
    _msgWidth = static_cast<int16_t>(_jokeText.length()) * CHAR_W;
    _scrollX = 64;

    if (_display) {
      _display->fillRect(0, SCROLL_Y, 64, CHAR_H, COLOR_BLACK);
    }
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

void JokeClockface::updateClock(const TimeSnapshot &snap)
{
  if (!_display) return;

  int h24 = snap.hour;
  int minute = snap.minute;
  bool am = snap.isAM;
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
    TimeSnapshot snap = _dateTime->snapshot();

    uint32_t gen = __atomic_load_n(&gJokeFetchGeneration, __ATOMIC_ACQUIRE);
    if (gen != _seenFetchGeneration) {
      _seenFetchGeneration = gen;
      loadOrFetchTodaysJoke(snap, false);
    } else if (snap.day != _loadedDay ||
               snap.month != _loadedMonth ||
               snap.year != _loadedYear) {
      loadOrFetchTodaysJoke(snap, true);
    }
    updateClock(snap);
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
