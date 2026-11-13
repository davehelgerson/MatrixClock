#include "StarsClockface.h"
#include <math.h>

namespace {
const int16_t MATRIX_SIZE = 64;
const uint16_t COLOR_BLACK = 0x0000;
const float PI_F = 3.14159265f;
}

StarsClockface::StarsClockface(Adafruit_GFX *display)
  : _display(display)
{
}

void StarsClockface::setup(CWDateTime *dateTime)
{
  (void)dateTime;
  randomSeed(esp_random());

  for (uint8_t i = 0; i < MAX_STARS; ++i) {
    _stars[i].active = false;
    _stars[i].hasPrev = false;
    _stars[i].hasTail = false;
  }

  if (_display) {
    _display->fillRect(0, 0, MATRIX_SIZE, MATRIX_SIZE, COLOR_BLACK);
  }

  // Seed several stars right away so motion starts immediately
  for (uint8_t i = 0; i < 12; ++i) {
    spawnStar(_stars[i]);
    float advance = static_cast<float>(random(0, 35));
    _stars[i].x += _stars[i].vx * advance;
    _stars[i].y += _stars[i].vy * advance;
  }

  _lastFrameMillis = 0;
  _lastSpawnMillis = millis();
}

void StarsClockface::update()
{
  if (!_display) {
    return;
  }

  unsigned long now = millis();
  if ((now - _lastFrameMillis) < FRAME_INTERVAL_MS) {
    return;
  }
  _lastFrameMillis = now;

  // Erase previous pixels first so overlapping stars don't erase each other
  for (uint8_t i = 0; i < MAX_STARS; ++i) {
    Star &s = _stars[i];
    if (!s.active) {
      continue;
    }
    if (s.hasPrev) {
      _display->drawPixel(s.prevX, s.prevY, COLOR_BLACK);
    }
    if (s.hasTail) {
      _display->drawPixel(s.tailX, s.tailY, COLOR_BLACK);
    }
  }

  // Periodically spawn a new star into a free slot
  if ((now - _lastSpawnMillis) >= SPAWN_INTERVAL_MS) {
    _lastSpawnMillis = now;
    for (uint8_t i = 0; i < MAX_STARS; ++i) {
      if (!_stars[i].active) {
        spawnStar(_stars[i]);
        break;
      }
    }
  }

  // Advance and draw all active stars
  for (uint8_t i = 0; i < MAX_STARS; ++i) {
    Star &s = _stars[i];
    if (!s.active) {
      continue;
    }

    int16_t oldX = static_cast<int16_t>(roundf(s.x));
    int16_t oldY = static_cast<int16_t>(roundf(s.y));

    s.x += s.vx;
    s.y += s.vy;

    int16_t nx = static_cast<int16_t>(roundf(s.x));
    int16_t ny = static_cast<int16_t>(roundf(s.y));

    if (nx < 0 || nx >= MATRIX_SIZE || ny < 0 || ny >= MATRIX_SIZE) {
      s.active = false;
      s.hasPrev = false;
      s.hasTail = false;
      continue;
    }

    if (s.hasPrev && (oldX != nx || oldY != ny) &&
        oldX >= 0 && oldX < MATRIX_SIZE && oldY >= 0 && oldY < MATRIX_SIZE) {
      _display->drawPixel(oldX, oldY, s.tailColor);
      s.tailX = static_cast<int8_t>(oldX);
      s.tailY = static_cast<int8_t>(oldY);
      s.hasTail = true;
    } else {
      s.hasTail = false;
    }

    _display->drawPixel(nx, ny, s.color);
    s.prevX = static_cast<int8_t>(nx);
    s.prevY = static_cast<int8_t>(ny);
    s.hasPrev = true;
  }
}

void StarsClockface::teardown()
{
  for (uint8_t i = 0; i < MAX_STARS; ++i) {
    _stars[i].active = false;
    _stars[i].hasPrev = false;
    _stars[i].hasTail = false;
  }
  if (_display) {
    _display->fillRect(0, 0, MATRIX_SIZE, MATRIX_SIZE, COLOR_BLACK);
  }
}

void StarsClockface::spawnStar(Star &star)
{
  uint8_t edge = static_cast<uint8_t>(random(0, 4));
  float angle = 0.0f;
  float spread = (static_cast<float>(random(-65, 66)) * PI_F) / 180.0f;

  switch (edge) {
    case 0: // Left edge -> dart rightwards
      star.x = 0.0f;
      star.y = static_cast<float>(random(0, MATRIX_SIZE));
      angle = spread;
      break;
    case 1: // Right edge -> dart leftwards
      star.x = static_cast<float>(MATRIX_SIZE - 1);
      star.y = static_cast<float>(random(0, MATRIX_SIZE));
      angle = PI_F + spread;
      break;
    case 2: // Top edge -> dart downwards
      star.x = static_cast<float>(random(0, MATRIX_SIZE));
      star.y = 0.0f;
      angle = (PI_F * 0.5f) + spread;
      break;
    case 3: // Bottom edge -> dart upwards
    default:
      star.x = static_cast<float>(random(0, MATRIX_SIZE));
      star.y = static_cast<float>(MATRIX_SIZE - 1);
      angle = (PI_F * 1.5f) + spread;
      break;
  }

  float speed = 0.35f + static_cast<float>(random(0, 145)) / 100.0f; // 0.35 .. 1.80 px/frame
  star.vx = cosf(angle) * speed;
  star.vy = sinf(angle) * speed;

  star.color = randomStarColor();
  star.tailColor = dimRgb565(star.color);
  star.hasPrev = false;
  star.hasTail = false;
  star.active = true;
}

uint16_t StarsClockface::randomStarColor()
{
  // Vivid saturated random colors in RGB565 (avoid very dark colors)
  uint8_t r = 0, g = 0, b = 0;
  uint8_t hueBucket = static_cast<uint8_t>(random(0, 7));
  switch (hueBucket) {
    case 0: r = random(180, 256); g = random(20, 120);  b = random(20, 120);  break; // Red / coral
    case 1: r = random(20, 120);  g = random(180, 256); b = random(20, 120);  break; // Green
    case 2: r = random(40, 130);  g = random(90, 190);  b = random(200, 256); break; // Blue
    case 3: r = random(200, 256); g = random(160, 256); b = random(20, 90);   break; // Yellow / gold
    case 4: r = random(20, 90);   g = random(190, 256); b = random(200, 256); break; // Cyan
    case 5: r = random(190, 256); g = random(20, 110);  b = random(190, 256); break; // Magenta / pink
    default:
      r = random(210, 256); g = random(210, 256); b = random(210, 256); break;      // White star
  }

  return ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3);
}

uint16_t StarsClockface::dimRgb565(uint16_t color)
{
  uint16_t r = ((color >> 11) & 0x1F) >> 2;
  uint16_t g = ((color >> 5) & 0x3F) >> 2;
  uint16_t b = (color & 0x1F) >> 2;
  return (r << 11) | (g << 5) | b;
}

