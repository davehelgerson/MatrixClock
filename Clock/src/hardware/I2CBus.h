#ifndef I2C_BUS_H
#define I2C_BUS_H

#include <Arduino.h>
#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>

/**
 * Shared FreeRTOS mutex for the Wire I2C bus accessed by both the touch
 * controller (Core 0) and the PCF8563 RTC (Core 1).
 */
namespace I2CBus {
  void init();
  bool lock(TickType_t timeoutTicks = portMAX_DELAY);
  void unlock();

  class Guard {
    public:
      explicit Guard(TickType_t timeoutTicks = pdMS_TO_TICKS(50));
      ~Guard();
      bool locked() const { return _locked; }
    private:
      bool _locked;
  };
}

#endif

