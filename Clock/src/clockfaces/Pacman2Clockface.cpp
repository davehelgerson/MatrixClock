#include "Pacman2Clockface.h"
#include "ClockfaceCommon.h"
#include "../core/Locator.h"
#include "../fonts/hour_font.h"
#include "../fonts/picopixel.h"

namespace {
const uint16_t FOOD_COLOR = 0xB58C;
const uint16_t WALL_COLOR = 0x0016;
const uint16_t SPCFOOD_COLOR = 0xFBE0;
const uint16_t PACMAN_COLOR = 0xFE40;

const uint8_t GHOST_BITMAP[2][5] = {
  {
    0b01110,
    0b11111,
    0b11111,
    0b11111,
    0b10101
  },
  {
    0b01110,
    0b11111,
    0b11111,
    0b11111,
    0b01010
  }
};

const uint8_t DEATH_FRAMES[5][5] = {
  {
    0b01110,
    0b11000,
    0b10000,
    0b11000,
    0b01110
  },
  {
    0b01110,
    0b10001,
    0b00000,
    0b10001,
    0b01110
  },
  {
    0b00100,
    0b10001,
    0b00000,
    0b10001,
    0b00100
  },
  {
    0b00000,
    0b01010,
    0b00100,
    0b01010,
    0b00000
  },
  {
    0b00000,
    0b00000,
    0b00100,
    0b00000,
    0b00000
  }
};

Direction oppositeDir(Direction dir)
{
  switch (dir) {
    case Direction::UP:    return Direction::DOWN;
    case Direction::DOWN:  return Direction::UP;
    case Direction::LEFT:  return Direction::RIGHT;
    case Direction::RIGHT: return Direction::LEFT;
  }
  return Direction::RIGHT;
}
}  // namespace

Pacman2Clockface::Pacman2Clockface(Adafruit_GFX* display)
    : _display(display), _dateTime(nullptr)
{
  Locator::provide(display);
  Locator::provide(&eventBus);
}

void Pacman2Clockface::setup(CWDateTime *dateTime)
{
  _dateTime = dateTime;
  _dying = false;
  _deathFrame = 0;
  _showColon = true;
  _invincibleStartMillis = 0;

  memcpy(_MAP, _MAP_CONST, sizeof(_MAP_CONST));
  Locator::getDisplay()->setTextSize(1);
  Locator::getDisplay()->setFont(&hourFont);
  drawMap();
  initGhosts();
  updateClock();

  unsigned long now = millis();
  _lastMoveMillis = now;
  _lastGhostMillis = now;
  _lastSecMillis = now;
}

void Pacman2Clockface::teardown()
{
  delete _pacman;
  _pacman = nullptr;
}

void Pacman2Clockface::spawnPacman()
{
  delete _pacman;
  _pacman = new Pacman((PACMAN_SPAWN_COL * 5) + 2, (PACMAN_SPAWN_ROW * 5) + 2);
  _pacman->turn(Direction::LEFT);
  _pacman->init();
}

void Pacman2Clockface::initGhosts()
{
  _ghosts[0] = {0, 0, Direction::RIGHT, 0xF800, 1, 0, false};
  _ghosts[1] = {0, 0, Direction::LEFT,  0x07FF, 10, 0, true};
  for (int i = 0; i < GHOST_COUNT; i++) {
    resetGhost(_ghosts[i]);
  }
}

void Pacman2Clockface::resetGhost(Ghost &g)
{
  g.x = (g.spawnCol * 5) + 2;
  g.y = (g.spawnRow * 5) + 2;
  g.dir = (g.spawnCol < MAP_SIZE / 2) ? Direction::RIGHT : Direction::LEFT;
  drawGhost(g);
}

void Pacman2Clockface::drawGhost(const Ghost &g)
{
  auto* display = Locator::getDisplay();
  uint16_t bodyColor = g.color;
  uint16_t eyeColor = 0xFFFF;

  if (_pacman && _pacman->_state == Pacman::State::INVENCIBLE) {
    bool flashing = (millis() - _invincibleStartMillis) >= 5000 && ((millis() / 200) % 2 == 0);
    bodyColor = flashing ? 0xFFFF : 0x001F;
    eyeColor = flashing ? 0x001F : 0xFFFF;
  }

  const uint8_t* rows = GHOST_BITMAP[g.animFrame ? 1 : 0];

  for (int r = 0; r < 5; r++) {
    for (int c = 0; c < 5; c++) {
      if (rows[r] & (1 << (4 - c))) {
        uint16_t pxColor = (r == 1 && (c == 1 || c == 3)) ? eyeColor : bodyColor;
        display->drawPixel(g.x + c, g.y + r, pxColor);
      }
    }
  }
}

void Pacman2Clockface::redrawCell(int col, int row)
{
  if (col < 0 || col >= MAP_SIZE || row < 0 || row >= MAP_SIZE) {
    return;
  }
  auto* display = Locator::getDisplay();
  byte blk = _MAP[row][col];
  if (blk == MapBlock::FOOD || blk == MapBlock::GATE) {
    display->fillRect((col * 5) + 3, (row * 5) + 4, 3, 1, FOOD_COLOR);
  } else if (blk == MapBlock::SUPER_FOOD) {
    display->fillRect((col * 5) + 3, (row * 5) + 3, 3, 3, SPCFOOD_COLOR);
  }
}

void Pacman2Clockface::eraseGhostAndRestore(const Ghost &g)
{
  Locator::getDisplay()->fillRect(g.x, g.y, 5, 5, 0x0000);

  int col0 = (g.x - 2) / 5;
  int row0 = (g.y - 2) / 5;
  redrawCell(col0, row0);
  if ((g.x - 2) % 5 != 0) {
    redrawCell(col0 + 1, row0);
  }
  if ((g.y - 2) % 5 != 0) {
    redrawCell(col0, row0 + 1);
  }
}

void Pacman2Clockface::update()
{
  if (!_dateTime || !_pacman) return;

  if ((millis() - _lastSecMillis) >= 1000) {
    _showColon = !_showColon;
    updateClock();
    _lastSecMillis = millis();
  }

  if (_dying) {
    updateDeathAnimation();
    return;
  }

  if (millis() - _lastMoveMillis >= 75) {
    bool fullBlock =
        ((_pacman->_direction == Direction::LEFT || _pacman->_direction == Direction::RIGHT) && (_pacman->getX() - 2) % 5 == 0) ||
        ((_pacman->_direction == Direction::UP || _pacman->_direction == Direction::DOWN) && (_pacman->getY() - 2) % 5 == 0);

    bool wrapped = false;
    if (fullBlock) {
      int col = (_pacman->getX() - 2) / 5;
      int row = (_pacman->getY() - 2) / 5;
      if (col >= 0 && col < MAP_SIZE && row >= 0 && row < MAP_SIZE) {
        MapBlock currentBlk = static_cast<MapBlock>(_MAP[row][col]);
        _MAP[row][col] = MapBlock::EMPTY;

        if (currentBlk == MapBlock::SUPER_FOOD) {
          _pacman->setState(Pacman::State::INVENCIBLE);
          _invincibleStartMillis = millis();
        }
      }

      if (countBlocks(MapBlock::FOOD) == 0) {
        resetMap();
        return;
      }

      choosePacmanDirection();

      // Side tunnel wrap-around on rows 5 and 6
      if (isTunnelRow(row)) {
        if (col == 0 && _pacman->_direction == Direction::LEFT) {
          Locator::getDisplay()->fillRect(_pacman->getX(), _pacman->getY(), 5, 5, 0x0000);
          _pacman->setPosition(((MAP_SIZE - 1) * 5) + 2, _pacman->getY());
          _pacman->init();
          wrapped = true;
        } else if (col == MAP_SIZE - 1 && _pacman->_direction == Direction::RIGHT) {
          Locator::getDisplay()->fillRect(_pacman->getX(), _pacman->getY(), 5, 5, 0x0000);
          _pacman->setPosition(2, _pacman->getY());
          _pacman->init();
          wrapped = true;
        }
      }
    }
    if (!wrapped) {
      _pacman->update();
    }
    _lastMoveMillis = millis();

    if (checkGhostCollisions()) {
      return;
    }
  }

  if (millis() - _lastGhostMillis >= 85) {
    updateGhosts();
    _lastGhostMillis = millis();
    checkGhostCollisions();
  }
}

void Pacman2Clockface::updateGhosts()
{
  for (int i = 0; i < GHOST_COUNT; i++) {
    Ghost &g = _ghosts[i];
    eraseGhostAndRestore(g);

    bool atTile = ((g.x - 2) % 5 == 0) && ((g.y - 2) % 5 == 0);
    if (atTile) {
      turnGhost(i);

      int col = (g.x - 2) / 5;
      int row = (g.y - 2) / 5;
      if (isTunnelRow(row)) {
        if (col == 0 && g.dir == Direction::LEFT) {
          g.x = ((MAP_SIZE - 1) * 5) + 2;
          g.animFrame = !g.animFrame;
          drawGhost(g);
          continue;
        } else if (col == MAP_SIZE - 1 && g.dir == Direction::RIGHT) {
          g.x = 2;
          g.animFrame = !g.animFrame;
          drawGhost(g);
          continue;
        }
      }
    }

    if (g.dir == Direction::RIGHT) g.x += 1;
    else if (g.dir == Direction::LEFT) g.x -= 1;
    else if (g.dir == Direction::DOWN) g.y += 1;
    else if (g.dir == Direction::UP) g.y -= 1;

    if ((g.x + g.y) % 2 == 0) {
      g.animFrame = !g.animFrame;
    }
    drawGhost(g);
  }
}

void Pacman2Clockface::turnGhost(int ghostIdx)
{
  Ghost &g = _ghosts[ghostIdx];
  Direction candidates[4];
  int count = 0;
  Direction opp = oppositeDir(g.dir);

  for (int d = 0; d < 4; d++) {
    Direction dir = static_cast<Direction>(d);
    if (dir == opp) continue;
    if (contains(nextBlockAt(g.x, g.y, dir), PACMAN_MOVING_BLOCKS)) {
      candidates[count++] = dir;
    }
  }

  if (count == 0) {
    if (contains(nextBlockAt(g.x, g.y, opp), PACMAN_MOVING_BLOCKS)) {
      g.dir = opp;
    }
    return;
  }

  int col = (g.x - 2) / 5;
  int row = (g.y - 2) / 5;
  int pacCol = (_pacman->getX() - 2) / 5;
  int pacRow = (_pacman->getY() - 2) / 5;

  bool frightened = (_pacman && _pacman->_state == Pacman::State::INVENCIBLE);

  // Target tile: Red (0) chases Pac-Man directly; Cyan (1) targets 2 tiles ahead
  int targetCol = pacCol;
  int targetRow = pacRow;
  if (!frightened && ghostIdx == 1) {
    if (_pacman->_direction == Direction::RIGHT) targetCol += 2;
    else if (_pacman->_direction == Direction::LEFT) targetCol -= 2;
    else if (_pacman->_direction == Direction::DOWN) targetRow += 2;
    else if (_pacman->_direction == Direction::UP) targetRow -= 2;
  }

  // Frightened ghosts always flee; normal ghosts use chase bias (75% Red, 65% Cyan)
  int chaseChance = (ghostIdx == 0) ? 75 : 65;
  if (!frightened && random(100) >= chaseChance) {
    g.dir = candidates[random(count)];
    return;
  }

  Direction bestDir = candidates[0];
  int bestScore = frightened ? -1 : 9999;

  for (int i = 0; i < count; i++) {
    int ncol, nrow;
    if (!stepCell(col, row, candidates[i], ncol, nrow)) continue;
    int dist = abs(ncol - targetCol) + abs(nrow - targetRow);
    if (frightened) {
      if (dist > bestScore) {
        bestScore = dist;
        bestDir = candidates[i];
      }
    } else {
      if (dist < bestScore) {
        bestScore = dist;
        bestDir = candidates[i];
      }
    }
  }

  g.dir = bestDir;
}

bool Pacman2Clockface::checkGhostCollisions()
{
  if (!_pacman || _dying) return false;

  int px = _pacman->getX();
  int py = _pacman->getY();

  for (int i = 0; i < GHOST_COUNT; i++) {
    Ghost &g = _ghosts[i];
    if (abs(px - g.x) < 4 && abs(py - g.y) < 4) {
      if (_pacman->_state == Pacman::State::INVENCIBLE) {
        eraseGhostAndRestore(g);
        resetGhost(g);
      } else {
        _dying = true;
        _deathFrame = 0;
        _lastDeathMillis = millis();
        return true;
      }
    }
  }
  return false;
}

void Pacman2Clockface::updateDeathAnimation()
{
  if (millis() - _lastDeathMillis < 120) {
    return;
  }
  _lastDeathMillis = millis();

  auto* display = Locator::getDisplay();
  int px = _pacman->getX();
  int py = _pacman->getY();

  display->fillRect(px, py, 5, 5, 0x0000);

  if (_deathFrame < 5) {
    for (int r = 0; r < 5; r++) {
      for (int c = 0; c < 5; c++) {
        if (DEATH_FRAMES[_deathFrame][r] & (1 << (4 - c))) {
          display->drawPixel(px + c, py + r, PACMAN_COLOR);
        }
      }
    }
    _deathFrame++;
  } else {
    _dying = false;
    for (int i = 0; i < GHOST_COUNT; i++) {
      eraseGhostAndRestore(_ghosts[i]);
      resetGhost(_ghosts[i]);
    }
    spawnPacman();
    updateClock();
  }
}

void Pacman2Clockface::drawMap()
{
  auto* display = Locator::getDisplay();
  display->fillRect(0, 0, 64, 64, 0x0000);

  display->drawRect(0, 0, 64, 64, WALL_COLOR);
  display->drawRect(1, 1, 62, 62, WALL_COLOR);

  // Open side tunnels on rows 5 and 6 (y = 27..36)
  display->fillRect(0, 27, 2, 10, 0x0000);
  display->fillRect(62, 27, 2, 10, 0x0000);

  for (int i = 0; i < MAP_SIZE; i++) {
    for (int j = 0; j < MAP_SIZE; j++) {
      if (_MAP[j][i] == MapBlock::FOOD) {
        display->fillRect((i * 5) + 3, (j * 5) + 4, 3, 1, FOOD_COLOR);
      } else if (_MAP[j][i] == MapBlock::WALL || _MAP[j][i] == MapBlock::CLOCK) {
        display->fillRect((i * 5) + 2, (j * 5) + 2, 5, 5, WALL_COLOR);
      } else if (_MAP[j][i] == MapBlock::GATE) {
        display->fillRect((i * 5) + 3, (j * 5) + 4, 3, 1, FOOD_COLOR);
      } else if (_MAP[j][i] == MapBlock::SUPER_FOOD) {
        display->fillRect((i * 5) + 3, (j * 5) + 3, 3, 3, SPCFOOD_COLOR);
      } else if (_MAP[j][i] == MapBlock::PACMAN) {
        spawnPacman();
      }
    }
  }
}

void Pacman2Clockface::updateClock()
{
  auto* display = Locator::getDisplay();
  display->drawRect(12, 17, 40, 30, WALL_COLOR);
  display->drawRect(13, 18, 38, 28, WALL_COLOR);
  display->fillRect(14, 19, 36, 26, 0x0000);

  display->setTextSize(1);
  display->setFont(&Picopixel);
  display->setTextColor(0xAD55);

  char dateBuf[12];
  snprintf(dateBuf, sizeof(dateBuf), "%s %d",
           monthName(_dateTime->getMonth()),
           _dateTime->getDay());

  int16_t x1, y1;
  uint16_t w, h;
  display->getTextBounds(dateBuf, 0, 36, &x1, &y1, &w, &h);
  display->setCursor(32 - (w / 2) - x1, 36);
  display->print(dateBuf);

  display->setTextColor(PACMAN_COLOR);
  const char* dayStr = weekDayName(_dateTime->getWeekday());
  display->getTextBounds(dayStr, 0, 43, &x1, &y1, &w, &h);
  display->setCursor(32 - (w / 2) - x1, 43);
  display->print(dayStr);

  drawCompactClockTime(display, _dateTime->getHour(), _dateTime->getMinute(), 15, 20, PACMAN_COLOR, 2, _showColon);
}

Pacman2Clockface::MapBlock Pacman2Clockface::nextBlock()
{
  return nextBlock(_pacman->_direction);
}

Pacman2Clockface::MapBlock Pacman2Clockface::nextBlock(Direction dir)
{
  return nextBlockAt(_pacman->getX(), _pacman->getY(), dir);
}

bool Pacman2Clockface::isTunnelRow(int row) const
{
  return row == 5 || row == 6;
}

bool Pacman2Clockface::stepCell(int col, int row, Direction dir, int &outCol, int &outRow) const
{
  if (col < 0 || col >= MAP_SIZE || row < 0 || row >= MAP_SIZE) {
    return false;
  }
  outCol = col;
  outRow = row;

  if (dir == Direction::RIGHT) {
    if (col + 1 < MAP_SIZE) {
      outCol = col + 1;
      return true;
    } else if (isTunnelRow(row)) {
      outCol = 0;
      return true;
    }
  } else if (dir == Direction::LEFT) {
    if (col - 1 >= 0) {
      outCol = col - 1;
      return true;
    } else if (isTunnelRow(row)) {
      outCol = MAP_SIZE - 1;
      return true;
    }
  } else if (dir == Direction::DOWN) {
    if (row + 1 < MAP_SIZE) {
      outRow = row + 1;
      return true;
    }
  } else if (dir == Direction::UP) {
    if (row - 1 >= 0) {
      outRow = row - 1;
      return true;
    }
  }
  return false;
}

bool Pacman2Clockface::isTileNearGhost(int col, int row) const
{
  if (_pacman && _pacman->_state == Pacman::State::INVENCIBLE) {
    return false;
  }
  for (int i = 0; i < GHOST_COUNT; i++) {
    int gCol = (_ghosts[i].x - 2) / 5;
    int gRow = (_ghosts[i].y - 2) / 5;
    if (abs(col - gCol) + abs(row - gRow) <= 2) {
      return true;
    }
  }
  return false;
}

Pacman2Clockface::MapBlock Pacman2Clockface::nextBlockAt(int x, int y, Direction dir)
{
  int col = (x - 2) / 5;
  int row = (y - 2) / 5;
  int ncol, nrow;
  if (stepCell(col, row, dir, ncol, nrow)) {
    return static_cast<MapBlock>(_MAP[nrow][ncol]);
  }
  return MapBlock::OUT_OF_MAP;
}

const char* Pacman2Clockface::weekDayName(int weekday)
{
  return (weekday >= 0 && weekday < 7) ? _weekDayNames[weekday] : "";
}

const char* Pacman2Clockface::monthName(int month)
{
  if (month < 1 || month > 12) {
    return "---";
  }
  strncpy(monthTemp, _monthWords + ((month - 1) * 4), 4);
  monthTemp[3] = '\0';
  return monthTemp;
}

void Pacman2Clockface::resetMap()
{
  memcpy(_MAP, _MAP_CONST, sizeof(_MAP_CONST));
  drawMap();
  initGhosts();
  updateClock();
}

void Pacman2Clockface::choosePacmanDirection()
{
  int col = (_pacman->getX() - 2) / 5;
  int row = (_pacman->getY() - 2) / 5;
  Direction cur = _pacman->_direction;
  Direction opp = oppositeDir(cur);
  bool isSuper = (_pacman->_state == Pacman::State::INVENCIBLE);

  // If super, chase the nearest ghost within 5 tiles
  if (isSuper) {
    int nearestDist = 9999;
    int targetCol = -1;
    int targetRow = -1;
    for (int i = 0; i < GHOST_COUNT; i++) {
      int gCol = (_ghosts[i].x - 2) / 5;
      int gRow = (_ghosts[i].y - 2) / 5;
      int d = abs(col - gCol) + abs(row - gRow);
      if (d <= 5 && d < nearestDist) {
        nearestDist = d;
        targetCol = gCol;
        targetRow = gRow;
      }
    }

    if (targetCol >= 0) {
      Direction bestDir = cur;
      int bestDist = 9999;
      bool found = false;
      for (int d = 0; d < 4; d++) {
        Direction dir = static_cast<Direction>(d);
        int ncol, nrow;
        if (!stepCell(col, row, dir, ncol, nrow)) continue;
        MapBlock blk = static_cast<MapBlock>(_MAP[nrow][ncol]);
        if (!contains(blk, PACMAN_MOVING_BLOCKS)) continue;
        // Avoid wasting another super pellet while already super if another path exists
        int penalty = (blk == MapBlock::SUPER_FOOD) ? 10 : 0;
        int dist = abs(ncol - targetCol) + abs(nrow - targetRow) + penalty;
        if (!found || dist < bestDist) {
          bestDist = dist;
          bestDir = dir;
          found = true;
        }
      }
      if (found) {
        if (bestDir != cur) _pacman->turn(bestDir);
        return;
      }
    }
  }

  Direction safeFood[4];
  int safeFoodCount = 0;
  bool straightIsSafeFood = false;

  Direction safeSuper[4];
  int safeSuperCount = 0;

  Direction safeMove[4];
  int safeMoveCount = 0;
  bool straightIsSafeMove = false;

  Direction anyMove[4];
  int anyMoveCount = 0;

  for (int d = 0; d < 4; d++) {
    Direction dir = static_cast<Direction>(d);
    int ncol, nrow;
    if (!stepCell(col, row, dir, ncol, nrow)) continue;

    MapBlock blk = static_cast<MapBlock>(_MAP[nrow][ncol]);
    if (!contains(blk, PACMAN_MOVING_BLOCKS)) continue;

    anyMove[anyMoveCount++] = dir;
    if (!isTileNearGhost(ncol, nrow)) {
      if (dir != opp && !(isSuper && blk == MapBlock::SUPER_FOOD)) {
        safeMove[safeMoveCount++] = dir;
        if (dir == cur) straightIsSafeMove = true;
      }
      if (blk == MapBlock::FOOD || (!isSuper && blk == MapBlock::SUPER_FOOD)) {
        safeFood[safeFoodCount++] = dir;
        if (dir == cur) straightIsSafeFood = true;
      } else if (isSuper && blk == MapBlock::SUPER_FOOD) {
        safeSuper[safeSuperCount++] = dir;
      }
    }
  }

  // Add slight randomness (15%) at multi-way intersections so paths vary between runs
  if (safeFoodCount > 0 && !(safeMoveCount > 1 && random(100) < 15)) {
    Direction chosen = (straightIsSafeFood && random(100) % 2 == 0) ? cur : safeFood[random(safeFoodCount)];
    if (chosen != cur) _pacman->turn(chosen);
  } else if (safeMoveCount > 0) {
    Direction chosen = (straightIsSafeMove && random(100) % 2 == 0) ? cur : safeMove[random(safeMoveCount)];
    if (chosen != cur) _pacman->turn(chosen);
  } else if (safeSuperCount > 0) {
    Direction chosen = safeSuper[random(safeSuperCount)];
    if (chosen != cur) _pacman->turn(chosen);
  } else if (anyMoveCount > 0) {
    Direction nonOpp[4];
    int nonOppCount = 0;
    for (int i = 0; i < anyMoveCount; i++) {
      if (anyMove[i] != opp) nonOpp[nonOppCount++] = anyMove[i];
    }
    Direction chosen = (nonOppCount > 0) ? nonOpp[random(nonOppCount)] : anyMove[random(anyMoveCount)];
    if (chosen != cur) _pacman->turn(chosen);
  }
}

bool Pacman2Clockface::contains(int v, const int* values)
{
  for (int i = 1; i < values[0] + 1; i++) {
    if (v == values[i])
      return true;
  }
  return false;
}

void Pacman2Clockface::turnRandom()
{
  for (int attempt = 0; attempt < 20; attempt++) {
    int dir = random(4);
    _pacman->turn(static_cast<Direction>(dir));
    if (contains(nextBlock(), PACMAN_MOVING_BLOCKS)) {
      return;
    }
  }
  for (int d = 0; d < 4; d++) {
    _pacman->turn(static_cast<Direction>(d));
    if (contains(nextBlock(), PACMAN_MOVING_BLOCKS)) {
      return;
    }
  }
}

int Pacman2Clockface::countBlocks(MapBlock elem)
{
  int count = 0;
  for (int i = 0; i < MAP_SIZE; i++) {
    for (int j = 0; j < MAP_SIZE; j++) {
      if (_MAP[i][j] == elem)
        count++;
    }
  }
  return count;
}
