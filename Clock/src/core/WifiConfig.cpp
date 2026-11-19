#include "WifiConfig.h"
#include <Preferences.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <HTTPClient.h>

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
const char *KEY_FACE_IDX    = "face_idx";
const char *KEY_FACE_MASK   = "face_mask";
const char *KEY_FACE_MASK32 = "face_m32";
const char *KEY_FACE_CNT    = "face_cnt";
const char *KEY_JOKE_DATE   = "joke_dt";
const char *KEY_JOKE_TEXT  = "joke_txt";

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

int WifiConfig::loadSelectedFace(int faceCount, int settingsIndex)
{
  Preferences prefs;
  if (!prefs.begin(PREF_NAMESPACE, true)) {
    return 0;
  }
  int idx = prefs.getInt(KEY_FACE_IDX, 0);
  prefs.end();
  if (idx < 0 || idx >= faceCount || idx == settingsIndex) {
    return 0;
  }
  return idx;
}

bool WifiConfig::saveSelectedFace(int index)
{
  Preferences prefs;
  if (!prefs.begin(PREF_NAMESPACE, false)) {
    return false;
  }
  size_t w = prefs.putInt(KEY_FACE_IDX, index);
  prefs.end();
  return w > 0;
}

uint32_t WifiConfig::loadEnabledFacesMask()
{
  Preferences prefs;
  if (!prefs.begin(PREF_NAMESPACE, true)) {
    return 0xFFFFFFFFUL;
  }

  uint32_t mask = 0xFFFFFFFFUL;
  if (prefs.isKey(KEY_FACE_MASK32)) {
    mask = prefs.getUInt(KEY_FACE_MASK32, 0xFFFFFFFFUL);
    if (prefs.isKey(KEY_FACE_CNT)) {
      uint8_t savedCount = prefs.getUChar(KEY_FACE_CNT, 32);
      if (savedCount < 32) {
        mask |= (0xFFFFFFFFUL << savedCount);
      }
    }
  } else if (prefs.isKey(KEY_FACE_MASK)) {
    // Migrate legacy 16-bit mask while preserving previously disabled faces
    // and defaulting any newly added faces to enabled.
    uint16_t legacyMask = prefs.getUShort(KEY_FACE_MASK, 0xFFFF);
    mask = static_cast<uint32_t>(legacyMask) | 0xFFFF0000UL;
    if (prefs.isKey(KEY_FACE_CNT)) {
      uint8_t savedCount = prefs.getUChar(KEY_FACE_CNT, 16);
      if (savedCount < 32) {
        mask |= (0xFFFFFFFFUL << savedCount);
      }
    } else {
      int topBit = -1;
      for (int b = 15; b >= 0; --b) {
        if ((legacyMask & (1U << b)) != 0) {
          topBit = b;
          break;
        }
      }
      if (topBit >= 0 && topBit < 32) {
        mask |= (0xFFFFFFFFUL << topBit);
      }
    }
  }
  prefs.end();
  return mask;
}

bool WifiConfig::saveEnabledFacesMask(uint32_t mask, int settingsIndex)
{
  if (settingsIndex >= 0 && settingsIndex < 32) {
    mask |= (0xFFFFFFFFUL << settingsIndex);
  }
  Preferences prefs;
  if (!prefs.begin(PREF_NAMESPACE, false)) {
    return false;
  }
  size_t w = prefs.putUInt(KEY_FACE_MASK32, mask);
  if (settingsIndex >= 0 && settingsIndex < 32) {
    prefs.putUChar(KEY_FACE_CNT, static_cast<uint8_t>(settingsIndex));
  }
  prefs.end();
  return w > 0;
}

bool WifiConfig::isFaceEnabled(uint32_t mask, int index, int settingsIndex)
{
  if (index == settingsIndex) {
    return true;
  }
  if (index < 0 || index >= 32) {
    return false;
  }
  return (mask & (1UL << index)) != 0;
}

bool WifiConfig::loadDailyJoke(String &dateOut, String &jokeOut)
{
  dateOut = "";
  jokeOut = "";
  Preferences prefs;
  if (!prefs.begin(PREF_NAMESPACE, true)) {
    return false;
  }
  dateOut = prefs.getString(KEY_JOKE_DATE, "");
  jokeOut = prefs.getString(KEY_JOKE_TEXT, "");
  prefs.end();
  return dateOut.length() > 0 && jokeOut.length() > 0;
}

bool WifiConfig::saveDailyJoke(const String &date, const String &joke)
{
  if (date.length() == 0 || joke.length() == 0) {
    return false;
  }
  Preferences prefs;
  if (!prefs.begin(PREF_NAMESPACE, false)) {
    return false;
  }
  prefs.putString(KEY_JOKE_DATE, date);
  prefs.putString(KEY_JOKE_TEXT, joke);
  prefs.end();
  return true;
}

String WifiConfig::getFallbackDailyJoke(int year, int month, int day)
{
  static const char *const KID_JOKES[] = {
    "Why did the math book look sad? Because it had too many problems!",
    "What do you call a bear with no teeth? A gummy bear!",
    "Why couldn't the bicycle stand up by itself? It was two-tired!",
    "What do you call a sleeping dinosaur? A dino-snore!",
    "Why did the golfer bring two pairs of pants? In case he got a hole in one!",
    "What do you call a fake noodle? An impasta!",
    "Why did the banana go to the doctor? Because it wasn't peeling well!",
    "What kind of tree fits in your hand? A palm tree!",
    "Why did the cookie go to the nurse? Because it felt crummy!",
    "What do you call a cow with no legs? Ground beef!",
    "Why are fish so smart? Because they live in schools!",
    "What did one ocean say to the other? Nothing, they just waved!",
    "Why did the picture go to jail? Because it was framed!",
    "What has keys that open no locks? A piano!",
    "Why did the robot go on vacation? To recharge its batteries!",
    "What do you call cheese that isn't yours? Nacho cheese!",
    "Why did the scarecrow win an award? He was outstanding in his field!",
    "What kind of room has no doors or windows? A mushroom!",
    "Why can't you give Elsa a balloon? Because she will let it go!",
    "What do you call a boomerang that won't come back? A stick!"
  };
  constexpr size_t COUNT = sizeof(KID_JOKES) / sizeof(KID_JOKES[0]);
  uint32_t seed = (uint32_t)(year * 372 + month * 31 + day);
  return String(KID_JOKES[seed % COUNT]);
}

namespace {
String extractJsonStringField(const String &json, const char *key)
{
  String pattern = String("\"") + key + "\"";
  int idx = json.indexOf(pattern);
  if (idx < 0) return "";
  int colon = json.indexOf(':', idx + pattern.length());
  if (colon < 0) return "";
  int startQuote = json.indexOf('"', colon + 1);
  if (startQuote < 0) return "";

  String out;
  bool escaped = false;
  for (int i = startQuote + 1; i < (int)json.length(); ++i) {
    char c = json[i];
    if (escaped) {
      if (c == 'n' || c == 'r') out += ' ';
      else if (c == 't') out += ' ';
      else out += c;
      escaped = false;
    } else if (c == '\\') {
      escaped = true;
    } else if (c == '"') {
      return out;
    } else {
      out += c;
    }
  }
  return "";
}
}  // namespace

bool WifiConfig::fetchDailyJokeIfNeeded(int year, int month, int day, bool connectWifiIfNeeded)
{
  char dateBuf[16];
  snprintf(dateBuf, sizeof(dateBuf), "%04d-%02d-%02d", year, month, day);
  String today(dateBuf);

  String cachedDate, cachedJoke;
  if (loadDailyJoke(cachedDate, cachedJoke) && cachedDate == today) {
    return true;
  }

  bool weStartedWifi = false;
  if (WiFi.status() != WL_CONNECTED && connectWifiIfNeeded) {
    String ssid, pass;
    if (load(ssid, pass)) {
      WiFi.persistent(false);
      WiFi.mode(WIFI_STA);
      WiFi.begin(ssid.c_str(), pass.c_str());
      unsigned long start = millis();
      while (WiFi.status() != WL_CONNECTED && (millis() - start) < 6000UL) {
        delay(250);
      }
      weStartedWifi = true;
    }
  }

  String fetchedJoke;
  if (WiFi.status() == WL_CONNECTED) {
    WiFiClientSecure client;
    client.setInsecure();
    HTTPClient http;
    http.setTimeout(3000);
    if (http.begin(client, "https://v2.jokeapi.dev/joke/Pun,Misc?blacklistFlags=nsfw,religious,political,racist,sexist,explicit&safe-mode")) {
      int code = http.GET();
      if (code == HTTP_CODE_OK) {
        // Cap HTTP body read to 1024 bytes to prevent heap exhaustion on unexpected responses
        String payload;
        payload.reserve(512);
        WiFiClient *stream = http.getStreamPtr();
        unsigned long readStart = millis();
        while (stream && payload.length() < 1024 && (millis() - readStart) < 3000UL) {
          while (stream->available() && payload.length() < 1024) {
            payload += static_cast<char>(stream->read());
          }
          if (!http.connected() && !stream->available()) {
            break;
          }
          delay(10);
        }
        String single = extractJsonStringField(payload, "joke");
        if (single.length() > 0) {
          fetchedJoke = single;
        } else {
          String setup = extractJsonStringField(payload, "setup");
          String delivery = extractJsonStringField(payload, "delivery");
          if (setup.length() > 0 && delivery.length() > 0) {
            fetchedJoke = setup + "  " + delivery;
          }
        }
      }
      http.end();
    }
  }

  if (weStartedWifi) {
    WiFi.disconnect(true);
    WiFi.mode(WIFI_OFF);
  }

  if (fetchedJoke.length() == 0) {
    fetchedJoke = getFallbackDailyJoke(year, month, day);
  }

  // Sanitize to printable ASCII and cap length
  String cleanJoke;
  cleanJoke.reserve(160);
  for (size_t i = 0; i < fetchedJoke.length() && cleanJoke.length() < 160; ++i) {
    unsigned char c = static_cast<unsigned char>(fetchedJoke[i]);
    if (c == '\n' || c == '\r' || c == '\t') {
      cleanJoke += ' ';
    } else if (c >= 0x20 && c <= 0x7E) {
      cleanJoke += static_cast<char>(c);
    }
  }
  cleanJoke.trim();
  if (cleanJoke.length() == 0) {
    cleanJoke = getFallbackDailyJoke(year, month, day);
  }
  saveDailyJoke(today, cleanJoke);
  return true;
}
