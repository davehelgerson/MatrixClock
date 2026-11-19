#ifndef AquariumClockface_h
#define AquariumClockface_h

#include <Adafruit_GFX.h>
#include "../core/IClockface.h"
#include "../core/CWDateTime.h"

/**
 * 64x64 Aquarium Clockface.
 *
 * Features a sunlit-to-deep water tank, textured sandy bottom, swaying seaweed,
 * rising bubbles, schooling tropical fish driven by a 2D boids flocking
 * algorithm with randomized swim speeds, turning angles, and idle wandering,
 * plus randomly spawned background events (a scuttling crab or drifting
 * bioluminescent jellyfish) and a lower-left HH:MM AM/PM readout.
 */
class AquariumClockface : public IClockface {
  private:
    static const int16_t MATRIX_SIZE        = 64;
    static const uint8_t FISH_COUNT         = 5;
    static const uint8_t WEED_COUNT         = 5;
    static const uint8_t BUBBLE_COUNT       = 4;
    static const uint8_t CHEST_BUBBLE_COUNT = 8;
    static const uint8_t SUB_BUBBLE_COUNT   = 6;
    static const int16_t CHEST_X            = 46;
    static const unsigned long FRAME_INTERVAL_MS = 33; // ~30 FPS
    static const unsigned long CRAB_INTERVAL_MS  = 2UL * 60000UL;
    static const unsigned long JELLY_INTERVAL_MS = 2UL * 60000UL;
    static const unsigned long MANTA_INTERVAL_MS = 10UL * 60000UL;
    static const unsigned long SHARK_INTERVAL_MS = 20UL * 60000UL;

    struct Fish {
      float x;
      float y;
      float vx;
      float vy;
      float minSpeed;
      float maxSpeed;
      float maxTurnRate;
      float wanderAngle;
      float idleTimer;
      bool  idling;
      bool  big;
      uint16_t bodyColor;
      uint16_t accentColor;
      uint8_t tailPhase;
    };

    struct Seaweed {
      int8_t baseX;
      uint8_t height;
      float phase;
      float speed;
      uint16_t color;
      uint16_t tipColor;
    };

    struct Bubble {
      float x;
      float y;
      float speed;
      float wobblePhase;
      bool  active;
    };

    enum EventType : uint8_t {
      EVENT_NONE      = 0,
      EVENT_CRAB      = 1,
      EVENT_JELLYFISH = 2,
      EVENT_MANTA     = 3,
      EVENT_SHARK     = 4,
      EVENT_SUBMARINE = 5
    };

    struct BgEvent {
      EventType type = EVENT_NONE;
      float x = 0.0f;
      float y = 0.0f;
      float vx = 0.0f;
      float vy = 0.0f;
      float phase = 0.0f;
      float pauseTimer = 0.0f;
      int8_t dir = 1;
    };

    Adafruit_GFX *_display;
    CWDateTime *_dateTime = nullptr;

    uint16_t *_fb = nullptr;
    uint16_t *_prevFb = nullptr;

    Fish _fish[FISH_COUNT];
    Seaweed _weeds[WEED_COUNT];
    Bubble _bubbles[BUBBLE_COUNT];
    Bubble _chestBubbles[CHEST_BUBBLE_COUNT];
    Bubble _subBubbles[SUB_BUBBLE_COUNT];
    int8_t _sandTop[MATRIX_SIZE];
    BgEvent _event;

    unsigned long _lastFrameMillis = 0;
    unsigned long _lastCrabMillis = 0;
    unsigned long _lastJellyMillis = 0;
    unsigned long _lastMantaMillis = 0;
    unsigned long _lastSharkMillis = 0;
    float _animTime = 0.0f;
    float _chestOpenTimer = 0.0f;
    float _chestBubbleSpawnTimer = 0.0f;
    float _subBubbleSpawnTimer = 0.0f;
    int _lastMinute = -1;
    int _lastHour = -1;
    bool _subPending = false;
    bool _forceFullFlush = true;

    void initScene();
    void updateBoids(float dt);
    void updateBubbles(float dt);
    void updateChest(float dt, int minute);
    void updateBgEvent(float dt, unsigned long now, int hour);
    void spawnCrab();
    void spawnJellyfish();
    void spawnSubmarine();
    void spawnManta();
    void spawnShark();

    void renderWaterAndSand();
    void renderSeaweed();
    void renderTreasureChest();
    void renderBubbles();
    void renderFish();
    void renderBgEvent();
    void renderTime(const TimeSnapshot &snap);
    void flushFramebuffer();
    inline void setFbPixel(int16_t x, int16_t y, uint16_t color);

  public:
    AquariumClockface(Adafruit_GFX *display);
    void setup(CWDateTime *dateTime) override;
    void update() override;
    void teardown() override;
};

#endif
