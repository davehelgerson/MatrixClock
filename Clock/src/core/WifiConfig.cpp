#include "WifiConfig.h"
#include <Preferences.h>

namespace {
const char *PREF_NAMESPACE = "wifi";
const char *KEY_SSID       = "ssid";
const char *KEY_PASSWORD   = "pass";
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
