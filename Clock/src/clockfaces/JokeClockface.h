#ifndef JokeClockface_h
#define JokeClockface_h

#include <Adafruit_GFX.h>
#include "../core/IClockface.h"
#include "../core/CWDateTime.h"

/**
 * Kid's Joke of the Day clockface.
 * Uses the daily joke cached in NVS (fetched at startup or once when a new day starts).
 */
class JokeClockface : public IClockface {
  private:
    static const uint16_t SCROLL_MS = 38;
    static const uint16_t CLOCK_POLL_MS = 500;

    Adafruit_GFX *_display;
    CWDateTime *_dateTime = nullptr;

    String _jokeText;
    uint16_t _jokeColor = 0x07FF;
    int16_t _scrollX = 64;
    int16_t _msgWidth = 0;

    int _loadedYear = -1;
    int _loadedMonth = -1;
    int _loadedDay = -1;
    int8_t _lastHour = -1;
    int8_t _lastMinute = -1;
    bool _lastIsAM = true;

    unsigned long _lastScrollMillis = 0;
    unsigned long _lastClockMillis = 0;
    uint32_t _seenFetchGeneration = 0;

    void loadOrFetchTodaysJoke(const TimeSnapshot &snap, bool allowBackgroundFetch);
    void drawHeader();
    void updateClock(const TimeSnapshot &snap);
    void drawScrollingJoke();

  public:
    JokeClockface(Adafruit_GFX *display);
    void setup(CWDateTime *dateTime) override;
    void update() override;
    void teardown() override;
};

#endif
