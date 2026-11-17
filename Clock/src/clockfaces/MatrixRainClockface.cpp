#include "MatrixRainClockface.h"

namespace {
const uint16_t COLOR_HEAD   = 0xE7FC;  // Pale bright mint-white
const uint16_t COLOR_GREEN1 = 0x07E0;  // Bright Matrix green
const uint16_t COLOR_GREEN2 = 0x0540;
const uint16_t COLOR_GREEN3 = 0x0320;
const uint16_t COLOR_GREEN4 = 0x0160;
}  // namespace

MatrixRainClockface::MatrixRainClockface(Adafruit_GFX *display)
  : _display(display)
{
}

void MatrixRainClockface::initDrop(int idx, bool randomStart)
{
  _drops[idx].x = random(0, 32) * 2;  // 32 possible 2px-aligned columns
  _drops[idx].speed = random(1, 5);   // 4 distinct speeds
  _drops[idx].length = random(6, 23);
  _drops[idx].tick = random(0, 4);
  _drops[idx].headY = randomStart ? random(-30, 64) : random(-32, -1);
}

void MatrixRainClockface::setup(CWDateTime *dateTime)
{
  (void)dateTime;
  randomSeed(esp_random());

  for (int i = 0; i < NUM_DROPS; i++) {
    initDrop(i, true);
  }

  if (_display) {
    _display->fillRect(0, 0, 64, 64, 0x0000);
  }
  _lastFrameMillis = millis();
}

void MatrixRainClockface::teardown()
{
  if (_display) {
    _display->fillRect(0, 0, 64, 64, 0x0000);
  }
}

void MatrixRainClockface::update()
{
  if (!_display) return;
  unsigned long now = millis();

  if (now - _lastFrameMillis < FRAME_MS) {
    return;
  }
  _lastFrameMillis = now;

  for (int i = 0; i < NUM_DROPS; i++) {
    Drop &d = _drops[i];
    d.tick++;
    if (d.tick < static_cast<uint8_t>(5 - d.speed)) {
      continue;
    }
    d.tick = 0;

    int16_t px = d.x;
    int16_t tailY = d.headY - d.length;

    if (tailY >= 0 && tailY < 64) {
      _display->fillRect(px, tailY, 2, 1, 0x0000);
    }

    if (d.headY >= 0 && d.headY < 64) {
      _display->drawPixel(px, d.headY, (random(4) == 0) ? COLOR_GREEN2 : COLOR_GREEN1);
      if (random(3) != 0) {
        _display->drawPixel(px + 1, d.headY, COLOR_GREEN2);
      } else {
        _display->drawPixel(px + 1, d.headY, 0x0000);
      }
    }

    // Fade and occasionally flicker/mutate trail glyphs
    for (int k = 2; k < d.length; k += 2) {
      int16_t fy = d.headY - k;
      if (fy >= 0 && fy < 64) {
        uint16_t fadeCol = (k < d.length / 3)
            ? COLOR_GREEN1
            : (k < (d.length * 2) / 3 ? COLOR_GREEN3 : COLOR_GREEN4);
        if (random(6) == 0) {
          _display->drawPixel(px + random(2), fy, (random(3) == 0) ? 0x0000 : fadeCol);
        } else {
          _display->drawPixel(px, fy, fadeCol);
        }
      }
    }

    d.headY++;

    if (d.headY >= 0 && d.headY < 64) {
      _display->fillRect(px, d.headY, 2, 1, COLOR_HEAD);
    }

    if (d.headY - d.length > 64) {
      initDrop(i, false);
    }
  }
}
