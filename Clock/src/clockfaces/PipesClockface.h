#ifndef PipesClockface_h
#define PipesClockface_h

#include <Adafruit_GFX.h>
#include "../core/IClockface.h"
#include "../core/CWDateTime.h"

/**
 * 64x64 pixel-art LED matrix animation inspired by the classic 1990s 3D Pipes
 * screensaver: snake-like colorful pipes (cyan, magenta, yellow, white, and
 * occasional candy-cane) enter from screen edges, make sharp 90-degree turns
 * with spherical joints and rare teapots, and navigate back off-screen.
 */
class PipesClockface : public IClockface {
  private:
    static const int GRID_SIZE = 16;   // 16 x 4px = 64px
    static const int CELL_PX = 4;
    static const int MAX_PIPES = 3;
    static const int MAX_PIPE_LEN = 34;
    static const unsigned long STEP_INTERVAL_MS = 55;  // 55ms/px => 220ms/cell

    enum Dir : uint8_t {
      DIR_UP = 0,
      DIR_RIGHT = 1,
      DIR_DOWN = 2,
      DIR_LEFT = 3
    };

    enum PipeStyle : uint8_t {
      STYLE_CYAN = 0,
      STYLE_MAGENTA = 1,
      STYLE_YELLOW = 2,
      STYLE_WHITE = 3,
      STYLE_CANDY = 4
    };

    struct CellPos {
      int8_t x;
      int8_t y;
    };

    struct PipeHead {
      int8_t gx;
      int8_t gy;
      Dir dir;
      PipeStyle style;
      uint16_t stepCount;
      uint8_t subStep;     // 0..3 pixel extrusion within current cell
      uint8_t targetLen;   // length in cells before tail starts following
      uint8_t len;         // current number of cells in ring buffer
      uint8_t headIdx;     // ring buffer write index
      uint8_t tailIdx;     // ring buffer read index
      CellPos cells[MAX_PIPE_LEN];
      bool headDone;       // true once head exits off-screen or dead-ends
      bool retracting;     // true once tail has started following head
      bool active;
    };

    Adafruit_GFX* _display;
    bool _occupied[GRID_SIZE][GRID_SIZE];
    bool _hasTeapot[GRID_SIZE][GRID_SIZE];
    PipeHead _pipes[MAX_PIPES];
    uint8_t _teapotCount = 0;
    unsigned long _lastStepMillis = 0;

    void resetGrid();
    bool spawnEdgePipe(int slot);
    void stepPipe(int slot);
    void retractTailSlice(PipeHead &p, uint8_t subStep);
    bool canMove(int8_t gx, int8_t gy, Dir d) const;
    bool isOffScreen(int8_t gx, int8_t gy, Dir d) const;
    void drawPipeSlice(int8_t gx, int8_t gy, Dir d, PipeStyle style, uint16_t step, uint8_t subStep);
    void drawJoint(int8_t gx, int8_t gy, PipeStyle style);
    void drawTeapot(int8_t gx, int8_t gy);
    static void getStyleColors(PipeStyle style, uint16_t &bright, uint16_t &dark);

  public:
    PipesClockface(Adafruit_GFX* display);
    void setup(CWDateTime *dateTime) override;
    void update() override;
    void teardown() override;
};

#endif
