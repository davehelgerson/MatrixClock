#ifndef WIFI_CONFIG_H
#define WIFI_CONFIG_H

#include <Arduino.h>

/**
 * Persists and retrieves home WiFi credentials in ESP32 NVS storage,
 * falling back to built-in defaults if no credentials have been saved.
 */
class WifiConfig
{
  public:
    static constexpr const char *DEFAULT_SSID     = "HWIFI_LAB";
    static constexpr const char *DEFAULT_PASSWORD = "TestTest1";

    static void load(String &ssid, String &password);
    static bool save(const String &ssid, const String &password);
    static bool hasSavedCredentials();
};

#endif

