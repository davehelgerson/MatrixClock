#ifndef ClockApp_h
#define ClockApp_h

#include <Arduino.h>
#include <esp_timer.h>
#include <Arduino_GFX_Library.h>
#include <ESP32-HUB75-MatrixPanel-I2S-DMA.h>
#include <lvgl.h>
#include "../core/CWDateTime.h"
#include "../core/IClockface.h"

class ClockApp {
  public:
    void begin();

    // FreeRTOS task loops & timer hooks
    void runTftTask();
    void runControlTask();
    void runTimeTask();
    void pollEncoder();
    void flushDisplay(lv_disp_drv_t *disp, const lv_area_t *area, lv_color_t *color_p);
    void readTouchpad(lv_indev_drv_t *indev_driver, lv_indev_data_t *data);
    void setTftRotation(uint8_t rotation);
    void setPowerTimeouts(uint16_t dimMinutes, uint16_t offMinutes);
    void refreshMenuLabels();

  private:
    static const uint16_t TFT_WIDTH  = 240;
    static const uint16_t TFT_HEIGHT = 240;
    static const uint8_t  PANEL_RES_X = 64;
    static const uint8_t  PANEL_RES_Y = 64;
    static const uint8_t  PANEL_CHAIN = 1;
    static const int     CLOCKFACE_COUNT = 13;

    Arduino_ESP32SPI *_bus = nullptr;
    Arduino_GFX *_gfx = nullptr;
    MatrixPanel_I2S_DMA *_dmaDisplay = nullptr;

    CWDateTime _dateTime;
    IClockface *_clockfaces[CLOCKFACE_COUNT] = {nullptr};
    int _visibleFaces[CLOCKFACE_COUNT] = {0};
    int _visibleCount = 0;

    lv_disp_draw_buf_t _drawBuf;
    lv_color_t _buf[TFT_WIDTH * TFT_HEIGHT / 10];

    portMUX_TYPE _encoderMux = portMUX_INITIALIZER_UNLOCKED;
    esp_timer_handle_t _encoderTimer = nullptr;
    volatile int _encoderDelta = 0;
    uint8_t _encoderState = 0;
    uint8_t _encoderMask = 0;
    uint8_t _lastRawAb = 3;
    uint8_t _debouncedAb = 3;
    uint8_t _rawStableCount = 0;

    volatile int _brightness = 30;
    volatile int _gifIndex = 0;
    volatile unsigned long _lastActivityMillis = 0;
    volatile bool _screenSleeping = false;
    volatile bool _screenDimmed = false;
    volatile bool _wakeRequested = false;
    volatile uint8_t _tftRotation = 3;
    volatile bool _rotationDirty = false;
    volatile bool _rollerOptionsDirty = false;
    volatile uint16_t _dimMinutes = 5;
    volatile uint16_t _offMinutes = 15;

    // Cross-task requests: hardware is only touched by the task that owns it.
    // _gfx (SPI TFT) belongs to Task_TFT; _dmaDisplay belongs to Task_Time.
    volatile bool _tftOnRequested = true;
    volatile int _matrixBrightnessRequested = -1; // 0-255, -1 = no change pending
    bool _touchSuppressed = false;

    void initPins();
    void initTft();
    void initMatrix();
    void initClockfaces();
    void handleEncoder();
    void handlePowerState();
    void updateUiWidgets();
    void requestMatrixBrightness(int percent);
    void rebuildVisibleMenu(bool animateOff);
    int rollerPositionForFace(int faceIndex) const;
};

extern ClockApp clockApp;

#endif
