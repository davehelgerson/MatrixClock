#include "I2CBus.h"

namespace {
  SemaphoreHandle_t s_i2cMutex = nullptr;
}

namespace I2CBus {

void init() {
  if (!s_i2cMutex) {
    s_i2cMutex = xSemaphoreCreateMutex();
  }
}

bool lock(TickType_t timeoutTicks) {
  if (!s_i2cMutex) {
    return true;
  }
  return xSemaphoreTake(s_i2cMutex, timeoutTicks) == pdTRUE;
}

void unlock() {
  if (s_i2cMutex) {
    xSemaphoreGive(s_i2cMutex);
  }
}

Guard::Guard(TickType_t timeoutTicks)
    : _locked(lock(timeoutTicks)) {}

Guard::~Guard() {
  if (_locked) {
    unlock();
  }
}

} // namespace I2CBus

