#ifndef ClockApp_h
#define ClockApp_h

#include <Arduino.h>
#include <Arduino_GFX_Library.h>
#include <ESP32-HUB75-MatrixPanel-I2S-DMA.h>
#include <lvgl.h>
#include "../core/CWDateTime.h"
#include "../core/IClockface.h"

class ClockApp {
  public:
    void begin();

    // FreeRTOS task loops & ISR hooks
    void runTftTask();
    void runControlTask();
    void runTimeTask();
    void IRAM_ATTR onEncoderEdge();
    void flushDisplay(lv_disp_drv_t *disp, const lv_area_t *area, lv_color_t *color_p);
    void readTouchpad(lv_indev_drv_t *indev_driver, lv_indev_data_t *data);

  private:
    static const uint16_t TFT_WIDTH  = 240;
    static const uint16_t TFT_HEIGHT = 240;
    static const uint8_t  PANEL_RES_X = 64;
    static const uint8_t  PANEL_RES_Y = 64;
    static const uint8_t  PANEL_CHAIN = 1;
    static const int     CLOCKFACE_COUNT = 6;

    Arduino_ESP32SPI *_bus = nullptr;
    Arduino_GFX *_gfx = nullptr;
    MatrixPanel_I2S_DMA *_dmaDisplay = nullptr;

    CWDateTime _dateTime;
    IClockface *_clockfaces[CLOCKFACE_COUNT] = {nullptr};

    lv_disp_draw_buf_t _drawBuf;
    lv_color_t _buf[TFT_WIDTH * TFT_HEIGHT / 10];

    portMUX_TYPE _encoderMux = portMUX_INITIALIZER_UNLOCKED;
    volatile int _encoderDelta = 0;
    volatile uint8_t _encoderState = 3;

    volatile int _brightness = 30;
    volatile int _gifIndex = 0;
    volatile unsigned long _lastActivityMillis = 0;
    volatile bool _screenSleeping = false;
    volatile bool _screenDimmed = false;
    volatile bool _wakeRequested = false;

    void initPins();
    void initTft();
    void initMatrix();
    void initClockfaces();
    void handleEncoder();
    void handlePowerState();
    void updateUiWidgets();
};

extern ClockApp clockApp;

#endif
