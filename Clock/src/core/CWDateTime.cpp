#include "CWDateTime.h"
#include <Arduino.h>
#include "../hardware/pin_config.h"
#include "../hardware/I2CBus.h"
#include <WiFi.h>
#include <time.h>

const char* ssid = "HWIFI_LAB";
const char* password = "TestTest1";
const char* ntpServer = "pool.ntp.org";
const long gmtOffset_sec = -18000;  // UTC-5 for EST, adjust for your timezone
const int daylightOffset_sec = 3600;

static const unsigned long WIFI_CONNECT_TIMEOUT_MS = 15000UL;
static const unsigned long RTC_RESYNC_INTERVAL_MS  = 30000UL;

char daysOfTheWeek[7][12] = {"Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday"};

void CWDateTime::begin()
{
  delay(500);  // give RTC time to power up
  I2CBus::init();
  Wire.begin(TOUCH_SDA, TOUCH_SCL);

  {
    I2CBus::Guard guard;
    if (!rtc.begin()) {
      Serial.println("Couldn't find RTC!");
      while (1) {
        delay(100);
      }
    }
  }

  // Connect to WiFi with timeout so offline boot still uses RTC time
  Serial.print("Connecting to WiFi");
  WiFi.begin(ssid, password);
  unsigned long wifiStart = millis();
  while (WiFi.status() != WL_CONNECTED && (millis() - wifiStart) < WIFI_CONNECT_TIMEOUT_MS) {
    delay(500);
    Serial.print(".");
  }

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println(" connected");

    // Sync time from NTP
    configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);

    struct tm timeinfo;
    if (getLocalTime(&timeinfo)) {
      I2CBus::Guard guard;
      rtc.adjust(DateTime(
        timeinfo.tm_year + 1900,
        timeinfo.tm_mon + 1,
        timeinfo.tm_mday,
        timeinfo.tm_hour,
        timeinfo.tm_min,
        timeinfo.tm_sec
      ));
      Serial.println("RTC set from NTP");
    } else {
      Serial.println("NTP failed, using existing RTC time");
    }
  } else {
    Serial.println(" WiFi timeout, using existing RTC time");
  }

  {
    I2CBus::Guard guard;
    DateTime current = rtc.now();
    _cachedUnixTime = current.unixtime();
    _lastSyncMillis = millis();
    Serial.println("RTC ready");
    Serial.println(current.timestamp());
  }

  WiFi.disconnect(true);
  WiFi.mode(WIFI_OFF);
}

DateTime CWDateTime::now()
{
  unsigned long elapsed = millis() - _lastSyncMillis;
  if (_cachedUnixTime == 0 || elapsed >= RTC_RESYNC_INTERVAL_MS) {
    I2CBus::Guard guard;
    if (guard.locked()) {
      DateTime hw = rtc.now();
      _cachedUnixTime = hw.unixtime();
      _lastSyncMillis = millis();
      return hw;
    }
  }
  return DateTime(_cachedUnixTime + (uint32_t)(elapsed / 1000UL));
}

char *CWDateTime::getHour(const char *format)
{
  static char buffer[3] = {'\0'};
  snprintf(buffer, sizeof(buffer), format, now().hour());
  return buffer;
}

char *CWDateTime::getMinute(const char *format)
{
  static char buffer[3] = {'\0'};
  snprintf(buffer, sizeof(buffer), format, now().minute());
  return buffer;
}

char *CWDateTime::getSecond(const char *format)
{
  static char buffer[3] = {'\0'};
  snprintf(buffer, sizeof(buffer), format, now().second());
  return buffer;
}

int CWDateTime::getHour()
{
  return now().hour();
}

int CWDateTime::getMinute()
{
  return now().minute();
}

int CWDateTime::getSecond()
{
  return now().second();
}

int CWDateTime::getWeekday()
{
  return now().dayOfTheWeek();
}

bool CWDateTime::isAM()
{
  return now().hour() < 12;
}

int CWDateTime::getDay()
{
  return now().day();
}

int CWDateTime::getMonth()
{
  return now().month();
}
