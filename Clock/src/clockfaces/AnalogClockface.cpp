#include "AnalogClockface.h"
#include "../fonts/picopixel.h"
#include <math.h>

namespace {
const uint16_t COLOR_BLACK  = 0x0000;
const uint16_t COLOR_BEZEL  = 0x3186;
const uint16_t COLOR_TICK   = 0x7BEF;
const uint16_t COLOR_CARD   = 0xFFFF;
const uint16_t COLOR_HOUR   = 0x04E0;  // Medium green
const uint16_t COLOR_MINUTE = 0x05FF;  // Sky blue
const uint16_t COLOR_SECOND = 0xFC00;  // Bright orange
const float DEG_TO_RAD_F    = 0.0174532925f;
}  // namespace

AnalogClockface::AnalogClockface(Adafruit_GFX *display)
  : _display(display)
{
}

void AnalogClockface::setup(CWDateTime *dateTime)
{
  _dateTime = dateTime;
  _lastSecond = -1;
  _hasPrevHands = false;

  if (!_display) return;
  _display->fillRect(0, 0, 64, 64, COLOR_BLACK);
  drawDial();
  update();
}

void AnalogClockface::teardown()
{
  _hasPrevHands = false;
  _lastSecond = -1;
  if (_display) {
    _display->fillRect(0, 0, 64, 64, COLOR_BLACK);
  }
}

void AnalogClockface::drawDial()
{
  if (!_display) return;

  _display->setFont(&Picopixel);
  _display->setTextSize(1);

  for (int i = 0; i < 12; i++) {
    int hourNum = (i == 0) ? 12 : i;
    char buf[3];
    snprintf(buf, sizeof(buf), "%d", hourNum);

    float rad = (i * 30.0f - 90.0f) * DEG_TO_RAD_F;
    int16_t tx = CX + static_cast<int16_t>(roundf(cosf(rad) * NUM_RADIUS));
    int16_t ty = CY + static_cast<int16_t>(roundf(sinf(rad) * NUM_RADIUS));

    int16_t x1, y1;
    uint16_t w, h;
    _display->getTextBounds(buf, 0, 0, &x1, &y1, &w, &h);
    _display->setTextColor(COLOR_TICK);
    _display->setCursor(tx - (w / 2) - x1, ty - (h / 2) - y1);
    _display->print(buf);
  }
}

void AnalogClockface::drawThickHand(int16_t x0, int16_t y0, int16_t x1, int16_t y1, uint16_t color)
{
  _display->drawLine(x0, y0, x1, y1, color);
  // 1px perpendicular offset for a bolder hour hand
  if (abs(x1 - x0) >= abs(y1 - y0)) {
    _display->drawLine(x0, y0 + 1, x1, y1 + 1, color);
  } else {
    _display->drawLine(x0 + 1, y0, x1 + 1, y1, color);
  }
}

void AnalogClockface::erasePrevHands()
{
  if (!_hasPrevHands || !_display) return;
  drawThickHand(CX, CY, _prevHourX, _prevHourY, COLOR_BLACK);
  _display->drawLine(CX, CY, _prevMinX, _prevMinY, COLOR_BLACK);
  _display->drawLine(_prevTailX, _prevTailY, _prevSecX, _prevSecY, COLOR_BLACK);
}

void AnalogClockface::drawHands(int hour, int minute, int second)
{
  if (!_display) return;

  float hourRad = (((hour % 12) + (minute / 60.0f)) * 30.0f - 90.0f) * DEG_TO_RAD_F;
  float minRad  = ((minute + (second / 60.0f)) * 6.0f - 90.0f) * DEG_TO_RAD_F;
  float secRad  = (second * 6.0f - 90.0f) * DEG_TO_RAD_F;

  int16_t hx = CX + static_cast<int16_t>(roundf(cosf(hourRad) * HOUR_LEN));
  int16_t hy = CY + static_cast<int16_t>(roundf(sinf(hourRad) * HOUR_LEN));

  int16_t mx = CX + static_cast<int16_t>(roundf(cosf(minRad) * MIN_LEN));
  int16_t my = CY + static_cast<int16_t>(roundf(sinf(minRad) * MIN_LEN));

  int16_t sx = CX + static_cast<int16_t>(roundf(cosf(secRad) * SEC_LEN));
  int16_t sy = CY + static_cast<int16_t>(roundf(sinf(secRad) * SEC_LEN));
  int16_t tx = CX - static_cast<int16_t>(roundf(cosf(secRad) * SEC_TAIL));
  int16_t ty = CY - static_cast<int16_t>(roundf(sinf(secRad) * SEC_TAIL));

  drawThickHand(CX, CY, hx, hy, COLOR_HOUR);
  _display->drawLine(CX, CY, mx, my, COLOR_MINUTE);
  _display->drawLine(tx, ty, sx, sy, COLOR_SECOND);

  // Center pivot cap
  _display->fillCircle(CX, CY, 2, COLOR_TICK);
  _display->drawPixel(CX, CY, COLOR_SECOND);

  _prevHourX = hx;
  _prevHourY = hy;
  _prevMinX  = mx;
  _prevMinY  = my;
  _prevSecX  = sx;
  _prevSecY  = sy;
  _prevTailX = tx;
  _prevTailY = ty;
  _hasPrevHands = true;
}

void AnalogClockface::update()
{
  if (!_display || !_dateTime) return;

  TimeSnapshot snap = _dateTime->snapshot();
  if (snap.second == _lastSecond) {
    return;
  }
  _lastSecond = snap.second;

  erasePrevHands();
  drawDial();
  drawHands(snap.hour, snap.minute, snap.second);
}
