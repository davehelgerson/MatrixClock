/*
Target / PlatformIO dependencies (see platformio.ini):
  platform: espressif32@6.5.0 (board: esp32-s3-devkitc-1)
  Arduino_GFX v1.3.1
  lvgl v8.3.11
  ESP32 HUB75 LED MATRIX PANEL DMA Display v3.0.11
  Adafruit GFX Library v1.11.11
  RTClib v2.1.4

Tools:
  USB CDC On Boot: Enabled
  Flash size: 16MB(128Mb)
  Partition Scheme: 16M Flash(3MB APP/9.9MB FATFS)
  PSRAM: OPI PSRAM
*/

#include "src/app/ClockApp.h"

void setup()
{
  clockApp.begin();
}

void loop()
{
  // Work runs on dedicated FreeRTOS tasks (Task_TFT, Task_Control, Task_Time).
  vTaskDelete(nullptr);
}
