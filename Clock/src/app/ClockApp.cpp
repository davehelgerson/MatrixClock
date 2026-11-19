#include "ClockApp.h"
#include <ui.h>
#include <soc/gpio_reg.h>
#include <soc/soc.h>
#include "../hardware/pin_config.h"
#include "../hardware/touch.h"
#include "../core/WifiConfig.h"
#include "../clockfaces/Clockface.h"

static_assert(ENCODER_CLK >= 32 && ENCODER_DT >= 32,
              "Encoder pins must map to GPIO_IN1_REG");

// Referenced by LVGL SquareLine events in lib/ui/src/ui_events.c
volatile int page_index = 0;

ClockApp clockApp;

namespace {
  const int SETTINGS_FACE_INDEX = WifiConfig::SETTINGS_FACE_INDEX;

  enum EncoderState : uint8_t {
    ENC_START     = 0,
    ENC_CW_BEGIN  = 1,
    ENC_CW_NEXT   = 2,
    ENC_CW_FINAL  = 3,
    ENC_CCW_BEGIN = 4,
    ENC_CCW_NEXT  = 5,
    ENC_CCW_FINAL = 6
  };

  // Buxton full-step quadrature transition tables indexed by [state][ab],
  // emitting +/-1 only once per complete 4-phase detent cycle so one physical
  // bump moves exactly one menu item and contact bounce is ignored.
  DRAM_ATTR const uint8_t ENCODER_NEXT_STATE[7][4] = {
    // 00         01             10             11
    {ENC_START,   ENC_CCW_BEGIN, ENC_CW_BEGIN,  ENC_START}, // ENC_START
    {ENC_CW_NEXT, ENC_START,     ENC_CW_BEGIN,  ENC_START}, // ENC_CW_BEGIN
    {ENC_CW_NEXT, ENC_CW_FINAL,  ENC_CW_BEGIN,  ENC_START}, // ENC_CW_NEXT
    {ENC_CW_NEXT, ENC_CW_FINAL,  ENC_START,     ENC_START}, // ENC_CW_FINAL  -> 11 emits +1
    {ENC_CCW_NEXT,ENC_CCW_BEGIN, ENC_START,     ENC_START}, // ENC_CCW_BEGIN
    {ENC_CCW_NEXT,ENC_CCW_BEGIN, ENC_CCW_FINAL, ENC_START}, // ENC_CCW_NEXT
    {ENC_CCW_NEXT,ENC_START,     ENC_CCW_FINAL, ENC_START}  // ENC_CCW_FINAL -> 11 emits -1
  };

  DRAM_ATTR const int8_t ENCODER_STEP[7][4] = {
    {0, 0, 0,  0},
    {0, 0, 0,  0},
    {0, 0, 0,  0},
    {0, 0, 0,  1},
    {0, 0, 0,  0},
    {0, 0, 0,  0},
    {0, 0, 0, -1}
  };

  inline uint8_t IRAM_ATTR readEncoderAb() {
    uint32_t in1 = REG_READ(GPIO_IN1_REG);
    uint8_t clk = (in1 >> (ENCODER_CLK - 32)) & 0x01U;
    uint8_t dt  = (in1 >> (ENCODER_DT - 32)) & 0x01U;
    return (clk << 1) | dt;
  }

  void onRotationSettingChanged(uint8_t rotation) {
    clockApp.setTftRotation(rotation);
  }

  void onPowerTimeoutsChanged(uint16_t dimMinutes, uint16_t offMinutes) {
    clockApp.setPowerTimeouts(dimMinutes, offMinutes);
  }

  void onDadSettingsChanged() {
    clockApp.refreshMenuLabels();
  }

  void dispFlushThunk(lv_disp_drv_t *disp, const lv_area_t *area, lv_color_t *color_p) {
    clockApp.flushDisplay(disp, area, color_p);
  }

  void touchpadReadThunk(lv_indev_drv_t *indev_driver, lv_indev_data_t *data) {
    clockApp.readTouchpad(indev_driver, data);
  }

  void encoderTimerThunk(void *arg) {
    (void)arg;
    clockApp.pollEncoder();
  }

  void tftTaskThunk(void *pvParameters) {
    clockApp.runTftTask();
  }

  void controlTaskThunk(void *pvParameters) {
    clockApp.runControlTask();
  }

  void timeTaskThunk(void *pvParameters) {
    clockApp.runTimeTask();
  }
} // namespace

void ClockApp::begin()
{
  Serial.begin(115200);
  delay(1000);
  Serial.println("Starting up...");

  initPins();
  initTft();
  initMatrix();

  _dimMinutes = WifiConfig::loadDimMinutes();
  _offMinutes = WifiConfig::loadOffMinutes();
  _dateTime.begin();
  initClockfaces();

  __atomic_store_n(&_lastActivityMillis, millis(), __ATOMIC_RELEASE);
  Serial.println("Setup done");

  xTaskCreatePinnedToCore(tftTaskThunk, "Task_TFT", 10240, nullptr, 2, nullptr, 0);
  xTaskCreatePinnedToCore(controlTaskThunk, "Task_Control", 4096, nullptr, 1, nullptr, 0);
  delay(2500); // Allow boot logo screen animation to finish before driving the matrix
  xTaskCreatePinnedToCore(timeTaskThunk, "Task_Time", 20480, nullptr, 1, nullptr, 1);
}

void ClockApp::initPins()
{
  pinMode(TFT_BLK, OUTPUT);
  digitalWrite(TFT_BLK, HIGH);

  pinMode(BUTTON_PIN, INPUT_PULLUP);
  pinMode(ENCODER_CLK, INPUT_PULLUP);
  pinMode(ENCODER_DT, INPUT_PULLUP);

  uint8_t ab = readEncoderAb();
  _encoderMask = (ab == 0) ? 0x03U : 0x00U;
  _lastRawAb = ab ^ _encoderMask;
  _debouncedAb = _lastRawAb;
  _rawStableCount = 2;
  _encoderState = ENC_START;

  esp_timer_create_args_t timerArgs = {};
  timerArgs.callback = encoderTimerThunk;
  timerArgs.arg = nullptr;
  timerArgs.dispatch_method = ESP_TIMER_TASK;
  timerArgs.name = "enc_poll";
  if (esp_timer_create(&timerArgs, &_encoderTimer) == ESP_OK) {
    esp_timer_start_periodic(_encoderTimer, 500);
  }
}

void ClockApp::initTft()
{
  _tftRotation = WifiConfig::loadTftRotation();
  _bus = new Arduino_ESP32SPI(TFT_DC, TFT_CS, TFT_SCLK, TFT_MOSI, TFT_MISO, HSPI, true);
  _gfx = new Arduino_GC9A01(_bus, TFT_RES, _tftRotation, true /* IPS */);
  _gfx->begin();
  delay(200);

  lv_init();
  lv_disp_draw_buf_init(&_drawBuf, _buf, nullptr, TFT_WIDTH * TFT_HEIGHT / 10);

  static lv_disp_drv_t dispDrv;
  lv_disp_drv_init(&dispDrv);
  dispDrv.hor_res = TFT_WIDTH;
  dispDrv.ver_res = TFT_HEIGHT;
  dispDrv.flush_cb = dispFlushThunk;
  dispDrv.draw_buf = &_drawBuf;
  lv_disp_drv_register(&dispDrv);

  static lv_indev_drv_t indevDrv;
  lv_indev_drv_init(&indevDrv);
  indevDrv.type = LV_INDEV_TYPE_POINTER;
  indevDrv.read_cb = touchpadReadThunk;
  lv_indev_drv_register(&indevDrv);

  ui_init();
  lv_obj_set_style_anim_time(ui_Roller1, 120, LV_PART_MAIN);
  _gifIndex = WifiConfig::loadSelectedFace(CLOCKFACE_COUNT, SETTINGS_FACE_INDEX);
  rebuildVisibleMenu(true);
}

void ClockApp::initMatrix()
{
  HUB75_I2S_CFG mxconfig(PANEL_RES_X, PANEL_RES_Y, PANEL_CHAIN);
  _dmaDisplay = new MatrixPanel_I2S_DMA(mxconfig);
  _dmaDisplay->begin(R1_PIN, G1_PIN, B1_PIN, R2_PIN, G2_PIN, B2_PIN,
                     A_PIN, B_PIN, C_PIN, D_PIN, E_PIN, LAT_PIN, OE_PIN, CLK_PIN);
  _dmaDisplay->setBrightness(map(_brightness, 0, 100, 0, 255));
  _dmaDisplay->clearScreen();
}

void ClockApp::initClockfaces()
{
  using namespace ClockfaceRegistry;
  _clockfaces[FACE_MARIO]     = new Clockface3(_dmaDisplay);
  _clockfaces[FACE_PACMAN]    = new Clockface2(_dmaDisplay);
  _clockfaces[FACE_PACMAN2]   = new Pacman2Clockface(_dmaDisplay);
  _clockfaces[FACE_POKEDEX]   = new Clockface1(_dmaDisplay);
  _clockfaces[FACE_MAPLE]     = new Clockface4(_dmaDisplay);
  _clockfaces[FACE_DAD]       = new Clockface5(_dmaDisplay);
  _clockfaces[FACE_STARS]     = new StarsClockface(_dmaDisplay);
  _clockfaces[FACE_PIPES]     = new PipesClockface(_dmaDisplay);
  _clockfaces[FACE_ANALOG]    = new AnalogClockface(_dmaDisplay);
  _clockfaces[FACE_MATRIX]    = new MatrixRainClockface(_dmaDisplay);
  _clockfaces[FACE_INVADERS]  = new InvadersClockface(_dmaDisplay);
  _clockfaces[FACE_JOKE]      = new JokeClockface(_dmaDisplay);
  _clockfaces[FACE_HOURGLASS] = new HourglassClockface(_dmaDisplay);
  _clockfaces[FACE_SETTINGS]  = new WifiClockface(_dmaDisplay,
                                                  onRotationSettingChanged,
                                                  onPowerTimeoutsChanged,
                                                  onDadSettingsChanged);

  int startIdx = (_gifIndex >= 0 && _gifIndex < CLOCKFACE_COUNT) ? _gifIndex : 0;
  _clockfaces[startIdx]->setup(&_dateTime);
}

void ClockApp::runTftTask()
{
  bool tftOn = true;
  while (1) {
    bool wantOn = _tftOnRequested;
    if (wantOn != tftOn) {
      tftOn = wantOn;
      if (tftOn) {
        _gfx->displayOn();
      } else {
        _gfx->displayOff();
      }
    }
    if (_rotationDirty) {
      _rotationDirty = false;
      _gfx->setRotation(_tftRotation);
      lv_obj_invalidate(lv_scr_act());
    }
    if (_rollerOptionsDirty) {
      _rollerOptionsDirty = false;
      rebuildVisibleMenu(false);
    }
    updateUiWidgets();
    lv_timer_handler();
    vTaskDelay(pdMS_TO_TICKS(10));
  }
}

void ClockApp::runControlTask()
{
  while (1) {
    handleEncoder();
    handlePowerState();
    vTaskDelay(pdMS_TO_TICKS(10));
  }
}

void ClockApp::runTimeTask()
{
  int activeIndex = _gifIndex;
  while (1) {
    int targetIndex = _gifIndex;
    if (targetIndex != activeIndex) {
      unsigned long settleMs = (targetIndex == SETTINGS_FACE_INDEX) ? 450UL : 150UL;
      unsigned long waitStart = millis();
      while ((millis() - waitStart) < settleMs) {
        vTaskDelay(pdMS_TO_TICKS(25));
        int latest = _gifIndex;
        if (latest != targetIndex) {
          targetIndex = latest;
          settleMs = (targetIndex == SETTINGS_FACE_INDEX) ? 450UL : 150UL;
          waitStart = millis();
        }
      }

      if (targetIndex != activeIndex) {
        _clockfaces[activeIndex]->teardown();
        activeIndex = targetIndex;
        _clockfaces[activeIndex]->setup(&_dateTime);
        if (activeIndex != SETTINGS_FACE_INDEX) {
          WifiConfig::saveSelectedFace(activeIndex);
        }
      }
    }

    // setBrightness rewrites OE bits across the DMA buffers, so it must not
    // race with clockface drawing; apply it here on the drawing task.
    // Atomic read-and-clear so a request written by Task_Control between the
    // read and the reset can't be dropped.
    int brightness = __atomic_exchange_n(&_matrixBrightnessRequested, -1, __ATOMIC_ACQ_REL);
    if (brightness >= 0) {
      _dmaDisplay->setBrightness(brightness);
    }

    _clockfaces[activeIndex]->update();
    vTaskDelay(pdMS_TO_TICKS(5));
  }
}

void ClockApp::pollEncoder()
{
  uint8_t rawAb = readEncoderAb() ^ _encoderMask;
  if (rawAb == _lastRawAb) {
    if (_rawStableCount < 2) {
      ++_rawStableCount;
    }
  } else {
    _lastRawAb = rawAb;
    _rawStableCount = 0;
  }

  // Require 3 consecutive matching 500 us reads (1.0 ms stability) before advancing state
  if (_rawStableCount < 2 || rawAb == _debouncedAb) {
    return;
  }

  _debouncedAb = rawAb;
  uint8_t state = _encoderState;
  int8_t step = ENCODER_STEP[state][rawAb];
  _encoderState = ENCODER_NEXT_STATE[state][rawAb];

  if (step != 0) {
    bool sleeping = __atomic_load_n(&_screenSleeping, __ATOMIC_ACQUIRE);
    bool dimmed   = __atomic_load_n(&_screenDimmed, __ATOMIC_ACQUIRE);
    portENTER_CRITICAL(&_encoderMux);
    if (sleeping) {
      // Both displays are dark: the first detent only wakes them, so the user
      // doesn't change clockface/brightness without seeing it happen.
      __atomic_store_n(&_wakeRequested, true, __ATOMIC_RELEASE);
    } else {
      _encoderDelta += step;
      if (dimmed) {
        __atomic_store_n(&_wakeRequested, true, __ATOMIC_RELEASE);
      }
    }
    portEXIT_CRITICAL(&_encoderMux);
  }
}

void ClockApp::handleEncoder()
{
  portENTER_CRITICAL(&_encoderMux);
  int delta = _encoderDelta;
  _encoderDelta = 0;
  portEXIT_CRITICAL(&_encoderMux);

  if (delta == 0) {
    return;
  }

  __atomic_store_n(&_lastActivityMillis, millis(), __ATOMIC_RELEASE);

  if (page_index == 0) {
    portENTER_CRITICAL(&_menuMux);
    if (_visibleCount > 0) {
      int curPos = rollerPositionForFace(_gifIndex);
      int nextPos = ((curPos + delta) % _visibleCount + _visibleCount) % _visibleCount;
      _gifIndex = _visibleFaces[nextPos];
      Serial.printf("enc delta=%d -> gif_num=%d (pos=%d)\n", delta, _gifIndex, nextPos);
    }
    portEXIT_CRITICAL(&_menuMux);
  } else if (page_index == 1) {
    int nextBrightness = constrain(_brightness + delta, 0, 100);
    _brightness = nextBrightness;
    requestMatrixBrightness(nextBrightness);
  }
}

void ClockApp::requestMatrixBrightness(int percent)
{
  _matrixBrightnessRequested = map(percent, 0, 100, 0, 255);
}

void ClockApp::handlePowerState()
{
  uint16_t dimMin = WifiConfig::normalizeTimeoutMinutes(__atomic_load_n(&_dimMinutes, __ATOMIC_ACQUIRE));
  uint16_t offMin = WifiConfig::normalizeTimeoutMinutes(__atomic_load_n(&_offMinutes, __ATOMIC_ACQUIRE));
  unsigned long idleTime = millis() - __atomic_load_n(&_lastActivityMillis, __ATOMIC_ACQUIRE);
  bool sleeping = __atomic_load_n(&_screenSleeping, __ATOMIC_ACQUIRE);
  bool dimmed   = __atomic_load_n(&_screenDimmed, __ATOMIC_ACQUIRE);

  if (!sleeping &&
      idleTime >= static_cast<unsigned long>(offMin) * 60000UL) {
    __atomic_store_n(&_screenSleeping, true, __ATOMIC_RELEASE);
    __atomic_store_n(&_screenDimmed, false, __ATOMIC_RELEASE);
    _tftOnRequested = false;
    _matrixBrightnessRequested = 0;
  } else if (!sleeping && !dimmed &&
             idleTime >= static_cast<unsigned long>(dimMin) * 60000UL && _brightness > 5) {
    __atomic_store_n(&_screenDimmed, true, __ATOMIC_RELEASE);
    requestMatrixBrightness(5);
  }

  if (__atomic_exchange_n(&_wakeRequested, false, __ATOMIC_ACQ_REL)) {
    _tftOnRequested = true;
    requestMatrixBrightness(_brightness);
    __atomic_store_n(&_lastActivityMillis, millis(), __ATOMIC_RELEASE);
    __atomic_store_n(&_screenSleeping, false, __ATOMIC_RELEASE);
    __atomic_store_n(&_screenDimmed, false, __ATOMIC_RELEASE);
  }
}

void ClockApp::updateUiWidgets()
{
  static int lastPage = -1;
  static int lastGif = -1;
  static int lastBrightness = -1;

  int curPage = page_index;
  int curGif = _gifIndex;
  int curBrightness = _brightness;

  if (curPage == 0) {
    portENTER_CRITICAL(&_menuMux);
    int targetPos = rollerPositionForFace(curGif);
    int lastPos = (lastGif >= 0) ? rollerPositionForFace(lastGif) : -1;
    portEXIT_CRITICAL(&_menuMux);
    bool needsSync = (curPage != lastPage) || (curGif != lastGif) ||
                     (static_cast<int>(lv_roller_get_selected(ui_Roller1)) != targetPos);
    if (needsSync) {
      if (curPage == lastPage && lastPos >= 0 && targetPos != lastPos) {
        // Re-center the infinite roller on its middle page before starting a new
        // animation so rapid turns cannot drift into LVGL's edge-page clamp/underflow.
        lv_roller_set_selected(ui_Roller1, lastPos, LV_ANIM_OFF);
      }
      lv_anim_enable_t anim = (curPage != lastPage) ? LV_ANIM_OFF : LV_ANIM_ON;
      lv_roller_set_selected(ui_Roller1, targetPos, anim);
      lastGif = curGif;
      lastPage = curPage;
    }
  } else if (curPage == 1 && (curPage != lastPage || curBrightness != lastBrightness)) {
    char temp[16] = "";
    lv_arc_set_value(ui_Arc1, curBrightness);
    snprintf(temp, sizeof(temp), "%d%%", curBrightness);
    lv_label_set_text(ui_Label1, temp);
    lastBrightness = curBrightness;
    lastPage = curPage;
  }
}

void ClockApp::flushDisplay(lv_disp_drv_t *disp, const lv_area_t *area, lv_color_t *color_p)
{
  uint32_t w = (area->x2 - area->x1 + 1);
  uint32_t h = (area->y2 - area->y1 + 1);

#if (LV_COLOR_16_SWAP != 0)
  _gfx->draw16bitBeRGBBitmap(area->x1, area->y1, (uint16_t *)&color_p->full, w, h);
#else
  _gfx->draw16bitRGBBitmap(area->x1, area->y1, (uint16_t *)&color_p->full, w, h);
#endif

  lv_disp_flush_ready(disp);
}

void ClockApp::setTftRotation(uint8_t rotation)
{
  if (rotation <= 3) {
    _tftRotation = rotation;
    _rotationDirty = true;
  }
}

void ClockApp::refreshMenuLabels()
{
  _rollerOptionsDirty = true;
}

int ClockApp::rollerPositionForFace(int faceIndex) const
{
  for (int i = 0; i < _visibleCount; ++i) {
    if (_visibleFaces[i] == faceIndex) {
      return i;
    }
  }
  return 0;
}

void ClockApp::rebuildVisibleMenu(bool animateOff)
{
  (void)animateOff;
  String dadName = WifiConfig::loadDadMenuName();

  uint32_t mask = WifiConfig::loadEnabledFacesMask();
  String rollerOpts;
  int nextFaces[CLOCKFACE_COUNT] = {0};
  int nextCount = 0;

  for (int i = 0; i < CLOCKFACE_COUNT; ++i) {
    if (WifiConfig::isFaceEnabled(mask, i, SETTINGS_FACE_INDEX)) {
      if (nextCount > 0) rollerOpts += "\n";
      const char *label = (i == ClockfaceRegistry::DAD_INDEX)
                          ? dadName.c_str()
                          : ClockfaceRegistry::defaultLabel(i);
      rollerOpts += label;
      nextFaces[nextCount++] = i;
    }
  }

  if (nextCount == 0) {
    rollerOpts = ClockfaceRegistry::defaultLabel(SETTINGS_FACE_INDEX);
    nextFaces[0] = SETTINGS_FACE_INDEX;
    nextCount = 1;
  }

  portENTER_CRITICAL(&_menuMux);
  for (int i = 0; i < nextCount; ++i) {
    _visibleFaces[i] = nextFaces[i];
  }
  _visibleCount = nextCount;
  if (!WifiConfig::isFaceEnabled(mask, _gifIndex, SETTINGS_FACE_INDEX)) {
    _gifIndex = _visibleFaces[0];
  }
  int selectedPos = rollerPositionForFace(_gifIndex);
  portEXIT_CRITICAL(&_menuMux);

  lv_roller_set_options(ui_Roller1, rollerOpts.c_str(), LV_ROLLER_MODE_INFINITE);
  lv_roller_set_selected(ui_Roller1, selectedPos, LV_ANIM_OFF);
}

void ClockApp::setPowerTimeouts(uint16_t dimMinutes, uint16_t offMinutes)
{
  __atomic_store_n(&_dimMinutes, WifiConfig::normalizeTimeoutMinutes(dimMinutes), __ATOMIC_RELEASE);
  __atomic_store_n(&_offMinutes, WifiConfig::normalizeTimeoutMinutes(offMinutes), __ATOMIC_RELEASE);
  __atomic_store_n(&_lastActivityMillis, millis(), __ATOMIC_RELEASE);
  if (__atomic_load_n(&_screenSleeping, __ATOMIC_ACQUIRE) ||
      __atomic_load_n(&_screenDimmed, __ATOMIC_ACQUIRE)) {
    __atomic_store_n(&_wakeRequested, true, __ATOMIC_RELEASE);
  }
}

void ClockApp::readTouchpad(lv_indev_drv_t *indev_driver, lv_indev_data_t *data)
{
  int touchX = 0, touchY = 0;

  if (read_touch(&touchX, &touchY) != 1) {
    _touchSuppressed = false;
    data->state = LV_INDEV_STATE_REL;
    return;
  }

  // Any touch counts as user activity so the idle dim/off timers restart.
  __atomic_store_n(&_lastActivityMillis, millis(), __ATOMIC_RELEASE);

  if (__atomic_load_n(&_screenSleeping, __ATOMIC_ACQUIRE) || _touchSuppressed) {
    // A tap on the dark screen only wakes it; swallow the press until the
    // finger lifts so it can't trigger whatever widget is under it.
    __atomic_store_n(&_wakeRequested, true, __ATOMIC_RELEASE);
    _touchSuppressed = true;
    data->state = LV_INDEV_STATE_REL;
    return;
  }

  data->state = LV_INDEV_STATE_PR;
  switch (_tftRotation) {
    case 0:
      data->point.x = (uint16_t)touchX;
      data->point.y = (uint16_t)touchY;
      break;
    case 1:
      data->point.x = (uint16_t)touchY;
      data->point.y = (uint16_t)(TFT_HEIGHT - touchX);
      break;
    case 2:
      data->point.x = (uint16_t)(TFT_WIDTH - touchX);
      data->point.y = (uint16_t)(TFT_HEIGHT - touchY);
      break;
    case 3:
    default:
      data->point.x = (uint16_t)(TFT_WIDTH - touchY);
      data->point.y = (uint16_t)touchX;
      break;
  }

  if (__atomic_load_n(&_screenDimmed, __ATOMIC_ACQUIRE)) {
    __atomic_store_n(&_wakeRequested, true, __ATOMIC_RELEASE);
  }
}

