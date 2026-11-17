#include "ClockfaceCommon.h"

EventBus eventBus;

const char* FORMAT_TWO_DIGITS = "%02d";

const uint8_t CLOCK_DIGITS[10][7] = {
  {0x0E, 0x11, 0x13, 0x15, 0x19, 0x11, 0x0E},
  {0x04, 0x0C, 0x04, 0x04, 0x04, 0x04, 0x0E},
  {0x0E, 0x11, 0x01, 0x02, 0x04, 0x08, 0x1F},
  {0x1F, 0x02, 0x04, 0x02, 0x01, 0x11, 0x0E},
  {0x02, 0x06, 0x0A, 0x12, 0x1F, 0x02, 0x02},
  {0x1F, 0x10, 0x1E, 0x01, 0x01, 0x11, 0x0E},
  {0x06, 0x08, 0x10, 0x1E, 0x11, 0x11, 0x0E},
  {0x1F, 0x01, 0x02, 0x04, 0x08, 0x08, 0x08},
  {0x0E, 0x11, 0x11, 0x0E, 0x11, 0x11, 0x0E},
  {0x0E, 0x11, 0x11, 0x0F, 0x01, 0x02, 0x0C}
};

const uint8_t COMPACT_CLOCK_DIGITS[10][5] = {
  {0x07, 0x05, 0x05, 0x05, 0x07},
  {0x02, 0x06, 0x02, 0x02, 0x07},
  {0x07, 0x01, 0x07, 0x04, 0x07},
  {0x07, 0x01, 0x07, 0x01, 0x07},
  {0x05, 0x05, 0x07, 0x01, 0x01},
  {0x07, 0x04, 0x07, 0x01, 0x07},
  {0x07, 0x04, 0x07, 0x05, 0x07},
  {0x07, 0x01, 0x01, 0x01, 0x01},
  {0x07, 0x05, 0x07, 0x05, 0x07},
  {0x07, 0x05, 0x07, 0x01, 0x07}
};

void drawClockDigit(Adafruit_GFX* display, uint8_t digit, int16_t x, int16_t y, uint16_t color)
{
  for (uint8_t row = 0; row < 7; row++) {
    for (uint8_t col = 0; col < 5; col++) {
      if (CLOCK_DIGITS[digit][row] & (0x10 >> col)) {
        display->drawPixel(x + col, y + row, color);
      }
    }
  }
}

void drawClockNumber(Adafruit_GFX* display, uint8_t value, int16_t x, int16_t y, uint16_t color)
{
  drawClockDigit(display, value / 10, x, y, color);
  drawClockDigit(display, value % 10, x + 6, y, color);
}

void drawCompactClockDigit(Adafruit_GFX* display, uint8_t digit, int16_t x, int16_t y, uint16_t color, uint8_t scale)
{
  for (uint8_t row = 0; row < 5; row++) {
    for (uint8_t col = 0; col < 3; col++) {
      if (COMPACT_CLOCK_DIGITS[digit][row] & (0x04 >> col)) {
        display->fillRect(x + (col * scale), y + (row * scale), scale, scale, color);
      }
    }
  }
}

void drawCompactClockTime(Adafruit_GFX* display, uint8_t hour, uint8_t minute, int16_t x, int16_t y, uint16_t color, uint8_t scale, bool showColon)
{
  drawCompactClockDigit(display, hour / 10, x, y, color, scale);
  drawCompactClockDigit(display, hour % 10, x + (4 * scale), y, color, scale);
  if (showColon) {
    display->fillRect(x + (8 * scale), y + scale, scale, scale, color);
    display->fillRect(x + (8 * scale), y + (3 * scale), scale, scale, color);
  }
  drawCompactClockDigit(display, minute / 10, x + (10 * scale), y, color, scale);
  drawCompactClockDigit(display, minute % 10, x + (14 * scale), y, color, scale);
}

// Draws 7x5 pixel-art "AM" or "PM" at (lx, ly).
void drawAmPm(Adafruit_GFX* d, bool am, int16_t lx, int16_t ly, uint16_t color)
{
  if (am) {
    d->drawPixel(lx + 1, ly,     color);
    d->drawPixel(lx,     ly + 1, color); d->drawPixel(lx + 2, ly + 1, color);
    d->fillRect( lx,     ly + 2, 3, 1, color);
    d->drawPixel(lx,     ly + 3, color); d->drawPixel(lx + 2, ly + 3, color);
    d->drawPixel(lx,     ly + 4, color); d->drawPixel(lx + 2, ly + 4, color);
  } else {
    d->fillRect( lx,     ly,     2, 1, color);
    d->drawPixel(lx,     ly + 1, color); d->drawPixel(lx + 2, ly + 1, color);
    d->fillRect( lx,     ly + 2, 2, 1, color);
    d->drawPixel(lx,     ly + 3, color);
    d->drawPixel(lx,     ly + 4, color);
  }
  const int16_t mx = lx + 4;
  d->drawPixel(mx,     ly,     color); d->drawPixel(mx + 2, ly,     color);
  d->fillRect( mx,     ly + 1, 3, 1, color);
  d->drawPixel(mx,     ly + 2, color); d->drawPixel(mx + 2, ly + 2, color);
  d->drawPixel(mx,     ly + 3, color); d->drawPixel(mx + 2, ly + 3, color);
  d->drawPixel(mx,     ly + 4, color); d->drawPixel(mx + 2, ly + 4, color);
}

