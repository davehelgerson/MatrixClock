#include "AquariumClockface.h"
#include <math.h>
#include <new>
#include <string.h>

namespace {
const float PI_F = 3.14159265f;

// Water & sand palette (RGB565)
const uint16_t COLOR_BLACK       = 0x0000;
const uint16_t COLOR_SAND_BASE   = 0x9C8A; // Pale beach sand
const uint16_t COLOR_SAND_LIGHT  = 0xBDD0; // Light sand highlight grain
const uint16_t COLOR_SAND_DARK   = 0x7367; // Sand shadow grain
const uint16_t COLOR_PEBBLE_1    = 0x52AA; // Smooth slate stone
const uint16_t COLOR_PEBBLE_2    = 0x6B4D; // Cool gray pebble
const uint16_t COLOR_BUBBLE      = 0xBFFF; // Bright pale aqua bubble

// Treasure chest palette (deep mahogany + black outline + bright gold so it pops off the sand)
const uint16_t COLOR_CHEST_WOOD  = 0x5880; // Deep reddish-brown mahogany
const uint16_t COLOR_CHEST_DARK  = 0x2000; // Dark wood shadow / lid seam
const uint16_t COLOR_CHEST_GOLD  = 0xFFE0; // Bright metallic gold trim & clasp
const uint16_t COLOR_CHEST_GLOW  = 0xFFFF; // Sparkling highlight inside open chest

// Crab palette
const uint16_t COLOR_CRAB_BODY   = 0xE142; // Bright red-orange shell
const uint16_t COLOR_CRAB_CLAW   = 0xFA42; // Lighter orange claws
const uint16_t COLOR_CRAB_LEG    = 0xA0A1; // Darker legs
const uint16_t COLOR_CRAB_EYE    = 0xFFFF;

// Jellyfish palette
const uint16_t COLOR_JELLY_BELL  = 0xE3BF; // Glowing orchid-pink bell
const uint16_t COLOR_JELLY_RIM   = 0xF81F; // Bright magenta rim
const uint16_t COLOR_JELLY_CORE  = 0xFDDF; // Pale bioluminescent core
const uint16_t COLOR_JELLY_TENT  = 0x8A3B; // Translucent lavender tentacles

// Manta ray palette
const uint16_t COLOR_MANTA_BACK   = 0x3B91; // Deep slate-blue back
const uint16_t COLOR_MANTA_WING   = 0x54B7; // Lighter blue wing edge
const uint16_t COLOR_MANTA_BELLY  = 0xA6DF; // Pale underbelly highlight

// Mini shark palette
const uint16_t COLOR_SHARK_BODY   = 0x6B6F; // Steel-gray shark body
const uint16_t COLOR_SHARK_FIN    = 0x4A6B; // Darker dorsal/tail fin
const uint16_t COLOR_SHARK_BELLY  = 0xCE79; // Pale underbelly

// Tiny submarine palette
const uint16_t COLOR_SUB_HULL     = 0xFFE0; // Bright yellow hull
const uint16_t COLOR_SUB_SHADE    = 0xFC00; // Warm orange-gold keel & tower
const uint16_t COLOR_SUB_WINDOW   = 0x07FF; // Glowing cyan portholes
const uint16_t COLOR_SUB_METAL    = 0xB596; // Silver periscope & propeller

// Time overlay palette
const uint16_t COLOR_TIME_BG     = 0x0000; // Pure black pill for crisp contrast
const uint16_t COLOR_TIME_FG     = 0xFFFF; // Crisp white digits
const uint16_t COLOR_TIME_AMPM   = 0xFFFF; // Crisp white AM/PM

// Compact 3x5 digits (matches ClockfaceCommon)
const uint8_t DIGITS_3X5[10][5] = {
  { 0x7, 0x5, 0x5, 0x5, 0x7 },
  { 0x2, 0x6, 0x2, 0x2, 0x7 },
  { 0x7, 0x1, 0x7, 0x4, 0x7 },
  { 0x7, 0x1, 0x7, 0x1, 0x7 },
  { 0x5, 0x5, 0x7, 0x1, 0x1 },
  { 0x7, 0x4, 0x7, 0x1, 0x7 },
  { 0x7, 0x4, 0x7, 0x5, 0x7 },
  { 0x7, 0x1, 0x1, 0x1, 0x1 },
  { 0x7, 0x5, 0x7, 0x5, 0x7 },
  { 0x7, 0x5, 0x7, 0x1, 0x7 }
};

struct FishColors {
  uint16_t body;
  uint16_t accent;
};

const FishColors FISH_PALETTE[] = {
  { 0xFC20, 0xFFFF }, // Clownfish orange & white
  { 0xFFE0, 0xFC00 }, // Yellow tang & gold
  { 0x07FF, 0xFFE0 }, // Neon cyan & yellow tail
  { 0xF9CE, 0xFDE0 }, // Coral pink & peach
  { 0x3F6A, 0x87FF }, // Reef teal & aqua
  { 0xFD40, 0xF800 }  // Goldfish amber & red
};
const uint8_t FISH_PALETTE_COUNT = sizeof(FISH_PALETTE) / sizeof(FISH_PALETTE[0]);

float randFloat(float lo, float hi)
{
  float u = static_cast<float>(random(0, 10001)) / 10000.0f;
  return lo + u * (hi - lo);
}

float wrapAngle(float a)
{
  while (a > PI_F) a -= 2.0f * PI_F;
  while (a < -PI_F) a += 2.0f * PI_F;
  return a;
}

uint16_t waterGradientColor(int16_t y)
{
  // Gentle, very dim deep-ocean blue gradient so fish & effects stand out
  // Top (y=0): R=0, G=1, B=5 -> Bottom (y=54): R=0, G=0, B=2
  if (y < 0) y = 0;
  if (y > 54) y = 54;
  uint16_t g = (y < 22) ? 1U : 0U;
  uint16_t b = static_cast<uint16_t>(2 + (3 * (54 - y)) / 54);
  return static_cast<uint16_t>((g << 5) | b);
}
}  // namespace

AquariumClockface::AquariumClockface(Adafruit_GFX *display)
  : _display(display)
{
}

void AquariumClockface::setup(CWDateTime *dateTime)
{
  _dateTime = dateTime;
  randomSeed(esp_random());

  if (!_fb) {
    _fb = new (std::nothrow) uint16_t[MATRIX_SIZE * MATRIX_SIZE];
  }
  if (!_prevFb) {
    _prevFb = new (std::nothrow) uint16_t[MATRIX_SIZE * MATRIX_SIZE];
  }
  if (_fb) {
    memset(_fb, 0, MATRIX_SIZE * MATRIX_SIZE * sizeof(uint16_t));
  }
  if (_prevFb) {
    memset(_prevFb, 0, MATRIX_SIZE * MATRIX_SIZE * sizeof(uint16_t));
  }

  _lastFrameMillis = 0;
  _animTime = 0.0f;
  _chestOpenTimer = 0.0f;
  _chestBubbleSpawnTimer = 0.0f;
  _subBubbleSpawnTimer = 0.0f;
  _lastMinute = -1;
  _lastHour = -1;
  _subPending = false;
  _forceFullFlush = true;
  unsigned long now = millis();
  _event.type = EVENT_NONE;
  // Stagger crab & jellyfish by 1 min so they alternate every 2 min each
  _lastCrabMillis  = now - 90000UL;
  _lastJellyMillis = now - 30000UL;
  _lastMantaMillis = now;
  _lastSharkMillis = now;

  initScene();

  if (_display) {
    _display->fillRect(0, 0, MATRIX_SIZE, MATRIX_SIZE, COLOR_BLACK);
  }
  update();
}

void AquariumClockface::teardown()
{
  delete[] _fb;
  _fb = nullptr;
  delete[] _prevFb;
  _prevFb = nullptr;
  if (_display) {
    _display->fillRect(0, 0, MATRIX_SIZE, MATRIX_SIZE, COLOR_BLACK);
  }
}

inline void AquariumClockface::setFbPixel(int16_t x, int16_t y, uint16_t color)
{
  if (x >= 0 && x < MATRIX_SIZE && y >= 0 && y < MATRIX_SIZE) {
    _fb[y * MATRIX_SIZE + x] = color;
  }
}

void AquariumClockface::initScene()
{
  // Gentle undulating sand dune contour across bottom (y = 51..54) so sand sits above the time
  for (int x = 0; x < MATRIX_SIZE; ++x) {
    float wave = sinf(static_cast<float>(x) * 0.12f) * 1.2f +
                 cosf(static_cast<float>(x) * 0.05f + 1.0f) * 0.6f;
    int y = 52 + static_cast<int>(roundf(wave));
    if (y < 51) y = 51;
    if (y > 54) y = 54;
    _sandTop[x] = static_cast<int8_t>(y);
  }

  // Seaweed stalks rooted across the tank (leaving clearing around chest at x=46)
  const int8_t weedX[WEED_COUNT]   = { 5, 16, 35, 56, 61 };
  const uint8_t weedH[WEED_COUNT]  = { 21, 16, 24, 18, 25 };
  const uint16_t weedC[WEED_COUNT] = { 0x04A2, 0x03E2, 0x0523, 0x0422, 0x04C3 };
  const uint16_t weedT[WEED_COUNT] = { 0x06C4, 0x05E3, 0x0725, 0x0643, 0x06E4 };
  for (uint8_t i = 0; i < WEED_COUNT; ++i) {
    _weeds[i].baseX    = weedX[i];
    _weeds[i].height   = weedH[i];
    _weeds[i].phase    = randFloat(0.0f, 2.0f * PI_F);
    _weeds[i].speed    = randFloat(1.3f, 2.2f);
    _weeds[i].color    = weedC[i];
    _weeds[i].tipColor = weedT[i];
  }

  // Rising ambient air bubbles
  for (uint8_t i = 0; i < BUBBLE_COUNT; ++i) {
    _bubbles[i].x = randFloat(6.0f, 58.0f);
    _bubbles[i].y = randFloat(8.0f, 50.0f);
    _bubbles[i].speed = randFloat(4.5f, 9.0f);
    _bubbles[i].wobblePhase = randFloat(0.0f, 2.0f * PI_F);
    _bubbles[i].active = true;
  }

  // Treasure chest & submarine bubbles (inactive until spawned)
  for (uint8_t i = 0; i < CHEST_BUBBLE_COUNT; ++i) {
    _chestBubbles[i].active = false;
  }
  for (uint8_t i = 0; i < SUB_BUBBLE_COUNT; ++i) {
    _subBubbles[i].active = false;
  }

  // 5 schooling fish: 1 slightly bigger centerpiece fish + 4 smaller fish
  float schoolAngle = randFloat(-0.5f, 0.5f);
  if (random(2) == 0) schoolAngle += PI_F;

  for (uint8_t i = 0; i < FISH_COUNT; ++i) {
    Fish &f = _fish[i];
    f.big = (i == 0);
    bool slowSmall = (!f.big && i == FISH_COUNT - 1);
    f.x = randFloat(12.0f, 52.0f);
    f.y = randFloat(10.0f, 41.0f);
    if (f.big) {
      f.minSpeed = randFloat(4.8f, 6.8f);
      f.maxSpeed = f.minSpeed + randFloat(4.5f, 7.5f);
      f.maxTurnRate = randFloat(1.5f, 2.4f);
    } else if (slowSmall) {
      f.minSpeed = randFloat(4.2f, 5.5f);
      f.maxSpeed = f.minSpeed + randFloat(3.5f, 5.5f);
      f.maxTurnRate = randFloat(1.8f, 2.8f);
    } else {
      f.minSpeed = randFloat(6.0f, 8.8f);
      f.maxSpeed = f.minSpeed + randFloat(6.0f, 11.0f);
      f.maxTurnRate = randFloat(2.0f, 3.5f);
    }
    float initSpeed = randFloat(f.minSpeed, f.maxSpeed * 0.85f);
    float heading = schoolAngle + randFloat(-0.6f, 0.6f);
    f.vx = cosf(heading) * initSpeed;
    f.vy = sinf(heading) * initSpeed;
    f.wanderAngle = heading;
    f.idleTimer = randFloat(2.0f, 7.0f);
    f.idling = (random(0, 4) == 0);

    const FishColors &pal = FISH_PALETTE[i % FISH_PALETTE_COUNT];
    f.bodyColor = pal.body;
    f.accentColor = pal.accent;
    f.tailPhase = static_cast<uint8_t>(random(0, 16));
  }
}

void AquariumClockface::updateBoids(float dt)
{
  const float sepRadius   = 7.5f;
  const float alignRadius = 17.0f;
  const float cohRadius   = 19.0f;

  float nextVx[FISH_COUNT];
  float nextVy[FISH_COUNT];

  for (uint8_t i = 0; i < FISH_COUNT; ++i) {
    Fish &f = _fish[i];

    // Periodically toggle between active schooling and gentle idle wandering
    f.idleTimer -= dt;
    if (f.idleTimer <= 0.0f) {
      f.idling = !f.idling;
      f.idleTimer = f.idling ? randFloat(2.0f, 4.5f) : randFloat(4.5f, 9.5f);
    }

    float sepX = 0.0f, sepY = 0.0f;
    float aliX = 0.0f, aliY = 0.0f;
    float cohX = 0.0f, cohY = 0.0f;
    int aliCount = 0;
    int cohCount = 0;

    for (uint8_t j = 0; j < FISH_COUNT; ++j) {
      if (i == j) continue;
      float dx = _fish[j].x - f.x;
      float dy = _fish[j].y - f.y;
      float distSq = dx * dx + dy * dy;
      if (distSq < 0.01f) continue;

      if (distSq < sepRadius * sepRadius) {
        float invDist = 1.0f / sqrtf(distSq);
        float push = (sepRadius - (1.0f / invDist)) * invDist;
        sepX -= dx * push;
        sepY -= dy * push;
      }
      if (distSq < alignRadius * alignRadius) {
        aliX += _fish[j].vx;
        aliY += _fish[j].vy;
        ++aliCount;
      }
      if (distSq < cohRadius * cohRadius) {
        cohX += _fish[j].x;
        cohY += _fish[j].y;
        ++cohCount;
      }
    }

    // Reynolds-style randomized wandering steering
    f.wanderAngle = wrapAngle(f.wanderAngle + randFloat(-2.6f, 2.6f) * dt);
    float steerX = cosf(f.wanderAngle) * (f.idling ? 7.5f : 3.2f);
    float steerY = sinf(f.wanderAngle) * (f.idling ? 5.5f : 2.2f);

    // Combine boids forces (softened when idling so fish peel off & rejoin naturally)
    float flockScale = f.idling ? 0.35f : 1.0f;
    steerX += sepX * 16.0f;
    steerY += sepY * 16.0f;

    if (aliCount > 0) {
      aliX /= static_cast<float>(aliCount);
      aliY /= static_cast<float>(aliCount);
      steerX += (aliX - f.vx) * 1.6f * flockScale;
      steerY += (aliY - f.vy) * 1.6f * flockScale;
    }
    if (cohCount > 0) {
      cohX = (cohX / static_cast<float>(cohCount)) - f.x;
      cohY = (cohY / static_cast<float>(cohCount)) - f.y;
      steerX += cohX * 0.9f * flockScale;
      steerY += cohY * 0.9f * flockScale;
    }

    // Soft tank boundary avoidance (keep fish in open water above sand)
    if (f.x < 7.0f)  steerX += (7.0f - f.x) * 5.5f;
    if (f.x > 56.0f) steerX -= (f.x - 56.0f) * 5.5f;
    if (f.y < 6.0f)  steerY += (6.0f - f.y) * 6.0f;
    if (f.y > 46.0f) steerY -= (f.y - 46.0f) * 6.5f;

    // Strong avoidance of shark and submarine (plus gentler avoidance of jellyfish)
    bool fleeing = false;
    if (_event.type == EVENT_SHARK || _event.type == EVENT_SUBMARINE) {
      // Shift detection center slightly ahead of the shark/sub nose so fish dodge early
      float threatX = _event.x + static_cast<float>(_event.dir) * 4.0f;
      float threatY = _event.y;
      float edx = f.x - threatX;
      float edy = f.y - threatY;
      float avoidRx = (_event.type == EVENT_SHARK) ? 24.0f : 22.0f;
      float avoidRy = (_event.type == EVENT_SHARK) ? 16.0f : 15.0f;
      float normSq = (edx * edx) / (avoidRx * avoidRx) + (edy * edy) / (avoidRy * avoidRy);

      if (normSq < 1.0f) {
        fleeing = true;
        f.idling = false;
        float dist = sqrtf(edx * edx + edy * edy);
        float nx = 0.0f;
        float ny = 0.0f;
        if (dist > 0.2f) {
          nx = edx / dist;
          ny = edy / dist;
        } else {
          // Directly on centerline: dodge vertically toward whichever half has more room
          ny = (f.y < 26.0f) ? -1.0f : 1.0f;
        }
        // Bias vertical dodge so fish split cleanly above/below the oncoming path
        ny *= 1.45f;
        float nLen = sqrtf(nx * nx + ny * ny);
        if (nLen > 0.01f) {
          nx /= nLen;
          ny /= nLen;
        }
        float urgency = 1.0f - sqrtf(normSq);
        float fleeForce = (_event.type == EVENT_SHARK ? 160.0f : 140.0f) * (0.45f + urgency);
        steerX = nx * fleeForce;
        steerY = ny * fleeForce;
      }
    } else if (_event.type == EVENT_JELLYFISH) {
      float jdx = f.x - _event.x;
      float jdy = f.y - _event.y;
      float jDistSq = jdx * jdx + jdy * jdy;
      if (jDistSq > 0.1f && jDistSq < 144.0f) {
        float jDist = sqrtf(jDistSq);
        float repel = (12.0f - jDist) * 2.4f;
        steerX += (jdx / jDist) * repel;
        steerY += (jdy / jDist) * repel;
      }
    }

    // Apply steering with per-fish turning-angle rate limit (boosted when dodging shark/sub)
    float curAngle = atan2f(f.vy, f.vx);
    float curSpeed = sqrtf(f.vx * f.vx + f.vy * f.vy);

    float desiredVx = f.vx + steerX * dt;
    float desiredVy = f.vy + steerY * dt;
    float desiredAngle = atan2f(desiredVy, desiredVx);
    float desiredSpeed = sqrtf(desiredVx * desiredVx + desiredVy * desiredVy);

    float deltaAngle = wrapAngle(desiredAngle - curAngle);
    float turnRate = fleeing ? (f.maxTurnRate * 4.0f) : f.maxTurnRate;
    float maxDelta = turnRate * dt;
    if (deltaAngle > maxDelta) deltaAngle = maxDelta;
    if (deltaAngle < -maxDelta) deltaAngle = -maxDelta;
    float newAngle = wrapAngle(curAngle + deltaAngle);

    float targetMin = fleeing ? (f.maxSpeed * 1.15f) : (f.idling ? (f.minSpeed * 0.55f) : f.minSpeed);
    float targetMax = fleeing ? (f.maxSpeed * 2.0f)  : (f.idling ? (f.minSpeed * 1.05f) : f.maxSpeed);
    float speedBlend = fleeing ? (10.0f * dt) : (3.0f * dt);
    float newSpeed = curSpeed + (desiredSpeed - curSpeed) * speedBlend;
    if (newSpeed < targetMin) newSpeed = targetMin;
    if (newSpeed > targetMax) newSpeed = targetMax;

    nextVx[i] = cosf(newAngle) * newSpeed;
    nextVy[i] = sinf(newAngle) * newSpeed;
  }

  for (uint8_t i = 0; i < FISH_COUNT; ++i) {
    Fish &f = _fish[i];
    f.vx = nextVx[i];
    f.vy = nextVy[i];
    f.x += f.vx * dt;
    f.y += f.vy * dt;

    // Keep fish out of the shark/submarine hull footprint
    if (_event.type == EVENT_SHARK || _event.type == EVENT_SUBMARINE) {
      float dx = f.x - _event.x;
      float dy = f.y - _event.y;
      const float hullRx = 11.0f;
      const float hullRy = 6.5f;
      float hNormSq = (dx * dx) / (hullRx * hullRx) + (dy * dy) / (hullRy * hullRy);
      if (hNormSq < 1.0f) {
        if (dy == 0.0f) dy = (f.y < 26.0f) ? -1.0f : 1.0f;
        float pushSign = (dy < 0.0f) ? -1.0f : 1.0f;
        f.y = _event.y + pushSign * hullRy;
        f.vy = pushSign * fabsf(f.vy);
      }
    }

    if (f.x < 2.0f)  { f.x = 2.0f;  f.vx = fabsf(f.vx); }
    if (f.x > 61.0f) { f.x = 61.0f; f.vx = -fabsf(f.vx); }
    if (f.y < 4.0f)  { f.y = 4.0f;  f.vy = fabsf(f.vy); }
    if (f.y > 49.0f) { f.y = 49.0f; f.vy = -fabsf(f.vy); }
  }
}

void AquariumClockface::updateBubbles(float dt)
{
  for (uint8_t i = 0; i < BUBBLE_COUNT; ++i) {
    Bubble &b = _bubbles[i];
    b.y -= b.speed * dt;
    b.wobblePhase += 3.2f * dt;
    if (b.y < 2.0f) {
      b.x = randFloat(5.0f, 59.0f);
      b.y = static_cast<float>(_sandTop[static_cast<int>(b.x)]) - 1.0f;
      b.speed = randFloat(4.5f, 9.0f);
    }
  }

  // Update bubbles rising out of the treasure chest
  for (uint8_t i = 0; i < CHEST_BUBBLE_COUNT; ++i) {
    Bubble &b = _chestBubbles[i];
    if (!b.active) continue;
    b.y -= b.speed * dt;
    b.wobblePhase += 4.2f * dt;
    if (b.y < 1.0f) {
      b.active = false;
    }
  }

  // Update propeller trail bubbles from the tiny submarine
  for (uint8_t i = 0; i < SUB_BUBBLE_COUNT; ++i) {
    Bubble &b = _subBubbles[i];
    if (!b.active) continue;
    b.y -= b.speed * dt;
    b.wobblePhase += 3.8f * dt;
    if (b.y < 1.0f) {
      b.active = false;
    }
  }
}

void AquariumClockface::updateChest(float dt, int minute)
{
  if (_lastMinute == -1) {
    _lastMinute = minute;
  } else if (minute != _lastMinute) {
    // Minute rollover: pop open the treasure chest and stream bubbles out
    _lastMinute = minute;
    _chestOpenTimer = 4.2f;
    _chestBubbleSpawnTimer = 0.0f;
  }

  if (_chestOpenTimer > 0.0f) {
    _chestOpenTimer -= dt;
    if (_chestOpenTimer < 0.0f) _chestOpenTimer = 0.0f;

    _chestBubbleSpawnTimer -= dt;
    if (_chestBubbleSpawnTimer <= 0.0f) {
      _chestBubbleSpawnTimer = randFloat(0.18f, 0.34f);
      for (uint8_t i = 0; i < CHEST_BUBBLE_COUNT; ++i) {
        if (!_chestBubbles[i].active) {
          _chestBubbles[i].active = true;
          _chestBubbles[i].x = static_cast<float>(CHEST_X) + randFloat(-2.5f, 2.5f);
          _chestBubbles[i].y = static_cast<float>(_sandTop[CHEST_X]) - 4.0f;
          _chestBubbles[i].speed = randFloat(11.0f, 17.5f);
          _chestBubbles[i].wobblePhase = randFloat(0.0f, 2.0f * PI_F);
          break;
        }
      }
    }
  }
}

void AquariumClockface::spawnSubmarine()
{
  _event.type = EVENT_SUBMARINE;
  _event.phase = 0.0f;
  _event.pauseTimer = 0.0f;
  _event.dir = (random(0, 2) == 0) ? 1 : -1;
  _event.x = (_event.dir > 0) ? -11.0f : 74.0f;
  _event.y = randFloat(16.0f, 32.0f);
  _event.vx = static_cast<float>(_event.dir) * 8.0f;
  _event.vy = 0.0f;
  _subBubbleSpawnTimer = 0.0f;
}

void AquariumClockface::spawnManta()
{
  _event.type = EVENT_MANTA;
  _event.phase = 0.0f;
  _event.pauseTimer = 0.0f;
  _event.dir = (random(0, 2) == 0) ? 1 : -1;
  _event.x = (_event.dir > 0) ? -10.0f : 73.0f;
  _event.y = randFloat(14.0f, 32.0f);
  _event.vx = static_cast<float>(_event.dir) * randFloat(6.0f, 8.5f);
  _event.vy = 0.0f;
}

void AquariumClockface::spawnShark()
{
  _event.type = EVENT_SHARK;
  _event.phase = 0.0f;
  _event.pauseTimer = 0.0f;
  _event.dir = (random(0, 2) == 0) ? 1 : -1;
  _event.x = (_event.dir > 0) ? -10.0f : 73.0f;
  _event.y = randFloat(14.0f, 35.0f);
  _event.vx = static_cast<float>(_event.dir) * randFloat(9.5f, 13.0f);
  _event.vy = 0.0f;
}

void AquariumClockface::spawnCrab()
{
  _event.type = EVENT_CRAB;
  _event.phase = 0.0f;
  _event.pauseTimer = 0.0f;
  _event.dir = (random(0, 2) == 0) ? 1 : -1;
  _event.x = (_event.dir > 0) ? -5.0f : 68.0f;
  _event.y = 53.0f;
  _event.vx = static_cast<float>(_event.dir) * randFloat(5.5f, 8.5f);
  _event.vy = 0.0f;
}

void AquariumClockface::spawnJellyfish()
{
  _event.type = EVENT_JELLYFISH;
  _event.phase = 0.0f;
  _event.pauseTimer = 0.0f;
  _event.dir = (random(0, 2) == 0) ? 1 : -1;
  _event.x = (_event.dir > 0) ? -6.0f : 69.0f;
  _event.y = randFloat(15.0f, 34.0f);
  _event.vx = static_cast<float>(_event.dir) * randFloat(3.8f, 6.2f);
  _event.vy = 0.0f;
}

void AquariumClockface::updateBgEvent(float dt, unsigned long now, int hour)
{
  if (_lastHour == -1) {
    _lastHour = hour;
  } else if (hour != _lastHour) {
    _lastHour = hour;
    _subPending = true;
  }

  if (_event.type == EVENT_NONE) {
    if (_subPending) {
      _subPending = false;
      spawnSubmarine();
    } else if ((now - _lastSharkMillis) >= SHARK_INTERVAL_MS) {
      _lastSharkMillis = now;
      spawnShark();
    } else if ((now - _lastMantaMillis) >= MANTA_INTERVAL_MS) {
      _lastMantaMillis = now;
      spawnManta();
    } else if ((now - _lastCrabMillis) >= CRAB_INTERVAL_MS) {
      _lastCrabMillis = now;
      spawnCrab();
    } else if ((now - _lastJellyMillis) >= JELLY_INTERVAL_MS) {
      _lastJellyMillis = now;
      spawnJellyfish();
    }
    return;
  }

  _event.phase += dt;

  if (_event.type == EVENT_CRAB) {
    // Occasional brief pause to wave claws
    if (_event.pauseTimer > 0.0f) {
      _event.pauseTimer -= dt;
    } else {
      _event.x += _event.vx * dt;
      if (random(0, 140) == 0 && _event.x > 12.0f && _event.x < 54.0f) {
        _event.pauseTimer = randFloat(0.6f, 1.3f);
      }
    }
    int sx = static_cast<int>(roundf(_event.x));
    if (sx < 0) sx = 0;
    if (sx >= MATRIX_SIZE) sx = MATRIX_SIZE - 1;
    _event.y = static_cast<float>(_sandTop[sx]) - 1.0f;

    if ((_event.dir > 0 && _event.x > 70.0f) || (_event.dir < 0 && _event.x < -7.0f)) {
      _event.type = EVENT_NONE;
    }
  } else if (_event.type == EVENT_JELLYFISH) {
    // Rhythmic bell-pulse propulsion + gentle vertical bob
    float pulse = sinf(_event.phase * 2.8f);
    float boost = (pulse > 0.0f) ? (1.0f + pulse * 0.65f) : 0.75f;
    _event.x += _event.vx * boost * dt;
    _event.y += (-pulse * 3.5f + sinf(_event.phase * 0.7f) * 1.5f) * dt;
    if (_event.y < 10.0f) _event.y = 10.0f;
    if (_event.y > 38.0f) _event.y = 38.0f;

    if ((_event.dir > 0 && _event.x > 71.0f) || (_event.dir < 0 && _event.x < -8.0f)) {
      _event.type = EVENT_NONE;
    }
  } else if (_event.type == EVENT_MANTA) {
    // Graceful undulating wing glide
    _event.x += _event.vx * dt;
    _event.y += sinf(_event.phase * 2.2f) * 2.6f * dt;
    if (_event.y < 12.0f) _event.y = 12.0f;
    if (_event.y > 35.0f) _event.y = 35.0f;

    if ((_event.dir > 0 && _event.x > 76.0f) || (_event.dir < 0 && _event.x < -13.0f)) {
      _event.type = EVENT_NONE;
    }
  } else if (_event.type == EVENT_SHARK) {
    // Brisk, steady predator cruise
    _event.x += _event.vx * dt;
    _event.y += sinf(_event.phase * 1.6f) * 1.4f * dt;
    if (_event.y < 11.0f) _event.y = 11.0f;
    if (_event.y > 37.0f) _event.y = 37.0f;

    if ((_event.dir > 0 && _event.x > 76.0f) || (_event.dir < 0 && _event.x < -13.0f)) {
      _event.type = EVENT_NONE;
    }
  } else if (_event.type == EVENT_SUBMARINE) {
    // Steady cruise with gentle bob and trailing propeller bubbles
    _event.x += _event.vx * dt;
    _event.y += sinf(_event.phase * 1.8f) * 1.0f * dt;

    _subBubbleSpawnTimer -= dt;
    if (_subBubbleSpawnTimer <= 0.0f && _event.x > 0.0f && _event.x < 63.0f) {
      _subBubbleSpawnTimer = 0.25f;
      for (uint8_t i = 0; i < SUB_BUBBLE_COUNT; ++i) {
        if (!_subBubbles[i].active) {
          _subBubbles[i].active = true;
          _subBubbles[i].x = _event.x - static_cast<float>(_event.dir) * 9.0f;
          _subBubbles[i].y = _event.y + randFloat(-1.0f, 1.0f);
          _subBubbles[i].speed = randFloat(7.0f, 11.0f);
          _subBubbles[i].wobblePhase = randFloat(0.0f, 2.0f * PI_F);
          break;
        }
      }
    }

    if ((_event.dir > 0 && _event.x > 78.0f) || (_event.dir < 0 && _event.x < -15.0f)) {
      _event.type = EVENT_NONE;
    }
  }
}

void AquariumClockface::renderWaterAndSand()
{
  // Fill water background with dim deep-ocean gradient
  for (int16_t y = 0; y < MATRIX_SIZE; ++y) {
    uint16_t wCol = waterGradientColor(y);
    int row = y * MATRIX_SIZE;
    for (int16_t x = 0; x < MATRIX_SIZE; ++x) {
      _fb[row + x] = wCol;
    }
  }

  // Sandy bottom with deterministic grain texture
  for (int16_t x = 0; x < MATRIX_SIZE; ++x) {
    int16_t top = _sandTop[x];
    for (int16_t y = top; y < MATRIX_SIZE; ++y) {
      uint8_t h = static_cast<uint8_t>((x * 29 + y * 13) & 0x07);
      uint16_t col = COLOR_SAND_BASE;
      if (y == top) {
        col = COLOR_SAND_LIGHT;
      } else if (h == 0) {
        col = COLOR_SAND_LIGHT;
      } else if (h == 1 || h == 2) {
        col = COLOR_SAND_DARK;
      }
      _fb[y * MATRIX_SIZE + x] = col;
    }
  }

  // A few small decorative pebbles on the right side of the seabed
  setFbPixel(37, _sandTop[37] + 2, COLOR_PEBBLE_1);
  setFbPixel(38, _sandTop[38] + 2, COLOR_PEBBLE_1);
  setFbPixel(46, _sandTop[46] + 3, COLOR_PEBBLE_2);
  setFbPixel(47, _sandTop[47] + 3, COLOR_PEBBLE_2);
  setFbPixel(55, _sandTop[55] + 2, COLOR_PEBBLE_1);
}

void AquariumClockface::renderSeaweed()
{
  for (uint8_t i = 0; i < WEED_COUNT; ++i) {
    const Seaweed &w = _weeds[i];
    int16_t baseY = _sandTop[w.baseX];
    for (uint8_t seg = 0; seg < w.height; ++seg) {
      float frac = static_cast<float>(seg) / static_cast<float>(w.height);
      float sway = sinf(_animTime * w.speed + w.phase - static_cast<float>(seg) * 0.28f) * (2.6f * frac);
      int16_t x = w.baseX + static_cast<int16_t>(roundf(sway));
      int16_t y = baseY - static_cast<int16_t>(seg);
      uint16_t col = (seg >= w.height - 3) ? w.tipColor : w.color;
      setFbPixel(x, y, col);
      // Slight frond thickness near lower/mid stalk
      if (seg > 1 && seg < w.height - 4 && (seg % 3 == 0)) {
        setFbPixel(x + ((seg & 1) ? 1 : -1), y, w.color);
      }
    }
  }
}

void AquariumClockface::renderTreasureChest()
{
  int16_t cx = CHEST_X;
  int16_t baseY = _sandTop[cx] + 1; // Nestled slightly into the sand so it feels grounded
  bool isOpen = (_chestOpenTimer > 0.0f);

  // Chest base (9px wide x 4px tall: x = cx-4..cx+4, y = baseY-3..baseY)
  for (int16_t y = baseY - 3; y <= baseY; ++y) {
    for (int16_t x = cx - 4; x <= cx + 4; ++x) {
      uint16_t col = COLOR_CHEST_WOOD;
      if (x == cx - 4 || x == cx + 4) {
        col = COLOR_CHEST_GOLD;
      } else if (y == baseY - 3 || y == baseY) {
        col = COLOR_CHEST_DARK;
      }
      setFbPixel(x, y, col);
    }
  }
  // Center gold clasp
  setFbPixel(cx, baseY - 2, COLOR_CHEST_GOLD);
  // Subtle sand grains tucked along the bottom corners to ground the chest in the seabed
  setFbPixel(cx - 4, baseY, COLOR_SAND_DARK);
  setFbPixel(cx + 4, baseY, COLOR_SAND_DARK);

  if (isOpen) {
    // Glowing golden treasure mound visible inside open chest
    bool sparkle = (static_cast<int>(_animTime * 8.0f) & 1) != 0;
    for (int16_t x = cx - 3; x <= cx + 3; ++x) {
      setFbPixel(x, baseY - 3, ((x + (sparkle ? 1 : 0)) & 1) ? COLOR_CHEST_GLOW : COLOR_CHEST_GOLD);
    }
    setFbPixel(cx - 1, baseY - 4, COLOR_CHEST_GOLD);
    setFbPixel(cx,     baseY - 4, COLOR_CHEST_GLOW);
    setFbPixel(cx + 1, baseY - 4, COLOR_CHEST_GOLD);

    // Flipped-open lid raised above the back of the chest
    for (int16_t x = cx - 4; x <= cx + 4; ++x) {
      uint16_t trim = (x == cx - 4 || x == cx + 4) ? COLOR_CHEST_GOLD : COLOR_CHEST_WOOD;
      setFbPixel(x, baseY - 6, trim);
      setFbPixel(x, baseY - 7, COLOR_CHEST_DARK);
    }
  } else {
    // Closed rounded lid (x = cx-4..cx+4, y = baseY-5..baseY-4)
    for (int16_t x = cx - 4; x <= cx + 4; ++x) {
      uint16_t col = (x == cx - 4 || x == cx + 4 || x == cx) ? COLOR_CHEST_GOLD : COLOR_CHEST_WOOD;
      setFbPixel(x, baseY - 4, col);
    }
    for (int16_t x = cx - 3; x <= cx + 3; ++x) {
      uint16_t col = (x == cx - 3 || x == cx + 3) ? COLOR_CHEST_GOLD : COLOR_CHEST_WOOD;
      setFbPixel(x, baseY - 5, col);
    }
  }
}

void AquariumClockface::renderBubbles()
{
  for (uint8_t i = 0; i < BUBBLE_COUNT; ++i) {
    const Bubble &b = _bubbles[i];
    int16_t bx = static_cast<int16_t>(roundf(b.x + sinf(b.wobblePhase) * 1.1f));
    int16_t by = static_cast<int16_t>(roundf(b.y));
    setFbPixel(bx, by, COLOR_BUBBLE);
  }

  for (uint8_t i = 0; i < CHEST_BUBBLE_COUNT; ++i) {
    const Bubble &b = _chestBubbles[i];
    if (!b.active) continue;
    int16_t bx = static_cast<int16_t>(roundf(b.x + sinf(b.wobblePhase) * 1.4f));
    int16_t by = static_cast<int16_t>(roundf(b.y));
    setFbPixel(bx, by, COLOR_BUBBLE);
    // Slight 2px highlight on larger chest bubbles
    if ((i & 1) == 0) {
      setFbPixel(bx + 1, by, COLOR_TIME_FG);
    }
  }

  for (uint8_t i = 0; i < SUB_BUBBLE_COUNT; ++i) {
    const Bubble &b = _subBubbles[i];
    if (!b.active) continue;
    int16_t bx = static_cast<int16_t>(roundf(b.x + sinf(b.wobblePhase) * 0.9f));
    int16_t by = static_cast<int16_t>(roundf(b.y));
    setFbPixel(bx, by, COLOR_BUBBLE);
  }
}

void AquariumClockface::renderFish()
{
  for (uint8_t i = 0; i < FISH_COUNT; ++i) {
    const Fish &f = _fish[i];
    int16_t cx = static_cast<int16_t>(roundf(f.x));
    int16_t cy = static_cast<int16_t>(roundf(f.y));
    int8_t dir = (f.vx >= 0.0f) ? 1 : -1;

    // Animated wagging tail offset (-1, 0, +1)
    uint8_t wagTick = static_cast<uint8_t>(static_cast<uint32_t>(_animTime * 10.0f) + f.tailPhase);
    int8_t wag = ((wagTick & 3U) == 1U) ? -1 : (((wagTick & 3U) == 3U) ? 1 : 0);

    if (f.big) {
      // Larger 7x5 tropical fish silhouette with dorsal/ventral fins & forked tail
      setFbPixel(cx + dir * 3, cy,     f.bodyColor);   // Nose
      setFbPixel(cx + dir * 2, cy - 1, f.bodyColor);   // Forehead
      setFbPixel(cx + dir * 2, cy,     COLOR_BLACK);   // Eye
      setFbPixel(cx + dir * 2, cy + 1, f.bodyColor);   // Chin
      setFbPixel(cx + dir,     cy - 2, f.accentColor); // Dorsal fin front
      setFbPixel(cx + dir,     cy - 1, f.bodyColor);
      setFbPixel(cx + dir,     cy,     f.bodyColor);
      setFbPixel(cx + dir,     cy + 1, f.bodyColor);
      setFbPixel(cx + dir,     cy + 2, f.accentColor); // Ventral fin
      setFbPixel(cx,           cy - 2, f.accentColor); // Dorsal fin mid
      setFbPixel(cx,           cy - 1, f.accentColor); // Body stripe
      setFbPixel(cx,           cy,     f.accentColor);
      setFbPixel(cx,           cy + 1, f.accentColor);
      setFbPixel(cx - dir,     cy - 1, f.bodyColor);   // Rear body
      setFbPixel(cx - dir,     cy,     f.bodyColor);
      setFbPixel(cx - dir,     cy + 1, f.bodyColor);
      setFbPixel(cx - dir * 2, cy,     f.accentColor); // Tail base
      setFbPixel(cx - dir * 3, cy - 2 + wag, f.accentColor); // Upper tail tip
      setFbPixel(cx - dir * 3, cy - 1 + wag, f.bodyColor);
      setFbPixel(cx - dir * 3, cy + 1 + wag, f.bodyColor);
      setFbPixel(cx - dir * 3, cy + 2 + wag, f.accentColor); // Lower tail tip
    } else {
      // Compact 5x3 tropical fish silhouette oriented by swim direction
      setFbPixel(cx + dir * 2, cy,     f.bodyColor);   // Nose
      setFbPixel(cx + dir,     cy - 1, f.bodyColor);   // Upper front
      setFbPixel(cx + dir,     cy,     COLOR_BLACK);   // Eye
      setFbPixel(cx + dir,     cy + 1, f.bodyColor);   // Lower front
      setFbPixel(cx,           cy - 1, f.accentColor); // Dorsal stripe
      setFbPixel(cx,           cy,     f.bodyColor);   // Center body
      setFbPixel(cx,           cy + 1, f.bodyColor);   // Belly
      setFbPixel(cx - dir,     cy,     f.accentColor); // Tail base
      setFbPixel(cx - dir * 2, cy - 1 + wag, f.accentColor); // Upper tail fin
      setFbPixel(cx - dir * 2, cy + 1 + wag, f.accentColor); // Lower tail fin
    }
  }
}

void AquariumClockface::renderBgEvent()
{
  if (_event.type == EVENT_CRAB) {
    int16_t cx = static_cast<int16_t>(roundf(_event.x));
    int16_t cy = static_cast<int16_t>(roundf(_event.y));
    bool legStep = (static_cast<int>(_event.phase * 8.0f) & 1) != 0;
    bool clawWave = (_event.pauseTimer > 0.0f) && ((static_cast<int>(_event.phase * 10.0f) & 1) != 0);

    // 5x2 rounded crab shell
    for (int16_t dx = -2; dx <= 2; ++dx) {
      setFbPixel(cx + dx, cy - 1, COLOR_CRAB_BODY);
      setFbPixel(cx + dx, cy,     COLOR_CRAB_BODY);
    }
    // Stalk eyes
    setFbPixel(cx - 1, cy - 2, COLOR_CRAB_EYE);
    setFbPixel(cx + 1, cy - 2, COLOR_CRAB_EYE);

    // Raised claws (wave up when paused)
    int16_t clawY = clawWave ? (cy - 3) : (cy - 2);
    setFbPixel(cx - 3, cy - 1, COLOR_CRAB_CLAW);
    setFbPixel(cx - 3, clawY,  COLOR_CRAB_CLAW);
    setFbPixel(cx + 3, cy - 1, COLOR_CRAB_CLAW);
    setFbPixel(cx + 3, clawY,  COLOR_CRAB_CLAW);

    // Scuttling legs
    setFbPixel(cx - 3, cy + (legStep ? 1 : 0), COLOR_CRAB_LEG);
    setFbPixel(cx - 1, cy + (legStep ? 0 : 1), COLOR_CRAB_LEG);
    setFbPixel(cx + 1, cy + (legStep ? 1 : 0), COLOR_CRAB_LEG);
    setFbPixel(cx + 3, cy + (legStep ? 0 : 1), COLOR_CRAB_LEG);
  } else if (_event.type == EVENT_JELLYFISH) {
    int16_t cx = static_cast<int16_t>(roundf(_event.x));
    int16_t cy = static_cast<int16_t>(roundf(_event.y));
    bool contracted = sinf(_event.phase * 2.8f) > 0.25f;

    // Bioluminescent dome bell
    int16_t rimHalf = contracted ? 2 : 3;
    for (int16_t dx = -1; dx <= 1; ++dx) {
      setFbPixel(cx + dx, cy - 2, COLOR_JELLY_BELL);
    }
    for (int16_t dx = -2; dx <= 2; ++dx) {
      setFbPixel(cx + dx, cy - 1, (dx == 0) ? COLOR_JELLY_CORE : COLOR_JELLY_BELL);
    }
    for (int16_t dx = -rimHalf; dx <= rimHalf; ++dx) {
      setFbPixel(cx + dx, cy, COLOR_JELLY_RIM);
    }

    // 3 undulating trailing tentacles
    for (int16_t t = -1; t <= 1; ++t) {
      int16_t baseX = cx + t * 2;
      for (int16_t seg = 1; seg <= 5; ++seg) {
        float wave = sinf(_event.phase * 4.5f - static_cast<float>(seg) * 0.7f + static_cast<float>(t));
        int16_t tx = baseX - _event.dir * (seg / 2) + static_cast<int16_t>(roundf(wave * 0.9f));
        setFbPixel(tx, cy + seg, COLOR_JELLY_TENT);
      }
    }
  } else if (_event.type == EVENT_MANTA) {
    int16_t cx = static_cast<int16_t>(roundf(_event.x));
    int16_t cy = static_cast<int16_t>(roundf(_event.y));
    int8_t d = _event.dir;
    float flap = sinf(_event.phase * 3.4f);
    int16_t wingShift = (flap > 0.3f) ? -1 : ((flap < -0.3f) ? 1 : 0);

    // Broad diamond manta body & central spine
    for (int16_t dx = -4; dx <= 4; ++dx) {
      setFbPixel(cx + d * dx, cy, COLOR_MANTA_BACK);
    }
    for (int16_t dx = -3; dx <= 4; ++dx) {
      setFbPixel(cx + d * dx, cy - 1, COLOR_MANTA_BACK);
      setFbPixel(cx + d * dx, cy + 1, (dx >= 1) ? COLOR_MANTA_BELLY : COLOR_MANTA_BACK);
    }
    // Iconic cephalic front lobes ("horns")
    setFbPixel(cx + d * 5, cy - 2, COLOR_MANTA_WING);
    setFbPixel(cx + d * 6, cy - 2, COLOR_MANTA_WING);
    setFbPixel(cx + d * 5, cy + 2, COLOR_MANTA_BELLY);
    setFbPixel(cx + d * 6, cy + 2, COLOR_MANTA_BELLY);

    // Wide sweeping upper & lower wings with undulating tips
    for (int16_t dx = -2; dx <= 3; ++dx) {
      setFbPixel(cx + d * dx, cy - 2, COLOR_MANTA_WING);
      setFbPixel(cx + d * dx, cy + 2, (dx >= 0) ? COLOR_MANTA_BELLY : COLOR_MANTA_WING);
    }
    for (int16_t dx = -1; dx <= 2; ++dx) {
      setFbPixel(cx + d * dx, cy - 3, COLOR_MANTA_WING);
      setFbPixel(cx + d * dx, cy + 3, COLOR_MANTA_WING);
    }
    // Outer wingtips curving up/down with the flap cycle
    setFbPixel(cx,          cy - 4 + wingShift, COLOR_MANTA_BACK);
    setFbPixel(cx + d * 1,  cy - 4 + wingShift, COLOR_MANTA_WING);
    setFbPixel(cx,          cy - 5 + wingShift, COLOR_MANTA_BACK);

    setFbPixel(cx,          cy + 4 + wingShift, COLOR_MANTA_BACK);
    setFbPixel(cx + d * 1,  cy + 4 + wingShift, COLOR_MANTA_BELLY);
    setFbPixel(cx,          cy + 5 + wingShift, COLOR_MANTA_BACK);

    // Long graceful whip tail trailing behind
    for (int16_t seg = 5; seg <= 10; ++seg) {
      int16_t ty = cy + static_cast<int16_t>(roundf(sinf(_event.phase * 4.0f - static_cast<float>(seg) * 0.55f) * 1.1f));
      setFbPixel(cx - d * seg, ty, COLOR_MANTA_BACK);
    }
  } else if (_event.type == EVENT_SHARK) {
    int16_t cx = static_cast<int16_t>(roundf(_event.x));
    int16_t cy = static_cast<int16_t>(roundf(_event.y));
    int8_t d = _event.dir;
    int8_t wag = (sinf(_event.phase * 7.5f) > 0.0f) ? 1 : -1;

    // Fuller torpedo body & pointed snout
    for (int16_t dx = -5; dx <= 5; ++dx) {
      setFbPixel(cx + d * dx, cy, COLOR_SHARK_BODY);
    }
    for (int16_t dx = -4; dx <= 5; ++dx) {
      setFbPixel(cx + d * dx, cy - 1, COLOR_SHARK_BODY);
    }
    for (int16_t dx = -3; dx <= 3; ++dx) {
      setFbPixel(cx + d * dx, cy - 2, COLOR_SHARK_BODY);
    }
    setFbPixel(cx + d * 6, cy,     COLOR_SHARK_BODY);  // Pointed snout
    setFbPixel(cx + d * 7, cy,     COLOR_SHARK_BODY);
    setFbPixel(cx + d * 6, cy - 1, COLOR_SHARK_BODY);
    setFbPixel(cx + d * 4, cy - 1, COLOR_BLACK);       // Dark eye
    // Pale underbelly & jaw
    for (int16_t dx = -3; dx <= 4; ++dx) {
      setFbPixel(cx + d * dx, cy + 1, COLOR_SHARK_BELLY);
    }
    for (int16_t dx = -1; dx <= 2; ++dx) {
      setFbPixel(cx + d * dx, cy + 2, COLOR_SHARK_BELLY);
    }
    // Tall curved triangular dorsal fin
    setFbPixel(cx + d * 1,  cy - 3, COLOR_SHARK_FIN);
    setFbPixel(cx,          cy - 3, COLOR_SHARK_FIN);
    setFbPixel(cx - d * 1,  cy - 3, COLOR_SHARK_FIN);
    setFbPixel(cx,          cy - 4, COLOR_SHARK_FIN);
    setFbPixel(cx - d * 1,  cy - 4, COLOR_SHARK_FIN);
    setFbPixel(cx - d * 2,  cy - 5, COLOR_SHARK_FIN);
    // Pectoral fin
    setFbPixel(cx + d * 2,  cy + 2, COLOR_SHARK_FIN);
    setFbPixel(cx + d * 1,  cy + 3, COLOR_SHARK_FIN);
    setFbPixel(cx,          cy + 3, COLOR_SHARK_FIN);
    // Larger crescent wagging tail
    setFbPixel(cx - d * 6, cy,           COLOR_SHARK_FIN);
    setFbPixel(cx - d * 7, cy - 1 + wag, COLOR_SHARK_FIN);
    setFbPixel(cx - d * 7, cy + wag,     COLOR_SHARK_FIN);
    setFbPixel(cx - d * 7, cy + 1 + wag, COLOR_SHARK_FIN);
    setFbPixel(cx - d * 8, cy - 3 + wag, COLOR_SHARK_FIN);
    setFbPixel(cx - d * 8, cy - 2 + wag, COLOR_SHARK_FIN);
    setFbPixel(cx - d * 8, cy + 2 + wag, COLOR_SHARK_FIN);
  } else if (_event.type == EVENT_SUBMARINE) {
    int16_t cx = static_cast<int16_t>(roundf(_event.x));
    int16_t cy = static_cast<int16_t>(roundf(_event.y));
    int8_t d = _event.dir;
    bool propSpin = (static_cast<int>(_event.phase * 14.0f) & 1) != 0;

    // Rounded yellow hull (2px longer in the middle: 16px long x 5px tall)
    for (int16_t dx = -5; dx <= 5; ++dx) {
      setFbPixel(cx + d * dx, cy - 2, COLOR_SUB_HULL);
      setFbPixel(cx + d * dx, cy + 2, COLOR_SUB_SHADE); // Keel
    }
    for (int16_t dx = -6; dx <= 6; ++dx) {
      setFbPixel(cx + d * dx, cy - 1, COLOR_SUB_HULL);
      setFbPixel(cx + d * dx, cy,     COLOR_SUB_HULL);
      setFbPixel(cx + d * dx, cy + 1, COLOR_SUB_HULL);
    }
    // Rounded nose cone
    setFbPixel(cx + d * 7, cy - 1, COLOR_SUB_HULL);
    setFbPixel(cx + d * 7, cy,     COLOR_SUB_HULL);
    setFbPixel(cx + d * 7, cy + 1, COLOR_SUB_SHADE);
    setFbPixel(cx + d * 8, cy,     COLOR_SUB_HULL);

    // Conning tower & silver periscope
    for (int16_t dx = -2; dx <= 2; ++dx) {
      setFbPixel(cx + d * dx, cy - 3, COLOR_SUB_SHADE);
    }
    for (int16_t dx = -1; dx <= 1; ++dx) {
      setFbPixel(cx + d * dx, cy - 4, COLOR_SUB_HULL);
    }
    setFbPixel(cx,         cy - 5, COLOR_SUB_METAL);
    setFbPixel(cx,         cy - 6, COLOR_SUB_METAL);
    setFbPixel(cx + d * 1, cy - 6, COLOR_SUB_METAL);
    setFbPixel(cx + d * 2, cy - 6, COLOR_SUB_METAL);

    // Two crisp 2x2 glowing cyan portholes spaced cleanly along the longer hull
    setFbPixel(cx + d * 2, cy - 1, COLOR_SUB_WINDOW);
    setFbPixel(cx + d * 3, cy - 1, COLOR_SUB_WINDOW);
    setFbPixel(cx + d * 2, cy,     COLOR_SUB_WINDOW);
    setFbPixel(cx + d * 3, cy,     COLOR_SUB_WINDOW);

    setFbPixel(cx - d * 2, cy - 1, COLOR_SUB_WINDOW);
    setFbPixel(cx - d * 3, cy - 1, COLOR_SUB_WINDOW);
    setFbPixel(cx - d * 2, cy,     COLOR_SUB_WINDOW);
    setFbPixel(cx - d * 3, cy,     COLOR_SUB_WINDOW);

    // Tail fins & spinning rear propeller
    setFbPixel(cx - d * 7, cy - 1, COLOR_SUB_SHADE);
    setFbPixel(cx - d * 7, cy,     COLOR_SUB_METAL);
    setFbPixel(cx - d * 7, cy + 1, COLOR_SUB_SHADE);
    if (propSpin) {
      for (int16_t dy = -2; dy <= 2; ++dy) {
        setFbPixel(cx - d * 8, cy + dy, COLOR_SUB_METAL);
      }
    } else {
      setFbPixel(cx - d * 8, cy - 1, COLOR_SUB_METAL);
      setFbPixel(cx - d * 8, cy,     COLOR_SUB_METAL);
      setFbPixel(cx - d * 8, cy + 1, COLOR_SUB_METAL);
    }
  }
}

void AquariumClockface::renderTime(const TimeSnapshot &snap)
{
  int h12 = snap.hour % 12;
  if (h12 == 0) h12 = 12;
  int minute = snap.minute;
  bool isAM = snap.isAM;

  // Tight black pill behind the time (shifted 1px up & 1px right)
  int16_t boxRight = (h12 >= 10) ? 30 : 26;
  for (int16_t py = 55; py <= 61; ++py) {
    for (int16_t px = 2; px <= boxRight; ++px) {
      setFbPixel(px, py, COLOR_TIME_BG);
    }
  }

  auto drawDigit = [this](uint8_t d, int16_t x, int16_t y, uint16_t col) {
    if (d > 9) return;
    for (uint8_t r = 0; r < 5; ++r) {
      for (uint8_t c = 0; c < 3; ++c) {
        if (DIGITS_3X5[d][r] & (0x4 >> c)) {
          setFbPixel(x + c, y + r, col);
        }
      }
    }
  };

  int16_t x = 3;
  const int16_t y = 56;
  if (h12 >= 10) {
    drawDigit(static_cast<uint8_t>(h12 / 10), x, y, COLOR_TIME_FG);
    x += 4;
  }
  drawDigit(static_cast<uint8_t>(h12 % 10), x, y, COLOR_TIME_FG);
  x += 4;
  setFbPixel(x, y + 1, COLOR_TIME_FG);
  setFbPixel(x, y + 3, COLOR_TIME_FG);
  x += 2;
  drawDigit(static_cast<uint8_t>(minute / 10), x, y, COLOR_TIME_FG);
  x += 4;
  drawDigit(static_cast<uint8_t>(minute % 10), x, y, COLOR_TIME_FG);
  x += 5;

  // 7x5 AM/PM
  if (isAM) {
    setFbPixel(x + 1, y,     COLOR_TIME_AMPM);
    setFbPixel(x,     y + 1, COLOR_TIME_AMPM); setFbPixel(x + 2, y + 1, COLOR_TIME_AMPM);
    setFbPixel(x,     y + 2, COLOR_TIME_AMPM); setFbPixel(x + 1, y + 2, COLOR_TIME_AMPM); setFbPixel(x + 2, y + 2, COLOR_TIME_AMPM);
    setFbPixel(x,     y + 3, COLOR_TIME_AMPM); setFbPixel(x + 2, y + 3, COLOR_TIME_AMPM);
    setFbPixel(x,     y + 4, COLOR_TIME_AMPM); setFbPixel(x + 2, y + 4, COLOR_TIME_AMPM);
  } else {
    setFbPixel(x,     y,     COLOR_TIME_AMPM); setFbPixel(x + 1, y,     COLOR_TIME_AMPM);
    setFbPixel(x,     y + 1, COLOR_TIME_AMPM); setFbPixel(x + 2, y + 1, COLOR_TIME_AMPM);
    setFbPixel(x,     y + 2, COLOR_TIME_AMPM); setFbPixel(x + 1, y + 2, COLOR_TIME_AMPM);
    setFbPixel(x,     y + 3, COLOR_TIME_AMPM);
    setFbPixel(x,     y + 4, COLOR_TIME_AMPM);
  }
  int16_t mx = x + 4;
  setFbPixel(mx,     y,     COLOR_TIME_AMPM); setFbPixel(mx + 2, y,     COLOR_TIME_AMPM);
  setFbPixel(mx,     y + 1, COLOR_TIME_AMPM); setFbPixel(mx + 1, y + 1, COLOR_TIME_AMPM); setFbPixel(mx + 2, y + 1, COLOR_TIME_AMPM);
  setFbPixel(mx,     y + 2, COLOR_TIME_AMPM); setFbPixel(mx + 2, y + 2, COLOR_TIME_AMPM);
  setFbPixel(mx,     y + 3, COLOR_TIME_AMPM); setFbPixel(mx + 2, y + 3, COLOR_TIME_AMPM);
  setFbPixel(mx,     y + 4, COLOR_TIME_AMPM); setFbPixel(mx + 2, y + 4, COLOR_TIME_AMPM);
}

void AquariumClockface::flushFramebuffer()
{
  for (int16_t y = 0; y < MATRIX_SIZE; ++y) {
    int row = y * MATRIX_SIZE;
    for (int16_t x = 0; x < MATRIX_SIZE; ++x) {
      int idx = row + x;
      uint16_t col = _fb[idx];
      if (_forceFullFlush || col != _prevFb[idx]) {
        _display->drawPixel(x, y, col);
        _prevFb[idx] = col;
      }
    }
  }
  _forceFullFlush = false;
}

void AquariumClockface::update()
{
  if (!_display || !_dateTime || !_fb || !_prevFb) return;

  unsigned long now = millis();
  if (_lastFrameMillis != 0 && (now - _lastFrameMillis) < FRAME_INTERVAL_MS) {
    return;
  }
  float dt = (_lastFrameMillis == 0)
             ? (static_cast<float>(FRAME_INTERVAL_MS) / 1000.0f)
             : static_cast<float>(now - _lastFrameMillis) / 1000.0f;
  if (dt > 0.1f) dt = 0.1f;
  _lastFrameMillis = now;
  _animTime += dt;

  TimeSnapshot snap = _dateTime->snapshot();

  updateBoids(dt);
  updateChest(dt, snap.minute);
  updateBubbles(dt);
  updateBgEvent(dt, now, snap.hour);

  renderWaterAndSand();
  renderSeaweed();
  renderTreasureChest();
  renderBubbles();
  renderBgEvent();
  renderFish();
  renderTime(snap);

  flushFramebuffer();
}
