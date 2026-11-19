#ifndef WIFI_CONFIG_H
#define WIFI_CONFIG_H

#include <Arduino.h>
#include <vector>
#include "ClockfaceRegistry.h"

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

    static constexpr const char *DEFAULT_MSG_MENU_NAME = "Messages";
    static constexpr const char *DEFAULT_MSG_TOP_TEXT  = "To Logan:";
    static constexpr const char *DEFAULT_MSG_BOT_TEXT  = "- Dad";

    static constexpr int JOKE_FACE_INDEX     = ClockfaceRegistry::JOKE_INDEX;
    static constexpr int SETTINGS_FACE_INDEX = ClockfaceRegistry::SETTINGS_INDEX;

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

    static String loadMessagesMenuName();
    static bool saveMessagesMenuName(const String &name);
    static String loadMessagesTopText();
    static bool saveMessagesTopText(const String &text);
    static String loadMessagesBottomText();
    static bool saveMessagesBottomText(const String &text);
    static void loadDefaultMessages(std::vector<String> &out);
    static void loadMessages(std::vector<String> &out);
    static bool saveMessages(const std::vector<String> &sayings);

    static int loadSelectedFace(int faceCount, int settingsIndex);
    static bool saveSelectedFace(int index);

    static uint32_t loadEnabledFacesMask();
    static bool saveEnabledFacesMask(uint32_t mask, int settingsIndex);
    static bool isFaceEnabled(uint32_t mask, int index, int settingsIndex);

    static bool loadDailyJoke(String &dateOut, String &jokeOut);
    static bool saveDailyJoke(const String &date, const String &joke);
    static String getFallbackDailyJoke(int year, int month, int day);
    static bool fetchDailyJokeIfNeeded(int year, int month, int day, bool connectWifiIfNeeded);
};

#endif
