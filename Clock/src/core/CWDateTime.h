#ifndef CWDateTimeCf_h
#define CWDateTimeCf_h

#include <Arduino.h>
#include <RTClib.h>

class CWDateTime
{
  private:
    RTC_PCF8563 rtc;
    uint32_t _cachedUnixTime = 0;
    unsigned long _lastSyncMillis = 0;

    DateTime now();

  public:
    void begin();
    bool syncTimeNtp();

    char *getHour(const char *format);
    char *getMinute(const char *format);
    char *getSecond(const char *format);
    int getHour();
    int getMinute();
    int getSecond();

    int getWeekday();
    bool isAM();

    int getDay();
    int getMonth();
};

#endif
