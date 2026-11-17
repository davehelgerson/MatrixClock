#ifndef InvadersClockface_h
#define InvadersClockface_h

#include <Adafruit_GFX.h>
#include "../core/IClockface.h"
#include "../core/CWDateTime.h"

/**
 * Retro Space Invaders arcade clockface with accelerating alien fleet,
 * enemy bombs, destructible pixel bunkers, smart dodging/aiming ship AI,
 * occasional mystery UFO, and a compact time/date bar at the bottom.
 */
class InvadersClockface : public IClockface {
  private:
    static const int ALIEN_ROWS = 3;
    static const int ALIEN_COLS = 5;
    static const int ALIEN_W = 7;
    static const int ALIEN_H = 5;
    static const int SHIP_W = 7;
    static const int SHIP_H = 4;
    static const int SHIP_Y = 43;

    static const int BUNKER_COUNT = 3;
    static const int BUNKER_W = 6;
    static const int BUNKER_H = 3;
    static const int BUNKER_Y = 37;
    static const int MAX_BOMBS = 2;

    static const unsigned long SHIP_STEP_MS  = 50;
    static const unsigned long LASER_STEP_MS = 30;
    static const unsigned long BOMB_STEP_MS  = 55;
    static const unsigned long UFO_STEP_MS   = 65;

    struct Bomb {
      bool active;
      int16_t x;
      int16_t y;
    };

    Adafruit_GFX *_display;
    CWDateTime *_dateTime = nullptr;

    bool _alive[ALIEN_ROWS][ALIEN_COLS];
    int16_t _fleetX = 4;
    int16_t _fleetY = 4;
    int8_t _fleetDir = 1;
    bool _alienFrame = false;
    int _aliveCount = ALIEN_ROWS * ALIEN_COLS;

    uint8_t _bunkers[BUNKER_COUNT][BUNKER_H];
    int16_t _bunkerX[BUNKER_COUNT] = {9, 29, 49};

    int16_t _shipX = 28;
    int8_t _shipDir = 1;
    bool _shipHit = false;
    unsigned long _shipRespawnAt = 0;

    bool _laserActive = false;
    int16_t _laserX = 0;
    int16_t _laserY = 0;

    Bomb _bombs[MAX_BOMBS];

    bool _ufoActive = false;
    int16_t _ufoX = 0;
    int8_t _ufoDir = 1;
    unsigned long _nextUfoMillis = 0;

    bool _boomActive = false;
    int16_t _boomX = 0;
    int16_t _boomY = 0;
    unsigned long _boomStart = 0;

    unsigned long _lastAlienMillis = 0;
    unsigned long _lastShipMillis = 0;
    unsigned long _lastLaserMillis = 0;
    unsigned long _lastBombMillis = 0;
    unsigned long _lastUfoMillis = 0;
    unsigned long _lastClockMillis = 0;
    int8_t _lastMinute = -1;

    void resetWave();
    void resetBunkers();
    void drawBunkers();
    bool hitBunker(int16_t x, int16_t y);
    void drawAlien(int16_t x, int16_t y, int row, bool frame, uint16_t color);
    void drawShip(int16_t x, uint16_t color);
    void drawUfo(int16_t x, uint16_t color);
    void triggerBoom(int16_t cx, int16_t cy, uint16_t color);
    void updateAliens();
    void updateShip();
    void updateLaser();
    void updateBombs();
    void updateUfo();
    void updateClockBar();

  public:
    InvadersClockface(Adafruit_GFX *display);
    void setup(CWDateTime *dateTime) override;
    void update() override;
    void teardown() override;
};

#endif
