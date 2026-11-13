#ifndef WIFI_CONFIG_H
#define WIFI_CONFIG_H

#include <Arduino.h>

/**
 * Persists and retrieves home WiFi credentials in ESP32 NVS storage.
 */
class WifiConfig
{
  public:
    static bool load(String &ssid, String &password);
    static bool save(const String &ssid, const String &password);
    static bool hasSavedCredentials();
};

#endif
