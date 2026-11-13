/*
Library version:
Arduino IDE 2.3.4
esp32 V3.1.1
GFX Library for Arduino v1.5.5
lvgl v8.3.11
ESP32 HUB75 LED MATRIX PANEL DMA Display v3.0.11
Adafruit GFX Library v1.11.11
Adafruit BusIO v1.16.2
RTClib v2.1.4

Tools:
USB CDC On Boot: Enabled
Flash size: 16MB(128Mb)
Partition Schrme: 16M Flash(3MB APP/9.9MB FATFS)
PSRAM: OPI PSRAM
*/

#include "src/app/ClockApp.h"

void setup()
{
  clockApp.begin();
}

void loop()
{
}
