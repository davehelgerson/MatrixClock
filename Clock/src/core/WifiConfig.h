#ifndef WIFI_CONFIG_H
#define WIFI_CONFIG_H

#include <Arduino.h>

/**
 * Persists and retrieves home WiFi credentials and device display settings
 * in ESP32 NVS storage.
 */
class WifiConfig
{
  public:
    static constexpr uint8_t  DEFAULT_TFT_ROTATION = 3;
    static constexpr uint16_t DEFAULT_DIM_MINUTES  = 5;
    static constexpr uint16_t DEFAULT_OFF_MINUTES  = 15;
    static constexpr uint16_t MIN_TIMEOUT_MINUTES  = 1;
    static constexpr uint16_t MAX_TIMEOUT_MINUTES  = 720;

    static uint16_t normalizeTimeoutMinutes(uint16_t minutes);

    static bool load(String &ssid, String &password);
    static bool save(const String &ssid, const String &password);
    static bool hasSavedCredentials();

    static uint8_t loadTftRotation();
    static bool saveTftRotation(uint8_t rotation);

    static uint16_t loadDimMinutes();
    static uint16_t loadOffMinutes();
    static bool savePowerTimeouts(uint16_t dimMinutes, uint16_t offMinutes);
};

#endif
