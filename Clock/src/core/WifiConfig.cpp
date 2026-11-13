#include "WifiConfig.h"
#include <Preferences.h>

namespace {
const char *PREF_NAMESPACE = "wifi";
const char *KEY_SSID       = "ssid";
const char *KEY_PASSWORD   = "pass";
const char *KEY_ROTATION   = "rot";
const char *KEY_DIM_MIN    = "dim_min";
const char *KEY_OFF_MIN    = "off_min";
}

bool WifiConfig::load(String &ssid, String &password)
{
  ssid = "";
  password = "";

  Preferences prefs;
  if (!prefs.begin(PREF_NAMESPACE, true)) {
    return false;
  }

  String savedSsid = prefs.getString(KEY_SSID, "");
  String savedPass = prefs.getString(KEY_PASSWORD, "");
  prefs.end();

  if (savedSsid.length() == 0) {
    return false;
  }

  ssid = savedSsid;
  password = savedPass;
  return true;
}

bool WifiConfig::save(const String &ssid, const String &password)
{
  if (ssid.length() == 0) {
    return false;
  }

  Preferences prefs;
  if (!prefs.begin(PREF_NAMESPACE, false)) {
    return false;
  }

  size_t ssidWritten = prefs.putString(KEY_SSID, ssid);
  prefs.putString(KEY_PASSWORD, password);
  prefs.end();

  return ssidWritten > 0;
}

bool WifiConfig::hasSavedCredentials()
{
  Preferences prefs;
  if (!prefs.begin(PREF_NAMESPACE, true)) {
    return false;
  }
  String savedSsid = prefs.getString(KEY_SSID, "");
  prefs.end();
  return savedSsid.length() > 0;
}

uint8_t WifiConfig::loadTftRotation()
{
  Preferences prefs;
  if (!prefs.begin(PREF_NAMESPACE, true)) {
    return DEFAULT_TFT_ROTATION;
  }
  uint8_t rot = prefs.getUChar(KEY_ROTATION, DEFAULT_TFT_ROTATION);
  prefs.end();
  return (rot <= 3) ? rot : DEFAULT_TFT_ROTATION;
}

bool WifiConfig::saveTftRotation(uint8_t rotation)
{
  if (rotation > 3) {
    return false;
  }

  Preferences prefs;
  if (!prefs.begin(PREF_NAMESPACE, false)) {
    return false;
  }
  size_t written = prefs.putUChar(KEY_ROTATION, rotation);
  prefs.end();
  return written > 0;
}

uint16_t WifiConfig::normalizeTimeoutMinutes(uint16_t minutes)
{
  if (minutes < MIN_TIMEOUT_MINUTES) {
    return MIN_TIMEOUT_MINUTES;
  }
  if (minutes > MAX_TIMEOUT_MINUTES) {
    return MAX_TIMEOUT_MINUTES;
  }
  return minutes;
}

uint16_t WifiConfig::loadDimMinutes()
{
  Preferences prefs;
  if (!prefs.begin(PREF_NAMESPACE, true)) {
    return DEFAULT_DIM_MINUTES;
  }
  uint16_t val = prefs.getUShort(KEY_DIM_MIN, DEFAULT_DIM_MINUTES);
  prefs.end();
  return normalizeTimeoutMinutes(val);
}

uint16_t WifiConfig::loadOffMinutes()
{
  Preferences prefs;
  if (!prefs.begin(PREF_NAMESPACE, true)) {
    return DEFAULT_OFF_MINUTES;
  }
  uint16_t val = prefs.getUShort(KEY_OFF_MIN, DEFAULT_OFF_MINUTES);
  prefs.end();
  return normalizeTimeoutMinutes(val);
}

bool WifiConfig::savePowerTimeouts(uint16_t dimMinutes, uint16_t offMinutes)
{
  if (dimMinutes > MAX_TIMEOUT_MINUTES || offMinutes > MAX_TIMEOUT_MINUTES) {
    return false;
  }

  dimMinutes = normalizeTimeoutMinutes(dimMinutes);
  offMinutes = normalizeTimeoutMinutes(offMinutes);

  Preferences prefs;
  if (!prefs.begin(PREF_NAMESPACE, false)) {
    return false;
  }
  size_t w1 = prefs.putUShort(KEY_DIM_MIN, dimMinutes);
  size_t w2 = prefs.putUShort(KEY_OFF_MIN, offMinutes);
  prefs.end();
  return (w1 > 0) && (w2 > 0);
}
