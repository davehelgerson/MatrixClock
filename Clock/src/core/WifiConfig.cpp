#include "WifiConfig.h"
#include <Preferences.h>

namespace {
const char *PREF_NAMESPACE = "wifi";
const char *DAD_NAMESPACE  = "dad";
const char *KEY_DAD_MENU   = "menu";
const char *KEY_DAD_TOP    = "top";
const char *KEY_DAD_BOT    = "bot";
const char *KEY_DAD_SAY    = "sayings";

const char *const DEFAULT_DAD_SAYINGS[] = {
  "I love you!",
  "You are the best son!",
  "You are amazing!",
  "I love your creativity!",
  "I'm proud of you!",
  "You're my favorite human!",
  "You are the son I always wanted!",
  "You make me happy every day!",
  "You're braver than you know!",
  "I'm so happy that you are my son!",
  "I'm here for you!",
  "I'm lucky to be your dad!",
  "I love adventures with you!",
  "Your smile brightens my day!",
  "Remember to always be kind!",
  "I'm always cheering for you!",
  "Thank you for being you!",
  "You are courageous and strong!"
};
constexpr size_t DEFAULT_DAD_SAYINGS_COUNT = sizeof(DEFAULT_DAD_SAYINGS) / sizeof(DEFAULT_DAD_SAYINGS[0]);
const char *KEY_SSID       = "ssid";
const char *KEY_PASSWORD   = "pass";
const char *KEY_ROTATION   = "rot";
const char *KEY_DIM_MIN    = "dim_min";
const char *KEY_OFF_MIN    = "off_min";
const char *KEY_TZ_INDEX   = "tz_idx";
const char *KEY_DST        = "dst";

struct TimeZoneOption {
  const char *label;
  const char *posixDst;
  const char *posixStd;
};

const TimeZoneOption TIME_ZONES[] = {
  { "US Eastern (ET)",       "EST5EDT,M3.2.0,M11.1.0",     "EST5"    },
  { "US Central (CT)",       "CST6CDT,M3.2.0,M11.1.0",     "CST6"    },
  { "US Mountain (MT)",      "MST7MDT,M3.2.0,M11.1.0",     "MST7"    },
  { "US Pacific (PT)",       "PST8PDT,M3.2.0,M11.1.0",     "PST8"    },
  { "US Alaska (AKT)",       "AKST9AKDT,M3.2.0,M11.1.0",   "AKST9"   },
  { "Hawaii (HST)",          "HST10",                      "HST10"   },
  { "Atlantic (AT)",         "AST4ADT,M3.2.0,M11.1.0",     "AST4"    },
  { "UK / London (GMT/BST)", "GMT0BST,M3.5.0/1,M10.5.0",   "GMT0"    },
  { "Central Europe (CET)",  "CET-1CEST,M3.5.0,M10.5.0/3", "CET-1"   },
  { "UTC (GMT)",             "UTC0",                       "UTC0"    },
  { "Japan (JST)",           "JST-9",                      "JST-9"   },
  { "Sydney (AEST/AEDT)",    "AEST-10AEDT,M10.1.0,M4.1.0/3", "AEST-10" },
};
constexpr uint8_t TIME_ZONE_COUNT = sizeof(TIME_ZONES) / sizeof(TIME_ZONES[0]);
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

uint8_t WifiConfig::getTimeZoneCount()
{
  return TIME_ZONE_COUNT;
}

const char *WifiConfig::getTimeZoneLabel(uint8_t index)
{
  if (index >= TIME_ZONE_COUNT) {
    index = DEFAULT_TZ_INDEX;
  }
  return TIME_ZONES[index].label;
}

uint8_t WifiConfig::loadTimeZoneIndex()
{
  Preferences prefs;
  if (!prefs.begin(PREF_NAMESPACE, true)) {
    return DEFAULT_TZ_INDEX;
  }
  uint8_t idx = prefs.getUChar(KEY_TZ_INDEX, DEFAULT_TZ_INDEX);
  prefs.end();
  if (idx >= TIME_ZONE_COUNT) {
    return DEFAULT_TZ_INDEX;
  }
  return idx;
}

bool WifiConfig::loadDstEnabled()
{
  Preferences prefs;
  if (!prefs.begin(PREF_NAMESPACE, true)) {
    return DEFAULT_DST_ENABLED;
  }
  bool enabled = prefs.getBool(KEY_DST, DEFAULT_DST_ENABLED);
  prefs.end();
  return enabled;
}

bool WifiConfig::saveTimeZone(uint8_t index, bool dstEnabled)
{
  if (index >= TIME_ZONE_COUNT) {
    return false;
  }

  Preferences prefs;
  if (!prefs.begin(PREF_NAMESPACE, false)) {
    return false;
  }
  size_t w1 = prefs.putUChar(KEY_TZ_INDEX, index);
  size_t w2 = prefs.putBool(KEY_DST, dstEnabled);
  prefs.end();
  return (w1 > 0) && (w2 > 0);
}

const char *WifiConfig::getPosixTimeZone()
{
  uint8_t idx = loadTimeZoneIndex();
  bool dst = loadDstEnabled();
  return dst ? TIME_ZONES[idx].posixDst : TIME_ZONES[idx].posixStd;
}

static String loadDadStr(const char *key, const char *defVal)
{
  Preferences prefs;
  if (!prefs.begin(DAD_NAMESPACE, true)) {
    return String(defVal);
  }
  String val = prefs.getString(key, defVal);
  prefs.end();
  return val;
}

static bool saveDadStr(const char *key, const String &val)
{
  Preferences prefs;
  if (!prefs.begin(DAD_NAMESPACE, false)) {
    return false;
  }
  prefs.putString(key, val);
  prefs.end();
  return true;
}

String WifiConfig::loadDadMenuName()
{
  String s = loadDadStr(KEY_DAD_MENU, DEFAULT_DAD_MENU_NAME);
  s.replace("\n", " ");
  s.replace("\r", "");
  s.trim();
  return (s.length() > 0) ? s : String(DEFAULT_DAD_MENU_NAME);
}

bool WifiConfig::saveDadMenuName(const String &name)
{
  String s = name;
  s.replace("\n", " ");
  s.replace("\r", "");
  s.trim();
  if (s.length() == 0) {
    s = DEFAULT_DAD_MENU_NAME;
  }
  return saveDadStr(KEY_DAD_MENU, s);
}

String WifiConfig::loadDadTopText()
{
  return loadDadStr(KEY_DAD_TOP, DEFAULT_DAD_TOP_TEXT);
}

bool WifiConfig::saveDadTopText(const String &text)
{
  return saveDadStr(KEY_DAD_TOP, text);
}

String WifiConfig::loadDadBottomText()
{
  return loadDadStr(KEY_DAD_BOT, DEFAULT_DAD_BOT_TEXT);
}

bool WifiConfig::saveDadBottomText(const String &text)
{
  return saveDadStr(KEY_DAD_BOT, text);
}

void WifiConfig::loadDefaultDadSayings(std::vector<String> &out)
{
  out.clear();
  out.reserve(DEFAULT_DAD_SAYINGS_COUNT);
  for (size_t i = 0; i < DEFAULT_DAD_SAYINGS_COUNT; ++i) {
    out.emplace_back(DEFAULT_DAD_SAYINGS[i]);
  }
}

void WifiConfig::loadDadSayings(std::vector<String> &out)
{
  out.clear();
  Preferences prefs;
  if (!prefs.begin(DAD_NAMESPACE, true)) {
    loadDefaultDadSayings(out);
    return;
  }

  if (!prefs.isKey(KEY_DAD_SAY)) {
    prefs.end();
    loadDefaultDadSayings(out);
    return;
  }

  String raw = prefs.getString(KEY_DAD_SAY, "");
  prefs.end();

  int start = 0;
  while (start <= (int)raw.length()) {
    int nl = raw.indexOf('\n', start);
    String line = (nl < 0) ? raw.substring(start) : raw.substring(start, nl);
    line.replace("\r", "");
    line.trim();
    if (line.length() > 0) {
      out.push_back(line);
    }
    if (nl < 0) break;
    start = nl + 1;
  }

  if (out.empty()) {
    loadDefaultDadSayings(out);
  }
}

bool WifiConfig::saveDadSayings(const std::vector<String> &sayings)
{
  String joined;
  for (size_t i = 0; i < sayings.size(); ++i) {
    String line = sayings[i];
    line.replace("\r", "");
    line.replace("\n", " ");
    line.trim();
    if (line.length() == 0) continue;
    if (joined.length() > 0) joined += "\n";
    joined += line;
  }

  Preferences prefs;
  if (!prefs.begin(DAD_NAMESPACE, false)) {
    return false;
  }
  if (joined.length() == 0) {
    prefs.remove(KEY_DAD_SAY);
  } else {
    prefs.putString(KEY_DAD_SAY, joined);
  }
  prefs.end();
  return true;
}
