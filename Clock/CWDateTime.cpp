#include "CWDateTime.h"
#include <Arduino.h>
#include "pin_config.h"
#include <WiFi.h>
#include <time.h>

const char* ssid = "HWIFI_LAB";
const char* password = "TestTest1";
const char* ntpServer = "pool.ntp.org";
const long gmtOffset_sec = -18000;  // UTC-5 for EST, adjust for your timezone
const int daylightOffset_sec = 3600;

char daysOfTheWeek[7][12] = {"Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday"};

//**********************************************************************************************
// void CWDateTime::begin()
// {
//   Wire.begin(TOUCH_SDA, TOUCH_SCL);
//   if (!rtc.begin()) {
//     Serial.println("Couldn't find RTC!");
//     while (1);
//   }

//   if (rtc.lostPower()) {
//     Serial.println("RTC lost power, please set the time!");
//   }
// }
void CWDateTime::begin()
{
  delay(500);  // give RTC time to power up
  Wire.begin(TOUCH_SDA, TOUCH_SCL);
  if (!rtc.begin()) {
    Serial.println("Couldn't find RTC!");
    while (1);
  }

  // Connect to WiFi
  Serial.print("Connecting to WiFi");
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println(" connected");

  // Sync time from NTP
  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
  
  struct tm timeinfo;
  if (getLocalTime(&timeinfo)) {
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

  Serial.println("RTC ready");
  Serial.println(rtc.now().timestamp());

  WiFi.disconnect(true);
  WiFi.mode(WIFI_OFF);
}
//**********************************************************************************************
char *CWDateTime::getHour(const char *format)
{
  static char buffer[3] = {'\0'};
  DateTime now = rtc.now();
  snprintf(buffer, sizeof(buffer), format, now.hour());
  return buffer;
}
//**********************************************************************************************
char *CWDateTime::getMinute(const char *format)
{
  static char buffer[3] = {'\0'};
  DateTime now = rtc.now();
  snprintf(buffer, sizeof(buffer), format, now.minute());
  return buffer;
}
//**********************************************************************************************
char *CWDateTime::getSecond(const char *format)
{
  static char buffer[3] = {'\0'};
  DateTime now = rtc.now();
  snprintf(buffer, sizeof(buffer), format, now.second());
  return buffer;
}
//**********************************************************************************************
int CWDateTime::getHour()
{
  DateTime now = rtc.now();
  return now.hour();
}
//**********************************************************************************************
int CWDateTime::getMinute()
{
  DateTime now = rtc.now();
  return now.minute();
}
//**********************************************************************************************
int CWDateTime::getSecond()
{
  DateTime now = rtc.now();
  return now.second();
}
//**********************************************************************************************
int CWDateTime::getWeekday() 
{
  DateTime now = rtc.now();
  return now.dayOfTheWeek();
}
//**********************************************************************************************
bool CWDateTime::isAM() 
{
  DateTime now = rtc.now();
  return now.hour() < 12; // 判断是否为上午
}
//**********************************************************************************************
int CWDateTime::getDay() 
{
  DateTime now = rtc.now();
  return now.day();
}
//**********************************************************************************************
int CWDateTime::getMonth()
{
  DateTime now = rtc.now();
  return now.month();
}
//**********************************************************************************************