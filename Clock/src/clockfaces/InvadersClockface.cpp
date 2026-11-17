#include "InvadersClockface.h"
#include "ClockfaceCommon.h"
#include "../fonts/picopixel.h"

namespace {
const uint16_t COLOR_BLACK   = 0x0000;
const uint16_t COLOR_SHIP    = 0x07E0;
const uint16_t COLOR_BUNKER  = 0x0540;
const uint16_t COLOR_LASER   = 0xFFE0;
const uint16_t COLOR_BOMB    = 0xF800;
const uint16_t COLOR_UFO     = 0xF800;
const uint16_t COLOR_BOOM    = 0xFD20;
const uint16_t COLOR_DIVIDER = 0x2124;

// 7x5 classic Space Invader bitmaps (2 frames x 3 alien types)
const uint8_t ALIEN_SPRITES[3][2][5] = {
  { // Row 0 (Top Squid - Magenta)
    { 0b0011100, 0b0111110, 0b1101011, 0b1111111, 0b0100010 },
    { 0b0011100, 0b0111110, 0b1101011, 0b1111111, 0b1001001 }
  },
  { // Row 1 (Crab - Cyan)
    { 0b0100010, 0b0011100, 0b0101010, 0b1111111, 0b1010101 },
    { 0b0100010, 0b1011101, 0b1101011, 0b1111111, 0b0100010 }
  },
  { // Row 2 (Octopus - Lime/Yellow-Green)
    { 0b0011100, 0b1111111, 0b1011101, 0b0111110, 0b1100011 },
    { 0b0011100, 0b1111111, 0b1011101, 0b0111110, 0b0110110 }
  }
};

const uint16_t ALIEN_COLORS[3] = { 0xF81F, 0x07FF, 0x87E0 };

const uint8_t SHIP_SPRITE[4] = {
  0b0001000,
  0b0011100,
  0b0111110,
  0b1111111
};

const uint8_t UFO_SPRITE[3] = {
  0b0011100,
  0b1101011,
  0b0111110
};

const char *const MONTH_NAMES[12] = {
  "JAN", "FEB", "MAR", "APR", "MAY", "JUN",
  "JUL", "AUG", "SEP", "OCT", "NOV", "DEC"
};
}  // namespace

InvadersClockface::InvadersClockface(Adafruit_GFX *display)
  : _display(display)
{
}

void InvadersClockface::setup(CWDateTime *dateTime)
{
  _dateTime = dateTime;
  randomSeed(esp_random());

  if (_display) {
    _display->fillRect(0, 0, 64, 64, COLOR_BLACK);
    _display->drawFastHLine(0, 49, 64, COLOR_DIVIDER);
  }

  _shipX = 28;
  _shipDir = 1;
  _shipHit = false;
  _laserActive = false;
  _boomActive = false;
  _ufoActive = false;
  _lastMinute = -1;
  for (int i = 0; i < MAX_BOMBS; i++) {
    _bombs[i].active = false;
  }

  resetBunkers();
  resetWave();
  drawShip(_shipX, COLOR_SHIP);
  updateClockBar();

  unsigned long now = millis();
  _lastAlienMillis = now;
  _lastShipMillis = now;
  _lastLaserMillis = now;
  _lastBombMillis = now;
  _lastUfoMillis = now;
  _lastClockMillis = now;
  _nextUfoMillis = now + random(8000, 14000);
}

void InvadersClockface::teardown()
{
  if (_display) {
    _display->fillRect(0, 0, 64, 64, COLOR_BLACK);
  }
}

void InvadersClockface::resetBunkers()
{
  for (int b = 0; b < BUNKER_COUNT; b++) {
    _bunkers[b][0] = 0b011110;
    _bunkers[b][1] = 0b111111;
    _bunkers[b][2] = 0b110011;
  }
  drawBunkers();
}

void InvadersClockface::drawBunkers()
{
  if (!_display) return;
  for (int b = 0; b < BUNKER_COUNT; b++) {
    for (int r = 0; r < BUNKER_H; r++) {
      for (int c = 0; c < BUNKER_W; c++) {
        uint16_t col = (_bunkers[b][r] & (1 << (BUNKER_W - 1 - c))) ? COLOR_BUNKER : COLOR_BLACK;
        _display->drawPixel(_bunkerX[b] + c, BUNKER_Y + r, col);
      }
    }
  }
}

bool InvadersClockface::hitBunker(int16_t x, int16_t y)
{
  if (y < BUNKER_Y || y >= BUNKER_Y + BUNKER_H) return false;
  int r = y - BUNKER_Y;
  for (int b = 0; b < BUNKER_COUNT; b++) {
    if (x >= _bunkerX[b] && x < _bunkerX[b] + BUNKER_W) {
      int c = x - _bunkerX[b];
      uint8_t mask = (1 << (BUNKER_W - 1 - c));
      if (_bunkers[b][r] & mask) {
        _bunkers[b][r] &= ~mask;
        if (_display) {
          _display->drawPixel(x, y, COLOR_BLACK);
        }
        return true;
      }
    }
  }
  return false;
}

void InvadersClockface::resetWave()
{
  if (_display) {
    _display->fillRect(0, 4, 64, 32, COLOR_BLACK);
  }
  _fleetX = 4;
  _fleetY = 4;
  _fleetDir = 1;
  _alienFrame = false;
  _aliveCount = ALIEN_ROWS * ALIEN_COLS;

  for (int r = 0; r < ALIEN_ROWS; r++) {
    for (int c = 0; c < ALIEN_COLS; c++) {
      _alive[r][c] = true;
      drawAlien(_fleetX + c * 10, _fleetY + r * 7, r, _alienFrame, ALIEN_COLORS[r]);
    }
  }
  resetBunkers();
}

void InvadersClockface::drawAlien(int16_t x, int16_t y, int row, bool frame, uint16_t color)
{
  if (!_display) return;
  const uint8_t *rows = ALIEN_SPRITES[row % 3][frame ? 1 : 0];
  for (int r = 0; r < ALIEN_H; r++) {
    for (int c = 0; c < ALIEN_W; c++) {
      if (rows[r] & (1 << (ALIEN_W - 1 - c))) {
        _display->drawPixel(x + c, y + r, color);
      }
    }
  }
}

void InvadersClockface::drawShip(int16_t x, uint16_t color)
{
  if (!_display) return;
  for (int r = 0; r < SHIP_H; r++) {
    for (int c = 0; c < SHIP_W; c++) {
      if (SHIP_SPRITE[r] & (1 << (SHIP_W - 1 - c))) {
        _display->drawPixel(x + c, SHIP_Y + r, color);
      }
    }
  }
}

void InvadersClockface::drawUfo(int16_t x, uint16_t color)
{
  if (!_display) return;
  for (int r = 0; r < 3; r++) {
    for (int c = 0; c < 7; c++) {
      if (UFO_SPRITE[r] & (1 << (6 - c))) {
        int16_t px = x + c;
        if (px >= 0 && px < 64) {
          _display->drawPixel(px, r, color);
        }
      }
    }
  }
}

void InvadersClockface::triggerBoom(int16_t cx, int16_t cy, uint16_t color)
{
  if (!_display) return;
  _boomActive = true;
  _boomX = cx;
  _boomY = cy;
  _boomStart = millis();
  _display->drawPixel(cx - 2, cy - 2, color);
  _display->drawPixel(cx + 2, cy - 2, color);
  _display->drawPixel(cx,     cy,     0xFFFF);
  _display->drawPixel(cx - 2, cy + 2, color);
  _display->drawPixel(cx + 2, cy + 2, color);
}

void InvadersClockface::updateAliens()
{
  for (int r = 0; r < ALIEN_ROWS; r++) {
    for (int c = 0; c < ALIEN_COLS; c++) {
      if (_alive[r][c]) {
        drawAlien(_fleetX + c * 10, _fleetY + r * 7, r, _alienFrame, COLOR_BLACK);
      }
    }
  }

  int minCol = ALIEN_COLS, maxCol = -1, maxRow = -1;
  _aliveCount = 0;
  for (int r = 0; r < ALIEN_ROWS; r++) {
    for (int c = 0; c < ALIEN_COLS; c++) {
      if (_alive[r][c]) {
        _aliveCount++;
        if (c < minCol) minCol = c;
        if (c > maxCol) maxCol = c;
        if (r > maxRow) maxRow = r;
      }
    }
  }

  if (_aliveCount == 0) {
    resetWave();
    return;
  }

  int16_t nextX = _fleetX + _fleetDir;
  int16_t leftEdge = nextX + minCol * 10;
  int16_t rightEdge = nextX + maxCol * 10 + ALIEN_W;

  if (leftEdge < 2 || rightEdge > 62) {
    _fleetDir = -_fleetDir;
    if (_fleetY + (maxRow * 7) + ALIEN_H < BUNKER_Y - 2) {
      _fleetY += 2;
    } else {
      // Aliens reached bunker line -> reset wave
      resetWave();
      return;
    }
  } else {
    _fleetX = nextX;
  }

  _alienFrame = !_alienFrame;

  for (int r = 0; r < ALIEN_ROWS; r++) {
    for (int c = 0; c < ALIEN_COLS; c++) {
      if (_alive[r][c]) {
        drawAlien(_fleetX + c * 10, _fleetY + r * 7, r, _alienFrame, ALIEN_COLORS[r]);
      }
    }
  }

  // Bottom-most alive aliens occasionally drop bombs
  if (random(100) < 35) {
    int freeSlot = -1;
    for (int i = 0; i < MAX_BOMBS; i++) {
      if (!_bombs[i].active) { freeSlot = i; break; }
    }
    if (freeSlot >= 0) {
      int shooterCols[ALIEN_COLS];
      int shooterRows[ALIEN_COLS];
      int count = 0;
      for (int c = 0; c < ALIEN_COLS; c++) {
        for (int r = ALIEN_ROWS - 1; r >= 0; r--) {
          if (_alive[r][c]) {
            shooterCols[count] = c;
            shooterRows[count] = r;
            count++;
            break;
          }
        }
      }
      if (count > 0) {
        int pick = random(count);
        _bombs[freeSlot].active = true;
        _bombs[freeSlot].x = _fleetX + shooterCols[pick] * 10 + 3;
        _bombs[freeSlot].y = _fleetY + shooterRows[pick] * 7 + ALIEN_H;
      }
    }
  }
}

void InvadersClockface::updateShip()
{
  if (_shipHit) {
    if (millis() >= _shipRespawnAt) {
      _shipHit = false;
      _shipX = 28;
      drawShip(_shipX, COLOR_SHIP);
    }
    return;
  }

  drawShip(_shipX, COLOR_BLACK);

  // 1) Dodge any incoming bomb near ship level, and hold steady just outside its path until it passes
  int8_t desiredDir = 0;
  bool holdingForBomb = false;
  int16_t shipCenter = _shipX + 3;
  for (int i = 0; i < MAX_BOMBS; i++) {
    if (_bombs[i].active && _bombs[i].y >= 24) {
      int16_t dx = abs(_bombs[i].x - shipCenter);
      if (dx <= 5) {
        if (_bombs[i].x >= shipCenter) {
          desiredDir = (_shipX > 2) ? -1 : 1;
        } else {
          desiredDir = (_shipX < 64 - SHIP_W - 2) ? 1 : -1;
        }
        break;
      } else if (dx <= 7) {
        holdingForBomb = true;
      }
    }
  }

  // 2) Otherwise aim smoothly toward the nearest alive alien column
  if (desiredDir == 0 && !holdingForBomb) {
    int16_t bestDist = 999;
    int16_t targetX = shipCenter;
    for (int c = 0; c < ALIEN_COLS; c++) {
      for (int r = 0; r < ALIEN_ROWS; r++) {
        if (_alive[r][c]) {
          int16_t ax = _fleetX + c * 10 + 3;
          int16_t dist = abs(ax - shipCenter);
          if (dist < bestDist) {
            bestDist = dist;
            targetX = ax;
          }
          break;
        }
      }
    }
    if (abs(targetX - shipCenter) > 1) {
      desiredDir = (targetX > shipCenter) ? 1 : -1;
    }
  }

  if (desiredDir != 0) {
    _shipDir = desiredDir;
    _shipX += desiredDir;
  }

  if (_shipX <= 2) {
    _shipX = 2;
    _shipDir = 1;
  } else if (_shipX >= 64 - SHIP_W - 2) {
    _shipX = 64 - SHIP_W - 2;
    _shipDir = -1;
  }

  drawShip(_shipX, COLOR_SHIP);

  if (!_laserActive && random(100) < 35) {
    _laserActive = true;
    _laserX = _shipX + 3;
    _laserY = SHIP_Y - 3;
  }
}

void InvadersClockface::updateLaser()
{
  if (!_laserActive) return;

  _display->drawFastVLine(_laserX, _laserY, 3, COLOR_BLACK);
  _laserY -= 2;

  if (_laserY < 0) {
    _laserActive = false;
    return;
  }

  if (hitBunker(_laserX, _laserY) || hitBunker(_laserX, _laserY + 1)) {
    _laserActive = false;
    return;
  }

  if (_ufoActive && _laserY <= 2 && _laserX >= _ufoX && _laserX < _ufoX + 7) {
    _ufoActive = false;
    _laserActive = false;
    drawUfo(_ufoX, COLOR_BLACK);
    triggerBoom(_ufoX + 3, 1, 0xFFE0);
    _nextUfoMillis = millis() + random(10000, 16000);
    return;
  }

  for (int r = ALIEN_ROWS - 1; r >= 0; r--) {
    for (int c = 0; c < ALIEN_COLS; c++) {
      if (!_alive[r][c]) continue;
      int16_t ax = _fleetX + c * 10;
      int16_t ay = _fleetY + r * 7;
      if (_laserX >= ax && _laserX < ax + ALIEN_W &&
          _laserY <= ay + ALIEN_H && (_laserY + 2) >= ay) {
        _alive[r][c] = false;
        _laserActive = false;
        drawAlien(ax, ay, r, _alienFrame, COLOR_BLACK);
        triggerBoom(ax + 3, ay + 2, COLOR_BOOM);
        return;
      }
    }
  }

  _display->drawFastVLine(_laserX, _laserY, 3, COLOR_LASER);
}

void InvadersClockface::updateBombs()
{
  for (int i = 0; i < MAX_BOMBS; i++) {
    Bomb &b = _bombs[i];
    if (!b.active) continue;

    _display->drawFastVLine(b.x, b.y, 2, COLOR_BLACK);
    b.y += 1;

    if (b.y >= 47) {
      b.active = false;
      continue;
    }

    if (hitBunker(b.x, b.y) || hitBunker(b.x, b.y + 1)) {
      b.active = false;
      continue;
    }

    if (!_shipHit && b.y + 1 >= SHIP_Y && b.y < SHIP_Y + SHIP_H &&
        b.x >= _shipX && b.x < _shipX + SHIP_W) {
      b.active = false;
      drawShip(_shipX, COLOR_BLACK);
      triggerBoom(_shipX + 3, SHIP_Y + 2, COLOR_BOMB);
      _shipHit = true;
      _shipRespawnAt = millis() + 700;
      continue;
    }

    _display->drawFastVLine(b.x, b.y, 2, COLOR_BOMB);
  }
}

void InvadersClockface::updateUfo()
{
  unsigned long now = millis();
  if (!_ufoActive) {
    if (now >= _nextUfoMillis) {
      _ufoActive = true;
      _ufoDir = (random(2) == 0) ? 1 : -1;
      _ufoX = (_ufoDir > 0) ? -7 : 64;
    }
    return;
  }

  drawUfo(_ufoX, COLOR_BLACK);
  _ufoX += _ufoDir;
  if ((_ufoDir > 0 && _ufoX >= 64) || (_ufoDir < 0 && _ufoX <= -7)) {
    _ufoActive = false;
    _nextUfoMillis = now + random(10000, 16000);
    return;
  }
  drawUfo(_ufoX, COLOR_UFO);
}

void InvadersClockface::updateClockBar()
{
  if (!_dateTime || !_display) return;

  int minute = _dateTime->getMinute();
  if (minute == _lastMinute) return;
  _lastMinute = minute;

  _display->fillRect(0, 51, 64, 13, COLOR_BLACK);

  int h24 = _dateTime->getHour();
  bool am = _dateTime->isAM();
  int h12 = h24 % 12;
  if (h12 == 0) h12 = 12;

  int16_t x = 2;
  const int16_t y = 55;
  if (h12 >= 10) {
    drawCompactClockDigit(_display, h12 / 10, x, y, 0xFFFF, 1);
    x += 4;
  }
  drawCompactClockDigit(_display, h12 % 10, x, y, 0xFFFF, 1);
  x += 4;
  _display->drawPixel(x, y + 1, 0xFFFF);
  _display->drawPixel(x, y + 3, 0xFFFF);
  x += 2;
  drawCompactClockDigit(_display, minute / 10, x, y, 0xFFFF, 1);
  x += 4;
  drawCompactClockDigit(_display, minute % 10, x, y, 0xFFFF, 1);
  x += 5;
  drawAmPm(_display, am, x, y, 0xFFE0);

  int m = _dateTime->getMonth();
  const char *mon = (m >= 1 && m <= 12) ? MONTH_NAMES[m - 1] : "---";
  char dateBuf[8];
  snprintf(dateBuf, sizeof(dateBuf), "%s %d", mon, _dateTime->getDay());

  _display->setFont(&Picopixel);
  _display->setTextSize(1);
  _display->setTextColor(0x07FF);
  int16_t x1, y1;
  uint16_t w, h;
  _display->getTextBounds(dateBuf, 0, 59, &x1, &y1, &w, &h);
  _display->setCursor(62 - w - x1, 59);
  _display->print(dateBuf);
}

void InvadersClockface::update()
{
  if (!_display) return;
  unsigned long now = millis();

  if (_boomActive && (now - _boomStart >= 140)) {
    _boomActive = false;
    _display->fillRect(_boomX - 2, _boomY - 2, 5, 5, COLOR_BLACK);
  }

  // Arcade speed-up: fewer aliens left => faster march!
  unsigned long alienInterval = 55 + static_cast<unsigned long>(_aliveCount) * 16;
  if (now - _lastAlienMillis >= alienInterval) {
    _lastAlienMillis = now;
    updateAliens();
  }

  if (now - _lastShipMillis >= SHIP_STEP_MS) {
    _lastShipMillis = now;
    updateShip();
  }

  if (now - _lastLaserMillis >= LASER_STEP_MS) {
    _lastLaserMillis = now;
    updateLaser();
  }

  if (now - _lastBombMillis >= BOMB_STEP_MS) {
    _lastBombMillis = now;
    updateBombs();
  }

  if (now - _lastUfoMillis >= UFO_STEP_MS) {
    _lastUfoMillis = now;
    updateUfo();
  }

  if (now - _lastClockMillis >= 500) {
    _lastClockMillis = now;
    updateClockBar();
  }
}
