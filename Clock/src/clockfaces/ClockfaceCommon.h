#ifndef ClockfaceCommon_h
#define ClockfaceCommon_h

#include <Adafruit_GFX.h>
#include "../core/EventBus.h"

extern EventBus eventBus;
extern const char* FORMAT_TWO_DIGITS;

void drawClockDigit(Adafruit_GFX* display, uint8_t digit, int16_t x, int16_t y, uint16_t color);
void drawClockNumber(Adafruit_GFX* display, uint8_t value, int16_t x, int16_t y, uint16_t color);
void drawCompactClockDigit(Adafruit_GFX* display, uint8_t digit, int16_t x, int16_t y, uint16_t color, uint8_t scale);
void drawCompactClockTime(Adafruit_GFX* display, uint8_t hour, uint8_t minute, int16_t x, int16_t y, uint16_t color, uint8_t scale, bool showColon = true);
void drawAmPm(Adafruit_GFX* d, bool am, int16_t lx, int16_t ly, uint16_t color);

#endif

