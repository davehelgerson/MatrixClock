#include "ClockApp.h"
#include <ui.h>
#include "../hardware/pin_config.h"
#include "../hardware/touch.h"
#include "../clockfaces/Clockface.h"

// Referenced by LVGL SquareLine events in lib/ui/src/ui_events.c
int page_index = 0;

ClockApp clockApp;

namespace {
  enum EncoderState : uint8_t {
    ENC_START_00 = 0,
    ENC_CW_00    = 1,
    ENC_CCW_00   = 2,
    ENC_START_11 = 3,
    ENC_CW_11    = 4,
    ENC_CCW_11   = 5
  };

  // Buxton half-step quadrature transition tables indexed by [state][ab],
  // where ab = (CLK << 1) | DT. Monitoring both pins cancels mechanical
  // contact bounce cleanly without skipping or reversing steps.
  DRAM_ATTR const uint8_t ENCODER_NEXT_STATE[6][4] = {
    {ENC_START_00, ENC_CW_00,  ENC_CCW_00, ENC_START_11},
    {ENC_START_00, ENC_CW_00,  ENC_CCW_00, ENC_START_11},
    {ENC_START_00, ENC_CW_00,  ENC_CCW_00, ENC_START_11},
    {ENC_START_00, ENC_CCW_11, ENC_CW_11,  ENC_START_11},
    {ENC_START_00, ENC_CCW_11, ENC_CW_11,  ENC_START_11},
    {ENC_START_00, ENC_CCW_11, ENC_CW_11,  ENC_START_11}
  };

  DRAM_ATTR const int8_t ENCODER_STEP[6][4] = {
    {  0,  0,  0,  0},
    {  0,  0,  0,  1},
    {  0,  0,  0, -1},
    {  0,  0,  0,  0},
    {  1,  0,  0,  0},
    { -1,  0,  0,  0}
  };

  void dispFlushThunk(lv_disp_drv_t *disp, const lv_area_t *area, lv_color_t *color_p) {
    clockApp.flushDisplay(disp, area, color_p);
  }

  void touchpadReadThunk(lv_indev_drv_t *indev_driver, lv_indev_data_t *data) {
    clockApp.readTouchpad(indev_driver, data);
  }

  void IRAM_ATTR encoderIsrThunk() {
    clockApp.onEncoderEdge();
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

  _dateTime.begin();
  initClockfaces();

  _lastActivityMillis = millis();
  Serial.println("Setup done");

  xTaskCreatePinnedToCore(tftTaskThunk, "Task_TFT", 10240, nullptr, 2, nullptr, 0);
  xTaskCreatePinnedToCore(controlTaskThunk, "Task_Control", 4096, nullptr, 1, nullptr, 0);
  delay(2500); // Allow boot logo screen animation to finish before driving the matrix
  xTaskCreatePinnedToCore(timeTaskThunk, "Task_Time", 16384, nullptr, 1, nullptr, 1);
}

void ClockApp::initPins()
{
  pinMode(TFT_BLK, OUTPUT);
  digitalWrite(TFT_BLK, HIGH);

  pinMode(BUTTON_PIN, INPUT_PULLUP);
  pinMode(ENCODER_CLK, INPUT_PULLUP);
  pinMode(ENCODER_DT, INPUT_PULLUP);

  uint8_t ab = ((uint8_t)digitalRead(ENCODER_CLK) << 1) | (uint8_t)digitalRead(ENCODER_DT);
  _encoderState = (ab == 0) ? ENC_START_00 : ENC_START_11;

  attachInterrupt(ENCODER_CLK, encoderIsrThunk, CHANGE);
  attachInterrupt(ENCODER_DT, encoderIsrThunk, CHANGE);
}

void ClockApp::initTft()
{
  _bus = new Arduino_ESP32SPI(TFT_DC, TFT_CS, TFT_SCLK, TFT_MOSI, TFT_MISO, HSPI, true);
  _gfx = new Arduino_GC9A01(_bus, TFT_RES, 3 /* rotation */, true /* IPS */);
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
  _clockfaces[0] = new Clockface3(_dmaDisplay);     // Mario
  _clockfaces[1] = new Clockface2(_dmaDisplay);     // Pacman
  _clockfaces[2] = new Clockface1(_dmaDisplay);     // Pokedex
  _clockfaces[3] = new Clockface4(_dmaDisplay);     // Maple
  _clockfaces[4] = new Clockface5(_dmaDisplay);     // Dad
  _clockfaces[5] = new WifiClockface(_dmaDisplay);  // WIFI

  _clockfaces[0]->setup(&_dateTime);
}

void ClockApp::runTftTask()
{
  while (1) {
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
  int activeIndex = 0;
  while (1) {
    int targetIndex = _gifIndex;
    if (targetIndex != activeIndex) {
      vTaskDelay(pdMS_TO_TICKS(150));
      targetIndex = _gifIndex;
      if (targetIndex != activeIndex) {
        _clockfaces[activeIndex]->teardown();
        activeIndex = targetIndex;
        _clockfaces[activeIndex]->setup(&_dateTime);
      }
    }
    _clockfaces[activeIndex]->update();
    vTaskDelay(pdMS_TO_TICKS(5));
  }
}

void IRAM_ATTR ClockApp::onEncoderEdge()
{
  uint8_t ab = ((uint8_t)digitalRead(ENCODER_CLK) << 1) | (uint8_t)digitalRead(ENCODER_DT);

  portENTER_CRITICAL_ISR(&_encoderMux);
  uint8_t state = _encoderState;
  int8_t step = ENCODER_STEP[state][ab];
  _encoderState = ENCODER_NEXT_STATE[state][ab];
  if (step != 0) {
    _encoderDelta += step;
    if (_screenSleeping || _screenDimmed) {
      _wakeRequested = true;
    }
  }
  portEXIT_CRITICAL_ISR(&_encoderMux);
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

  _lastActivityMillis = millis();

  if (page_index == 0) {
    int nextIndex = ((_gifIndex + delta) % CLOCKFACE_COUNT + CLOCKFACE_COUNT) % CLOCKFACE_COUNT;
    _gifIndex = nextIndex;
    Serial.print("gif_num=");
    Serial.println(nextIndex);
  } else if (page_index == 1) {
    int nextBrightness = constrain(_brightness + delta, 0, 100);
    _brightness = nextBrightness;
    _dmaDisplay->setBrightness(map(nextBrightness, 0, 100, 0, 255));
  }
}

void ClockApp::handlePowerState()
{
  const unsigned long dimTimeout   = 300000UL;   // 5 minutes
  const unsigned long sleepTimeout = 3600000UL;  // 1 hour

  unsigned long idleTime = millis() - _lastActivityMillis;

  if (!_screenSleeping && idleTime >= sleepTimeout) {
    _screenSleeping = true;
    _screenDimmed = false;
    _gfx->displayOff();
    _dmaDisplay->setBrightness(0);
  } else if (!_screenSleeping && !_screenDimmed && idleTime >= dimTimeout && _brightness > 5) {
    _screenDimmed = true;
    _dmaDisplay->setBrightness(map(5, 0, 100, 0, 255));
  }

  if (_wakeRequested) {
    _gfx->displayOn();
    _dmaDisplay->setBrightness(map(_brightness, 0, 100, 0, 255));
    _lastActivityMillis = millis();
    _wakeRequested = false;
    _screenSleeping = false;
    _screenDimmed = false;
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

  if (curPage == 0 && (curPage != lastPage || curGif != lastGif)) {
    lv_anim_enable_t anim = (curPage != lastPage) ? LV_ANIM_OFF : LV_ANIM_ON;
    lv_roller_set_selected(ui_Roller1, curGif, anim);
    lastGif = curGif;
    lastPage = curPage;
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

void ClockApp::readTouchpad(lv_indev_drv_t *indev_driver, lv_indev_data_t *data)
{
  int touchX = 0, touchY = 0;

  if (read_touch(&touchX, &touchY) == 1) {
    data->state = LV_INDEV_STATE_PR;
    data->point.x = (uint16_t)(240 - touchY);
    data->point.y = (uint16_t)touchX;

    if (_screenSleeping || _screenDimmed) {
      _wakeRequested = true;
    }
  } else {
    data->state = LV_INDEV_STATE_REL;
  }
}

