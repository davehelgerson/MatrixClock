#ifndef CWDateTimeCf_h
#define CWDateTimeCf_h

#include <Arduino.h>
#include <RTClib.h>
#include <time.h>

struct TimeSnapshot
{
  int year;
  int month;
  int day;
  int hour;
  int minute;
  int second;
  int weekday;
  bool isAM;
};

class CWDateTime
{
  private:
    RTC_PCF8563 rtc;
    portMUX_TYPE _cacheMux = portMUX_INITIALIZER_UNLOCKED;
    uint32_t _cachedUnixTime = 0;
    unsigned long _lastSyncMillis = 0;

    DateTime now();
    bool updateRtcFromTm(const struct tm &timeinfo, const char *logMsg);

  public:
    void begin();
    bool syncTimeNtp();
    bool applyTimeZone();
    void formatDateTime(char *buf, size_t len);
    TimeSnapshot snapshot();

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
    int getYear();
};

#endif
