#ifndef Pacman2Clockface_h
#define Pacman2Clockface_h

#include <Adafruit_GFX.h>
#include "../core/IClockface.h"
#include "../core/CWDateTime.h"
#include "../core/Game.h"
#include "../sprites/pacman.h"

class Pacman2Clockface : public IClockface {
  private:
    static const int MAP_SIZE = 12;
    static const int GHOST_COUNT = 2;
    static const int PACMAN_SPAWN_COL = 4;
    static const int PACMAN_SPAWN_ROW = 9;

    Adafruit_GFX* _display;
    CWDateTime* _dateTime;
    Pacman* _pacman = nullptr;
    unsigned long _lastMoveMillis = 0;
    unsigned long _lastGhostMillis = 0;
    unsigned long _lastSecMillis = 0;
    unsigned long _lastDeathMillis = 0;
    unsigned long _invincibleStartMillis = 0;

    bool _dying = false;
    bool _showColon = true;
    uint8_t _deathFrame = 0;

    struct Ghost {
      int x;
      int y;
      Direction dir;
      uint16_t color;
      int spawnCol;
      int spawnRow;
      bool animFrame;
    };

    Ghost _ghosts[GHOST_COUNT];

    const char* _weekDayNames[7] = {"SUNDAY", "MONDAY", "TUESDAY", "WEDNESDAY", "THURSDAY", "FRIDAY", "SATURDAY"};
    const char* _monthWords = "JAN\0FEB\0MAR\0APR\0MAY\0JUN\0JUL\0AUG\0SEP\0OCT\0NOV\0DEC\0";
    char monthTemp[4] = "\0";

    enum MapBlock {
      EMPTY = 0,
      FOOD = 1,
      WALL = 2,
      GATE = 3,
      SUPER_FOOD = 4,
      CLOCK = 5,
      GHOST = 6,
      PACMAN = 7,
      OUT_OF_MAP = 99
    };

    const byte _MAP_CONST[12][12] = {
      {4,1,1,1,1,1,1,1,1,1,1,4},
      {1,2,2,1,2,2,2,2,1,2,2,1},
      {1,1,1,1,1,2,2,1,1,1,1,1},
      {2,1,5,5,5,5,5,5,5,5,1,2},
      {2,1,5,5,5,5,5,5,5,5,1,2},
      {3,1,5,5,5,5,5,5,5,5,1,3},
      {3,1,5,5,5,5,5,5,5,5,1,3},
      {2,1,5,5,5,5,5,5,5,5,1,2},
      {2,1,5,5,5,5,5,5,5,5,1,2},
      {1,1,1,1,7,2,2,1,1,1,1,1},
      {1,2,2,1,2,2,2,2,1,2,2,1},
      {4,1,1,1,1,1,1,1,1,1,1,4}
    };

    byte _MAP[12][12] = {};

    const byte MAP_BORDER_SIZE = 2;
    const byte MAP_MIN_POS = 0 + MAP_BORDER_SIZE;
    const byte MAP_MAX_POS = 64 - MAP_BORDER_SIZE;

    const int PACMAN_MOVING_BLOCKS[5] = {4, MapBlock::EMPTY, MapBlock::FOOD, MapBlock::GATE, MapBlock::SUPER_FOOD};
    const int PACMAN_BLOCKING_BLOCKS[4] = {3, MapBlock::OUT_OF_MAP, MapBlock::WALL, MapBlock::CLOCK};

    void drawMap();
    void redrawCell(int col, int row);
    void spawnPacman();
    void initGhosts();
    void resetGhost(Ghost &g);
    void drawGhost(const Ghost &g);
    void eraseGhostAndRestore(const Ghost &g);
    void updateGhosts();
    void updateDeathAnimation();
    bool checkGhostCollisions();

    bool isTunnelRow(int row) const;
    bool stepCell(int col, int row, Direction dir, int &outCol, int &outRow) const;
    bool isTileNearGhost(int col, int row) const;

    MapBlock nextBlockAt(int x, int y, Direction dir);
    MapBlock nextBlock(Direction dir);
    MapBlock nextBlock();
    void turnRandom();
    void turnGhost(int ghostIdx);
    int countBlocks(MapBlock elem);
    bool contains(int v, const int* values);
    void resetMap();
    void choosePacmanDirection();
    void updateClock();
    const char* weekDayName(int weekday);
    const char* monthName(int month);

  public:
    Pacman2Clockface(Adafruit_GFX* display);
    void setup(CWDateTime *dateTime) override;
    void update() override;
    void teardown() override;
};

#endif
