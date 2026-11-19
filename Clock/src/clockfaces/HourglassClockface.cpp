#include "HourglassClockface.h"
#include "ClockfaceCommon.h"
#include <algorithm>
#include <math.h>

namespace {
const uint16_t COLOR_BLACK       = 0x0000;
const uint16_t COLOR_GLASS       = 0x52AA;
const uint16_t COLOR_GLASS_SHINE = 0x9CF3;
const uint16_t COLOR_CAP_LIGHT   = 0xC4A3;
const uint16_t COLOR_CAP_DARK    = 0x7A41;
const uint16_t COLOR_TIME        = 0xFFFF;
const uint16_t COLOR_AMPM        = 0xFD20;

const int16_t HOURGLASS_CX[3] = { 13, 35, 53 };
const int16_t CAP_HALF_WIDTH[3] = { 9, 7, 5 };
const char HOURGLASS_LABEL[3] = { 'D', 'H', 'M' };

// Interior half-width for each row distance (0 = neck, 20 = cap interior),
// graduated from widest (Day) -> medium (Hour) -> slimmest (Minute).
const int8_t HALF_WIDTH[3][21] = {
  // Day: max interior 15px (cap 19px), 216 pixels per bulb
  { 0, 1, 1, 2, 2, 3, 3, 4, 4, 5, 5, 6, 6, 7, 7, 7, 7, 7, 7, 7, 7 },
  // Hour: max interior 11px (cap 15px), 180 pixels per bulb
  { 0, 1, 1, 2, 2, 3, 3, 4, 4, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5 },
  // Minute: max interior 7px (cap 11px), 126 pixels per bulb
  { 0, 1, 1, 2, 2, 2, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3 }
};

struct SandPalette {
  uint16_t base;
  uint16_t light;
  uint16_t dark;
  uint16_t label;
  uint8_t streamStride;
  uint8_t streamDivider;
};

const SandPalette PALETTES[3] = {
  // Day: warm amber-gold, gentle trickle
  { 0xFC60, 0xFFE4, 0xB2A0, 0xFD20, 5, 6 },
  // Hour: aqua-cyan, steady trickle
  { 0x05FD, 0x7FFF, 0x03D3, 0x06BF, 4, 4 },
  // Minute: coral-rose, calm falling stream
  { 0xFA68, 0xFDD0, 0xB184, 0xFB0B, 3, 3 }
};

uint16_t grainColor(uint8_t idx, int16_t x, int16_t y)
{
  uint8_t h = static_cast<uint8_t>((x * 37 + y * 17 + idx * 53) & 0x07);
  if (h == 0) return PALETTES[idx].light;
  if (h == 1) return PALETTES[idx].dark;
  return PALETTES[idx].base;
}

inline int iabs(int v)
{
  return (v < 0) ? -v : v;
}
}  // namespace

bool HourglassClockface::_tablesReady = false;
int HourglassClockface::_bulbPixels[HourglassClockface::HOURGLASS_COUNT] = { 0, 0, 0 };
HourglassClockface::SandCell HourglassClockface::_topOrder[HourglassClockface::HOURGLASS_COUNT][HourglassClockface::MAX_BULB_PIXELS];
HourglassClockface::SandCell HourglassClockface::_bottomOrder[HourglassClockface::HOURGLASS_COUNT][HourglassClockface::MAX_BULB_PIXELS];

HourglassClockface::HourglassClockface(Adafruit_GFX *display)
  : _display(display)
{
}

void HourglassClockface::initTables()
{
  if (_tablesReady) return;

  for (uint8_t hg = 0; hg < HOURGLASS_COUNT; ++hg) {
    int count = 0;
    for (uint8_t dist = 1; dist <= BULB_HEIGHT; ++dist) {
      int8_t hw = HALF_WIDTH[hg][dist];
      for (int8_t dx = -hw; dx <= hw; ++dx) {
        if (count < MAX_BULB_PIXELS) {
          _topOrder[hg][count] = { dx, dist };
          _bottomOrder[hg][count] = { dx, dist };
          ++count;
        }
      }
    }
    _bulbPixels[hg] = count;

    // Top bulb: smaller key remains longest (near neck, outer edges hold slightly
    // higher than the center so a natural funnel dip forms as sand drains).
    std::sort(_topOrder[hg], _topOrder[hg] + count, [](const SandCell &a, const SandCell &b) {
      int keyA = static_cast<int>(a.dist) * 4 - iabs(a.dx);
      int keyB = static_cast<int>(b.dist) * 4 - iabs(b.dx);
      if (keyA != keyB) return keyA < keyB;
      if (iabs(a.dx) != iabs(b.dx)) return iabs(a.dx) > iabs(b.dx);
      return a.dx < b.dx;
    });

    // Bottom bulb: smaller key fills first (bottom center first, growing into a cone).
    std::sort(_bottomOrder[hg], _bottomOrder[hg] + count, [](const SandCell &a, const SandCell &b) {
      int keyA = (BULB_HEIGHT - static_cast<int>(a.dist)) * 2 + iabs(a.dx);
      int keyB = (BULB_HEIGHT - static_cast<int>(b.dist)) * 2 + iabs(b.dx);
      if (keyA != keyB) return keyA < keyB;
      if (a.dist != b.dist) return a.dist > b.dist;
      return a.dx < b.dx;
    });
  }

  _tablesReady = true;
}

void HourglassClockface::setup(CWDateTime *dateTime)
{
  _dateTime = dateTime;
  _lastFrameMillis = 0;
  _secStartMillis = millis();
  _lastSecond = -1;
  _lastHour12 = -1;
  _lastMinute = -1;
  _lastIsAM = true;
  _animTick = 0;

  initTables();

  if (!_display) return;
  _display->fillRect(0, 0, MATRIX_SIZE, MATRIX_SIZE, COLOR_BLACK);

  for (uint8_t i = 0; i < HOURGLASS_COUNT; ++i) {
    drawLabel(HOURGLASS_LABEL[i], HOURGLASS_CX[i], 2, PALETTES[i].label);
    drawHourglassFrame(i, HOURGLASS_CX[i]);
  }

  if (_dateTime) {
    TimeSnapshot snap = _dateTime->snapshot();
    drawTime(snap.hour, snap.minute, snap.isAM, true);
  }
  update();
}

void HourglassClockface::teardown()
{
  _lastSecond = -1;
  _lastHour12 = -1;
  _lastMinute = -1;
  if (_display) {
    _display->fillRect(0, 0, MATRIX_SIZE, MATRIX_SIZE, COLOR_BLACK);
  }
}

void HourglassClockface::drawLabel(char ch, int16_t cx, int16_t y, uint16_t color)
{
  // 3x5 pixel-art letters for 'D', 'H', 'M'
  static const uint8_t GLYPH_D[5] = { 0x6, 0x5, 0x5, 0x5, 0x6 };
  static const uint8_t GLYPH_H[5] = { 0x5, 0x5, 0x7, 0x5, 0x5 };
  static const uint8_t GLYPH_M[5] = { 0x5, 0x7, 0x5, 0x5, 0x5 };

  const uint8_t *rows = GLYPH_D;
  if (ch == 'H') rows = GLYPH_H;
  else if (ch == 'M') rows = GLYPH_M;

  int16_t x0 = cx - 1;
  for (uint8_t r = 0; r < 5; ++r) {
    for (uint8_t c = 0; c < 3; ++c) {
      if (rows[r] & (0x4 >> c)) {
        _display->drawPixel(x0 + c, y + r, color);
      }
    }
  }
}

void HourglassClockface::drawHourglassFrame(uint8_t idx, int16_t cx)
{
  int16_t capHalf = CAP_HALF_WIDTH[idx];
  int16_t capWidth = capHalf * 2 + 1;

  // Top wooden/brass cap (2px tall)
  _display->drawFastHLine(cx - capHalf, GLASS_CY - BULB_HEIGHT - 2, capWidth, COLOR_CAP_LIGHT);
  _display->drawFastHLine(cx - capHalf, GLASS_CY - BULB_HEIGHT - 1, capWidth, COLOR_CAP_DARK);

  // Bottom wooden/brass cap (2px tall)
  _display->drawFastHLine(cx - capHalf, GLASS_CY + BULB_HEIGHT + 1, capWidth, COLOR_CAP_DARK);
  _display->drawFastHLine(cx - capHalf, GLASS_CY + BULB_HEIGHT + 2, capWidth, COLOR_CAP_LIGHT);

  // Neck glass walls
  _display->drawPixel(cx - 1, GLASS_CY, COLOR_GLASS);
  _display->drawPixel(cx + 1, GLASS_CY, COLOR_GLASS);

  // Top and bottom bulb glass walls
  for (int16_t dist = 1; dist <= BULB_HEIGHT; ++dist) {
    int16_t wall = HALF_WIDTH[idx][dist] + 1;
    uint16_t leftTopColor = (dist >= 13 && dist <= 18) ? COLOR_GLASS_SHINE : COLOR_GLASS;
    _display->drawPixel(cx - wall, GLASS_CY - dist, leftTopColor);
    _display->drawPixel(cx + wall, GLASS_CY - dist, COLOR_GLASS);
    _display->drawPixel(cx - wall, GLASS_CY + dist, COLOR_GLASS);
    _display->drawPixel(cx + wall, GLASS_CY + dist, COLOR_GLASS);
  }
}

void HourglassClockface::drawHourglassSand(uint8_t idx, int16_t cx, float progress)
{
  if (progress < 0.0f) progress = 0.0f;
  if (progress > 1.0f) progress = 1.0f;

  int totalPixels = _bulbPixels[idx];
  int bottomCount = static_cast<int>(roundf(progress * totalPixels));
  if (bottomCount < 0) bottomCount = 0;
  if (bottomCount > totalPixels) bottomCount = totalPixels;
  int topCount = totalPixels - bottomCount;

  // Draw top bulb (remaining sand)
  for (int i = 0; i < totalPixels; ++i) {
    int16_t x = cx + _topOrder[idx][i].dx;
    int16_t y = GLASS_CY - static_cast<int16_t>(_topOrder[idx][i].dist);
    uint16_t col = (i < topCount) ? grainColor(idx, x, y) : COLOR_BLACK;
    _display->drawPixel(x, y, col);
  }

  // Draw bottom bulb (accumulated sand pile) and track center pile top for stream
  int16_t pileTopY = GLASS_CY + BULB_HEIGHT + 1;
  for (int i = 0; i < totalPixels; ++i) {
    int16_t x = cx + _bottomOrder[idx][i].dx;
    int16_t y = GLASS_CY + static_cast<int16_t>(_bottomOrder[idx][i].dist);
    if (i < bottomCount) {
      _display->drawPixel(x, y, grainColor(idx, x, y));
      if (_bottomOrder[idx][i].dx == 0 && y < pileTopY) {
        pileTopY = y;
      }
    } else {
      _display->drawPixel(x, y, COLOR_BLACK);
    }
  }

  // Falling sand stream through the neck down to the top of the bottom pile
  if (topCount > 0) {
    uint8_t stride = PALETTES[idx].streamStride;
    uint32_t phase = _animTick / PALETTES[idx].streamDivider;
    _display->drawPixel(cx, GLASS_CY, PALETTES[idx].light);
    for (int16_t y = GLASS_CY + 1; y < pileTopY; ++y) {
      if (((static_cast<uint32_t>(y) + stride * 100U - (phase % stride)) % stride) == 0) {
        _display->drawPixel(cx, y, PALETTES[idx].light);
      }
    }
  } else {
    _display->drawPixel(cx, GLASS_CY, COLOR_BLACK);
  }
}

void HourglassClockface::drawTime(int hour24, int minute, bool isAM, bool force)
{
  int h12 = hour24 % 12;
  if (h12 == 0) h12 = 12;

  if (!force && h12 == _lastHour12 && minute == _lastMinute && isAM == _lastIsAM) {
    return;
  }
  _lastHour12 = h12;
  _lastMinute = minute;
  _lastIsAM = isAM;

  // Clear lower-left time area
  _display->fillRect(0, 55, 34, 9, COLOR_BLACK);

  int16_t x = 2;
  const int16_t y = 57;
  if (h12 >= 10) {
    drawCompactClockDigit(_display, h12 / 10, x, y, COLOR_TIME, 1);
    x += 4;
  }
  drawCompactClockDigit(_display, h12 % 10, x, y, COLOR_TIME, 1);
  x += 4;
  _display->drawPixel(x, y + 1, COLOR_TIME);
  _display->drawPixel(x, y + 3, COLOR_TIME);
  x += 2;
  drawCompactClockDigit(_display, minute / 10, x, y, COLOR_TIME, 1);
  x += 4;
  drawCompactClockDigit(_display, minute % 10, x, y, COLOR_TIME, 1);
  x += 5;
  drawAmPm(_display, isAM, x, y, COLOR_AMPM);
}

void HourglassClockface::update()
{
  if (!_display || !_dateTime) return;

  unsigned long now = millis();
  if (_lastFrameMillis != 0 && (now - _lastFrameMillis) < FRAME_INTERVAL_MS) {
    return;
  }
  _lastFrameMillis = now;
  ++_animTick;

  TimeSnapshot snap = _dateTime->snapshot();
  int hour   = snap.hour;
  int minute = snap.minute;
  int second = snap.second;
  bool isAM  = snap.isAM;

  if (second != _lastSecond) {
    _lastSecond = second;
    _secStartMillis = now;
  }

  unsigned long elapsedMs = now - _secStartMillis;
  if (elapsedMs > 999UL) elapsedMs = 999UL;
  float subSec = static_cast<float>(elapsedMs) / 1000.0f;

  float dayProgress  = (static_cast<float>(hour) * 3600.0f +
                        static_cast<float>(minute) * 60.0f +
                        static_cast<float>(second)) / 86400.0f;
  float hourProgress = (static_cast<float>(minute) * 60.0f +
                        static_cast<float>(second) + subSec) / 3600.0f;
  float minProgress  = (static_cast<float>(second) + subSec) / 60.0f;

  drawHourglassSand(0, HOURGLASS_CX[0], dayProgress);
  drawHourglassSand(1, HOURGLASS_CX[1], hourProgress);
  drawHourglassSand(2, HOURGLASS_CX[2], minProgress);

  drawTime(hour, minute, isAM, false);
}
