#include "PipesClockface.h"

namespace {
const uint16_t COLOR_BLACK = 0x0000;
const uint16_t COLOR_WHITE = 0xFFFF;
const uint16_t COLOR_RED   = 0xF800;

const int8_t DX[4] = { 0, 1, 0, -1};
const int8_t DY[4] = {-1, 0, 1,  0};
}  // namespace

PipesClockface::PipesClockface(Adafruit_GFX* display)
  : _display(display)
{
}

void PipesClockface::setup(CWDateTime *dateTime)
{
  (void)dateTime;
  randomSeed(esp_random());
  resetGrid();
  _lastStepMillis = millis();
}

void PipesClockface::teardown()
{
  if (_display) {
    _display->fillRect(0, 0, 64, 64, COLOR_BLACK);
  }
  for (int i = 0; i < MAX_PIPES; i++) {
    _pipes[i].active = false;
  }
}

void PipesClockface::resetGrid()
{
  for (int y = 0; y < GRID_SIZE; y++) {
    for (int x = 0; x < GRID_SIZE; x++) {
      _occupied[y][x] = false;
      _hasTeapot[y][x] = false;
    }
  }
  _teapotCount = 0;

  for (int i = 0; i < MAX_PIPES; i++) {
    _pipes[i].active = false;
  }

  if (_display) {
    _display->fillRect(0, 0, 64, 64, COLOR_BLACK);
  }

  spawnEdgePipe(0);
  spawnEdgePipe(1);
  spawnEdgePipe(2);
}

void PipesClockface::getStyleColors(PipeStyle style, uint16_t &bright, uint16_t &dark)
{
  switch (style) {
    case STYLE_CYAN:
      bright = 0x07FF;
      dark   = 0x0415;
      break;
    case STYLE_MAGENTA:
      bright = 0xF81F;
      dark   = 0x8010;
      break;
    case STYLE_YELLOW:
      bright = 0xFFE0;
      dark   = 0x8400;
      break;
    case STYLE_WHITE:
      bright = 0xFFFF;
      dark   = 0x8410;
      break;
    case STYLE_CANDY:
    default:
      bright = COLOR_RED;
      dark   = 0x9000;
      break;
  }
}

bool PipesClockface::isOffScreen(int8_t gx, int8_t gy, Dir d) const
{
  int8_t nx = gx + DX[d];
  int8_t ny = gy + DY[d];
  return (nx < 0 || nx >= GRID_SIZE || ny < 0 || ny >= GRID_SIZE);
}

bool PipesClockface::canMove(int8_t gx, int8_t gy, Dir d) const
{
  if (isOffScreen(gx, gy, d)) {
    return false;
  }
  return !_occupied[gy + DY[d]][gx + DX[d]];
}

bool PipesClockface::spawnEdgePipe(int slot)
{
  if (slot < 0 || slot >= MAX_PIPES) return false;

  int8_t gx = -1;
  int8_t gy = -1;
  Dir dir = DIR_RIGHT;

  for (int attempt = 0; attempt < 24; attempt++) {
    int edge = random(4);
    int pos = random(1, GRID_SIZE - 1);
    if (edge == 0) { gx = pos; gy = 0; dir = DIR_DOWN; }
    else if (edge == 1) { gx = GRID_SIZE - 1; gy = pos; dir = DIR_LEFT; }
    else if (edge == 2) { gx = pos; gy = GRID_SIZE - 1; dir = DIR_UP; }
    else { gx = 0; gy = pos; dir = DIR_RIGHT; }

    if (!_occupied[gy][gx]) {
      break;
    }
    gx = -1;
  }

  if (gx < 0 || gy < 0 || _occupied[gy][gx]) {
    return false;
  }

  PipeStyle style = (random(100) < 15)
      ? STYLE_CANDY
      : static_cast<PipeStyle>(random(4));

  _occupied[gy][gx] = true;
  _hasTeapot[gy][gx] = false;

  PipeHead &p = _pipes[slot];
  p.gx = gx;
  p.gy = gy;
  p.dir = dir;
  p.style = style;
  p.stepCount = 0;
  p.subStep = 0;
  p.targetLen = random(10, 31);
  p.len = 1;
  p.tailIdx = 0;
  p.headIdx = 1;
  p.cells[0] = {gx, gy};
  p.headDone = false;
  p.retracting = false;
  p.active = true;

  return true;
}

void PipesClockface::drawJoint(int8_t gx, int8_t gy, PipeStyle style)
{
  if (!_display) return;
  int16_t px = gx * CELL_PX;
  int16_t py = gy * CELL_PX;

  uint16_t bright, dark;
  if (style == STYLE_CANDY) {
    bright = COLOR_RED;
    dark   = 0x9000;
  } else {
    getStyleColors(style, bright, dark);
  }

  // 4x4 corner-clipped spherical joint with top-left specular highlight
  _display->drawPixel(px + 1, py + 0, bright);
  _display->drawPixel(px + 2, py + 0, bright);

  _display->drawPixel(px + 0, py + 1, bright);
  _display->drawPixel(px + 1, py + 1, COLOR_WHITE);
  _display->drawPixel(px + 2, py + 1, bright);
  _display->drawPixel(px + 3, py + 1, dark);

  _display->drawPixel(px + 0, py + 2, bright);
  _display->drawPixel(px + 1, py + 2, bright);
  _display->drawPixel(px + 2, py + 2, bright);
  _display->drawPixel(px + 3, py + 2, dark);

  _display->drawPixel(px + 1, py + 3, dark);
  _display->drawPixel(px + 2, py + 3, dark);
}

void PipesClockface::drawTeapot(int8_t gx, int8_t gy)
{
  if (!_display) return;
  int16_t px = (gx * CELL_PX) - 1;
  int16_t py = (gy * CELL_PX) - 1;

  const uint16_t B = 0xFEA0;
  const uint16_t D = 0x9240;
  const uint16_t W = COLOR_WHITE;

  const uint16_t teapot[5][5] = {
    {0, 0, W, 0, 0},
    {0, B, B, B, 0},
    {B, B, W, B, B},
    {0, B, B, B, D},
    {0, D, D, D, 0}
  };

  for (int r = 0; r < 5; r++) {
    for (int c = 0; c < 5; c++) {
      uint16_t col = teapot[r][c];
      if (col != 0) {
        int16_t x = px + c;
        int16_t y = py + r;
        if (x >= 0 && x < 64 && y >= 0 && y < 64) {
          _display->drawPixel(x, y, col);
        }
      }
    }
  }
  _hasTeapot[gy][gx] = true;
}

void PipesClockface::drawPipeSlice(int8_t gx, int8_t gy, Dir d, PipeStyle style, uint16_t step, uint8_t subStep)
{
  if (!_display) return;
  int16_t px = gx * CELL_PX;
  int16_t py = gy * CELL_PX;

  uint16_t bright, dark;
  getStyleColors(style, bright, dark);

  uint16_t c1 = bright;
  uint16_t c2 = dark;
  if (style == STYLE_CANDY) {
    bool isWhiteStripe = (((step * CELL_PX) + subStep) / 2) % 2 == 0;
    c1 = isWhiteStripe ? COLOR_WHITE : COLOR_RED;
    c2 = isWhiteStripe ? 0xCE79 : 0x9000;
  }

  bool glint = (style != STYLE_CANDY && (step % 2 == 0) && subStep == 1);

  if (d == DIR_RIGHT || d == DIR_LEFT) {
    int16_t x = (d == DIR_RIGHT) ? (px + subStep) : (px + (CELL_PX - 1 - subStep));
    _display->drawPixel(x, py + 1, glint ? COLOR_WHITE : c1);
    _display->drawPixel(x, py + 2, c2);
  } else {
    int16_t y = (d == DIR_DOWN) ? (py + subStep) : (py + (CELL_PX - 1 - subStep));
    _display->drawPixel(px + 1, y, glint ? COLOR_WHITE : c1);
    _display->drawPixel(px + 2, y, c2);
  }
}

void PipesClockface::retractTailSlice(PipeHead &p, uint8_t subStep)
{
  if (p.len == 0) return;

  CellPos tail = p.cells[p.tailIdx];
  Dir tailDir = p.dir;
  if (p.len > 1) {
    CellPos next = p.cells[(p.tailIdx + 1) % MAX_PIPE_LEN];
    int8_t dx = next.x - tail.x;
    int8_t dy = next.y - tail.y;
    if (dx > 0) tailDir = DIR_RIGHT;
    else if (dx < 0) tailDir = DIR_LEFT;
    else if (dy > 0) tailDir = DIR_DOWN;
    else if (dy < 0) tailDir = DIR_UP;
  }

  if (_display) {
    int16_t px = tail.x * CELL_PX;
    int16_t py = tail.y * CELL_PX;

    if (tailDir == DIR_RIGHT) {
      _display->fillRect(px + subStep, py, 1, CELL_PX, COLOR_BLACK);
    } else if (tailDir == DIR_LEFT) {
      _display->fillRect(px + (CELL_PX - 1 - subStep), py, 1, CELL_PX, COLOR_BLACK);
    } else if (tailDir == DIR_DOWN) {
      _display->fillRect(px, py + subStep, CELL_PX, 1, COLOR_BLACK);
    } else {
      _display->fillRect(px, py + (CELL_PX - 1 - subStep), CELL_PX, 1, COLOR_BLACK);
    }

    if (subStep == CELL_PX - 1 && _hasTeapot[tail.y][tail.x]) {
      _display->fillRect(px - 1, py - 1, 5, 5, COLOR_BLACK);
      _hasTeapot[tail.y][tail.x] = false;
      if (_teapotCount > 0) _teapotCount--;
    }
  }

  if (subStep == CELL_PX - 1) {
    p.tailIdx = (p.tailIdx + 1) % MAX_PIPE_LEN;
    p.len--;
    _occupied[tail.y][tail.x] = false;

    if (p.headDone && p.len == 0) {
      p.active = false;
    }
  }
}

void PipesClockface::stepPipe(int slot)
{
  PipeHead &p = _pipes[slot];
  if (!p.active) return;

  if (p.headDone) {
    retractTailSlice(p, p.subStep);
    p.subStep++;
    if (p.subStep >= CELL_PX) {
      p.subStep = 0;
    }
    return;
  }

  // First cell: extrude straight inward from the edge
  if (p.stepCount == 0) {
    drawPipeSlice(p.gx, p.gy, p.dir, p.style, p.stepCount, p.subStep);
    p.subStep++;
    if (p.subStep >= CELL_PX) {
      p.subStep = 0;
      p.stepCount++;
    }
    return;
  }

  if (p.subStep == 0) {
    // Allow exiting off-screen once the pipe has navigated across the matrix
    if (p.stepCount >= 6 && isOffScreen(p.gx, p.gy, p.dir) && random(100) < 65) {
      p.headDone = true;
      p.retracting = true;
      retractTailSlice(p, 0);
      p.subStep = 1;
      return;
    }

    bool straightOk = canMove(p.gx, p.gy, p.dir);

    Dir turns[2];
    int turnCount = 0;
    for (int d = 0; d < 4; d++) {
      Dir dir = static_cast<Dir>(d);
      if (dir == p.dir || dir == ((p.dir + 2) & 3)) continue;
      if (canMove(p.gx, p.gy, dir)) {
        turns[turnCount++] = dir;
      }
    }

    bool turned = false;
    if (!straightOk) {
      // If at an edge after traveling, exit off-screen rather than dead-ending
      if (p.stepCount >= 6 && isOffScreen(p.gx, p.gy, p.dir)) {
        p.headDone = true;
        p.retracting = true;
        retractTailSlice(p, 0);
        p.subStep = 1;
        return;
      }
      if (turnCount == 0) {
        drawJoint(p.gx, p.gy, p.style);
        p.headDone = true;
        p.retracting = true;
        return;
      }
      p.dir = turns[random(turnCount)];
      turned = true;
    } else if (turnCount > 0 && random(100) < 28) {
      p.dir = turns[random(turnCount)];
      turned = true;
    }

    if (turned) {
      if (_teapotCount < 1 && p.gx > 1 && p.gx < GRID_SIZE - 2 &&
          p.gy > 1 && p.gy < GRID_SIZE - 2 && random(100) < 4) {
        drawTeapot(p.gx, p.gy);
        _teapotCount++;
      } else {
        drawJoint(p.gx, p.gy, p.style);
      }
    }

    if (p.len >= p.targetLen || p.len >= MAX_PIPE_LEN) {
      p.retracting = true;
    }

    p.gx += DX[p.dir];
    p.gy += DY[p.dir];
    _occupied[p.gy][p.gx] = true;
    _hasTeapot[p.gy][p.gx] = false;

    p.cells[p.headIdx] = {p.gx, p.gy};
    p.headIdx = (p.headIdx + 1) % MAX_PIPE_LEN;
    p.len++;
  }

  drawPipeSlice(p.gx, p.gy, p.dir, p.style, p.stepCount, p.subStep);
  if (p.retracting) {
    retractTailSlice(p, p.subStep);
  }
  p.subStep++;
  if (p.subStep >= CELL_PX) {
    p.subStep = 0;
    p.stepCount++;
  }
}

void PipesClockface::update()
{
  if (!_display) return;

  unsigned long now = millis();
  if (now - _lastStepMillis < STEP_INTERVAL_MS) {
    return;
  }
  _lastStepMillis = now;

  int activeCount = 0;
  for (int i = 0; i < MAX_PIPES; i++) {
    if (_pipes[i].active) {
      stepPipe(i);
      if (_pipes[i].active) activeCount++;
    }
  }

  if (activeCount < MAX_PIPES) {
    for (int i = 0; i < MAX_PIPES; i++) {
      if (!_pipes[i].active) {
        spawnEdgePipe(i);
        break;
      }
    }
  }
}
