#ifndef WIFI_CONFIG_H
#define WIFI_CONFIG_H

#include <Arduino.h>
#include <vector>

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
    static constexpr uint8_t  DEFAULT_TZ_INDEX     = 0;
    static constexpr bool     DEFAULT_DST_ENABLED  = true;

    static constexpr const char *DEFAULT_DAD_MENU_NAME = "Dad";
    static constexpr const char *DEFAULT_DAD_TOP_TEXT  = "To Logan:";
    static constexpr const char *DEFAULT_DAD_BOT_TEXT  = "- Dad";

    static uint16_t normalizeTimeoutMinutes(uint16_t minutes);

    static bool load(String &ssid, String &password);
    static bool save(const String &ssid, const String &password);
    static bool hasSavedCredentials();

    static uint8_t loadTftRotation();
    static bool saveTftRotation(uint8_t rotation);

    static uint16_t loadDimMinutes();
    static uint16_t loadOffMinutes();
    static bool savePowerTimeouts(uint16_t dimMinutes, uint16_t offMinutes);

    static uint8_t getTimeZoneCount();
    static const char *getTimeZoneLabel(uint8_t index);
    static uint8_t loadTimeZoneIndex();
    static bool loadDstEnabled();
    static bool saveTimeZone(uint8_t index, bool dstEnabled);
    static const char *getPosixTimeZone();

    static String loadDadMenuName();
    static bool saveDadMenuName(const String &name);
    static String loadDadTopText();
    static bool saveDadTopText(const String &text);
    static String loadDadBottomText();
    static bool saveDadBottomText(const String &text);
    static void loadDefaultDadSayings(std::vector<String> &out);
    static void loadDadSayings(std::vector<String> &out);
    static bool saveDadSayings(const std::vector<String> &sayings);
};

#endif
