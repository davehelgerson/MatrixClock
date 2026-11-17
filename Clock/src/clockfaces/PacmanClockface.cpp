#include "PacmanClockface.h"
#include "ClockfaceCommon.h"
#include "../core/Locator.h"
#include "../fonts/hour_font.h"
#include "../fonts/picopixel.h"

Clockface2::Clockface2(Adafruit_GFX* display)
    : _display(display), _dateTime(nullptr)
{
  Locator::provide(display);
  Locator::provide(&eventBus);
}

void Clockface2::setup(CWDateTime *dateTime)
{
  _dateTime = dateTime;
  Locator::getDisplay()->setTextSize(1);
  Locator::getDisplay()->setFont(&hourFont);
  drawMap();
  updateClock();
  _lastMoveMillis = millis();
  _lastSecMillis = millis();
}

void Clockface2::update()
{
  if (!_dateTime || !_pacman) return;

  if ((millis() - _lastSecMillis) >= 1000) {
    updateClock();
    show_seconds = !show_seconds;
    _lastSecMillis = millis();
  }

  if (millis() - _lastMoveMillis >= 75) {
    bool fullBlock =
        ((_pacman->_direction == Direction::LEFT || _pacman->_direction == Direction::RIGHT) && (_pacman->getX() - 2) % 5 == 0) ||
        ((_pacman->_direction == Direction::UP || _pacman->_direction == Direction::DOWN) && (_pacman->getY() - 2) % 5 == 0);

    if (fullBlock) {
      MapBlock nextBlk = nextBlock();
      _MAP[(_pacman->getY() - 2) / 5][(_pacman->getX() - 2) / 5] = MapBlock::EMPTY;
      directionDecision(nextBlk, (_pacman->_direction == Direction::LEFT || _pacman->_direction == Direction::RIGHT));

      if (nextBlk == MapBlock::SUPER_FOOD) {
        _pacman->setState(Pacman::State::INVENCIBLE);
      }

      if (countBlocks(MapBlock::FOOD) == 0) {
        resetMap();
      }
    }
    _pacman->update();
    _lastMoveMillis = millis();
  }
}

void Clockface2::drawMap()
{
  Locator::getDisplay()->fillRect(0, 0, 64, 64, 0x0000);

  uint16_t food_color = 0xB58C;
  uint16_t wall_color = 0x0016;
  uint16_t spcfood_color = 0xFBE0;

  Locator::getDisplay()->drawRect(0, 0, 64, 64, wall_color);
  Locator::getDisplay()->drawRect(1, 1, 62, 62, wall_color);

  for (int i = 0; i < MAP_SIZE; i++) {
    for (int j = 0; j < MAP_SIZE; j++) {
      if (_MAP[j][i] == MapBlock::FOOD) {
        Locator::getDisplay()->fillRect((i * 5) + 3, (j * 5) + 4, 3, 1, food_color);
      } else if (_MAP[j][i] == MapBlock::WALL || _MAP[j][i] == MapBlock::CLOCK) {
        Locator::getDisplay()->fillRect((i * 5) + 2, (j * 5) + 2, 5, 5, wall_color);
      } else if (_MAP[j][i] == MapBlock::GATE) {
        Locator::getDisplay()->fillRect((i * 5) + 3, (j * 5) + 4, 3, 1, food_color);
      } else if (_MAP[j][i] == MapBlock::SUPER_FOOD) {
        Locator::getDisplay()->fillRect((i * 5) + 3, (j * 5) + 3, 3, 3, spcfood_color);
      } else if (_MAP[j][i] == MapBlock::PACMAN) {
        delete _pacman;
        _pacman = new Pacman((i * 5) + 2, (j * 5) + 2);
      }
    }
  }
}

void Clockface2::updateClock()
{
  auto* display = Locator::getDisplay();
  display->drawRect(12, 17, 40, 30, 0x0016);
  display->drawRect(13, 18, 38, 28, 0x0016);
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

  display->setTextColor(0xFE40);
  const char* dayStr = weekDayName(_dateTime->getWeekday());
  display->getTextBounds(dayStr, 0, 43, &x1, &y1, &w, &h);
  display->setCursor(32 - (w / 2) - x1, 43);
  display->print(dayStr);

  drawCompactClockTime(display, _dateTime->getHour(), _dateTime->getMinute(), 15, 20, 0xFE40, 2);
}

Clockface2::MapBlock Clockface2::nextBlock()
{
  return nextBlock(_pacman->_direction);
}

Clockface2::MapBlock Clockface2::nextBlock(Direction dir)
{
  MapBlock map_block = MapBlock::OUT_OF_MAP;

  if (dir == Direction::RIGHT) {
    if (_pacman->getX() + _pacman->SPRITE_SIZE < MAP_MAX_POS) {
      map_block = static_cast<MapBlock>(_MAP[(_pacman->getY() - 2) / 5][((_pacman->getX() - 2) / 5) + 1]);
    }
  } else if (dir == Direction::DOWN) {
    if (_pacman->getY() + _pacman->SPRITE_SIZE < MAP_MAX_POS) {
      map_block = static_cast<MapBlock>(_MAP[((_pacman->getY() - 2) / 5) + 1][(_pacman->getX() - 2) / 5]);
    }
  } else if (dir == Direction::LEFT) {
    if ((_pacman->getX() - 2) > 0) {
      map_block = static_cast<MapBlock>(_MAP[(_pacman->getY() - 2) / 5][((_pacman->getX() - 2) / 5) - 1]);
    }
  } else if (dir == Direction::UP) {
    if ((_pacman->getY() - 2) > 0) {
      map_block = static_cast<MapBlock>(_MAP[((_pacman->getY() - 2) / 5) - 1][((_pacman->getX() - 2) / 5)]);
    }
  }

  return map_block;
}

const char* Clockface2::weekDayName(int weekday)
{
  return (weekday >= 0 && weekday < 7) ? _weekDayNames[weekday] : "";
}

const char* Clockface2::monthName(int month)
{
  strncpy(monthTemp, _monthWords + ((month - 1) * 4), 4);
  return monthTemp;
}

void Clockface2::resetMap()
{
  memcpy(_MAP, _MAP_CONST, sizeof(_MAP_CONST));
  drawMap();
  updateClock();
}

void Clockface2::directionDecision(MapBlock nextBlk, bool moving_axis_x)
{
  if (contains(nextBlk, PACMAN_BLOCKING_BLOCKS)) {
    turnRandom();
  } else if (moving_axis_x && contains(nextBlock(Direction::DOWN), PACMAN_MOVING_BLOCKS) && random(100) % 2 == 0) {
    _pacman->turn(Direction::DOWN);
  } else if (moving_axis_x && contains(nextBlock(Direction::UP), PACMAN_MOVING_BLOCKS) && random(100) % 2 == 0) {
    _pacman->turn(Direction::UP);
  } else if (!moving_axis_x && contains(nextBlock(Direction::LEFT), PACMAN_MOVING_BLOCKS) && random(100) % 2 == 0) {
    _pacman->turn(Direction::LEFT);
  } else if (!moving_axis_x && contains(nextBlock(Direction::RIGHT), PACMAN_MOVING_BLOCKS) && random(100) % 2 == 0) {
    _pacman->turn(Direction::RIGHT);
  }
}

bool Clockface2::contains(int v, const int* values)
{
  for (int i = 1; i < values[0] + 1; i++) {
    if (v == values[i])
      return true;
  }
  return false;
}

void Clockface2::turnRandom()
{
  int dir = random(4);
  do {
    _pacman->turn(static_cast<Direction>(dir));
    dir = random(4);
  } while (!contains(nextBlock(), PACMAN_MOVING_BLOCKS));
}

int Clockface2::countBlocks(MapBlock elem)
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

