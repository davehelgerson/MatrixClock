#include "WifiClockface.h"
#include "../core/WifiConfig.h"
#include <WiFi.h>
#include <vector>
#include <algorithm>

namespace {
const uint16_t COLOR_BG        = 0x0000;
const uint16_t COLOR_WHITE     = 0xFFFF;
const uint16_t COLOR_CYAN      = 0x07FF;
const uint16_t COLOR_BLUE      = 0x3C9F;
const uint16_t COLOR_DIM_BLUE  = 0x1188;
const uint16_t COLOR_GREEN     = 0x07E0;
const uint16_t COLOR_AMBER     = 0xFD20;
const uint16_t COLOR_GRAY      = 0x8410;

const IPAddress AP_IP(192, 168, 4, 1);
const IPAddress AP_NETMASK(255, 255, 255, 0);
const uint16_t DNS_PORT  = 53;
const uint16_t HTTP_PORT = 80;
const unsigned long ANIM_INTERVAL_MS = 300;
const unsigned long STA_TEST_TIMEOUT_MS = 8000;

struct ScannedNetwork {
  String ssid;
  int32_t rssi;
  bool secure;
};

const char INDEX_HTML[] PROGMEM = R"rawliteral(<!DOCTYPE html>
<html lang="en">
<head>
<meta charset="utf-8">
<meta name="viewport" content="width=device-width,initial-scale=1">
<title>MatrixClock Settings</title>
<style>
  *{box-sizing:border-box;font-family:-apple-system,BlinkMacSystemFont,"Segoe UI",Roboto,sans-serif}
  body{margin:0;padding:16px;background:#0f172a;color:#e2e8f0;display:flex;justify-content:center}
  .stack{width:100%;max-width:420px;display:flex;flex-direction:column;gap:14px}
  .card{width:100%;background:#1e293b;border:1px solid #334155;border-radius:14px;padding:20px;box-shadow:0 10px 25px rgba(0,0,0,.35)}
  h1{margin:0 0 6px;font-size:1.25rem;color:#38bdf8}
  h2{margin:0 0 8px;font-size:1.05rem;color:#38bdf8}
  p.sub{margin:0 0 16px;font-size:.9rem;color:#94a3b8}
  .saved{background:#0f172a;border:1px solid #334155;border-radius:8px;padding:10px 12px;margin-bottom:14px;font-size:.88rem}
  label{display:block;font-size:.85rem;color:#cbd5e1;margin:12px 0 6px}
  select,input{width:100%;padding:10px 12px;border-radius:8px;border:1px solid #475569;background:#0f172a;color:#f8fafc;font-size:1rem}
  .row{display:flex;gap:8px;align-items:center}
  .btn{width:100%;margin-top:16px;padding:11px;border:0;border-radius:8px;font-size:1rem;font-weight:600;cursor:pointer;background:#0284c7;color:#fff}
  .btn:hover{background:#0369a1}
  .btn-sec{width:auto;margin-top:0;padding:10px 12px;background:#334155;color:#e2e8f0;white-space:nowrap}
  .btn-sec:hover{background:#475569}
  .chk{display:flex;align-items:center;gap:8px;margin-top:8px;font-size:.85rem;color:#94a3b8}
  .chk input{width:auto}
  .status{margin-top:10px;font-size:.85rem;color:#38bdf8;min-height:1.2em}
</style>
</head>
<body>
<div class="stack">
<div class="card">
  <h1>MatrixClock Settings</h1>
  <p class="sub">Select your home WiFi network and save credentials.</p>
  <div class="saved">Saved network: <strong id="savedSsid">__SAVED_SSID__</strong></div>
  <form method="POST" action="/save">
    <label for="netList">Available Networks</label>
    <div class="row">
      <select id="netList" onchange="pickNetwork(this.value)">
        <option value="">Scanning...</option>
      </select>
      <button type="button" class="btn btn-sec" onclick="loadScan(true)">Rescan</button>
    </div>
    <div class="status" id="scanMsg">Scanning nearby WiFi networks...</div>

    <label for="ssid">WiFi Name (SSID)</label>
    <input id="ssid" name="ssid" type="text" required maxlength="32" value="__SAVED_SSID_ATTR__" placeholder="Enter or pick WiFi name">

    <label for="password">Password</label>
    <input id="password" name="password" type="password" maxlength="64" placeholder="Leave blank if open network">
    <label class="chk"><input type="checkbox" onclick="document.getElementById('password').type=this.checked?'text':'password'"> Show password</label>

    <button type="submit" class="btn">Save &amp; Test WiFi</button>
  </form>
</div>

<div class="card">
  <h2>Display &amp; Power</h2>
  <p class="sub">Rotate the menu dial or adjust idle dim and display off timers.</p>
  <form method="POST" action="/display">
    <label for="rotation">Menu Dial Rotation</label>
    <select id="rotation" name="rotation">
      <option value="3"__ROT_3__>0&deg; (Default)</option>
      <option value="0"__ROT_0__>90&deg; Clockwise</option>
      <option value="1"__ROT_1__>180&deg; (Upside Down)</option>
      <option value="2"__ROT_2__>270&deg; Clockwise</option>
    </select>

    <label for="dimMinutes">Dim Clock After (1&ndash;720 min)</label>
    <input id="dimMinutes" name="dimMinutes" type="number" min="0" max="720" required value="__DIM_MIN__">

    <label for="offMinutes">Turn Display Off After (1&ndash;720 min)</label>
    <input id="offMinutes" name="offMinutes" type="number" min="0" max="720" required value="__OFF_MIN__">

    <button type="submit" class="btn">Apply &amp; Save Settings</button>
  </form>
</div>
</div>
<script>
function pickNetwork(val){
  if(val){document.getElementById('ssid').value=val;}
}
function loadScan(force){
  var msg=document.getElementById('scanMsg');
  var sel=document.getElementById('netList');
  msg.textContent='Scanning nearby WiFi networks...';
  fetch(force?'/scan?rescan=1':'/scan')
    .then(function(r){return r.json();})
    .then(function(data){
      if(data.scanning){
        setTimeout(function(){loadScan(false);},700);
        return;
      }
      var cur=document.getElementById('ssid').value;
      sel.innerHTML='<option value="">-- Choose a network --</option>';
      if(!data.networks||data.networks.length===0){
        msg.textContent='No networks found. Tap Rescan or type SSID manually.';
        return;
      }
      data.networks.forEach(function(n){
        var opt=document.createElement('option');
        opt.value=n.ssid;
        opt.textContent=n.ssid+' ('+n.rssi+' dBm'+(n.secure?', lock':'')+')';
        if(n.ssid===cur){opt.selected=true;}
        sel.appendChild(opt);
      });
      msg.textContent='Found '+data.networks.length+' network(s).';
    })
    .catch(function(){
      msg.textContent='Scan error. Tap Rescan to try again.';
    });
}
loadScan(false);
</script>
</body>
</html>)rawliteral";
}

WifiClockface::WifiClockface(Adafruit_GFX *display,
                             RotationCallback onRotationChanged,
                             PowerTimeoutCallback onPowerTimeoutsChanged)
  : _display(display),
    _onRotationChanged(onRotationChanged),
    _onPowerTimeoutsChanged(onPowerTimeoutsChanged)
{
}

WifiClockface::~WifiClockface()
{
  teardown();
}

void WifiClockface::setup(CWDateTime *dateTime)
{
  teardown();

  _dateTime = dateTime;
  _active = true;
  _animFrame = 0;
  _lastAnimMillis = 0;
  _statusLine = "SETTINGS";
  _statusColor = COLOR_GREEN;
  _dirtyDisplay = true;

  String dummyPass;
  WifiConfig::load(_savedSsid, dummyPass);

  WiFi.persistent(false);
  WiFi.mode(WIFI_AP_STA);
  WiFi.softAPConfig(AP_IP, AP_IP, AP_NETMASK);
  WiFi.softAP(AP_SSID);

  _dnsServer.reset(new DNSServer());
  _dnsServer->start(DNS_PORT, "*", AP_IP);

  _server.reset(new WebServer(HTTP_PORT));
  _server->on("/", HTTP_GET, [this]() { handleRoot(); });
  _server->on("/scan", HTTP_GET, [this]() { handleScan(); });
  _server->on("/save", HTTP_POST, [this]() { handleSave(); });
  _server->on("/display", HTTP_POST, [this]() { handleDisplay(); });
  _server->onNotFound([this]() { handleNotFound(); });
  _server->begin();

  WiFi.scanDelete();
  WiFi.scanNetworks(true);

  drawScreen();
}

void WifiClockface::update()
{
  if (!_active) {
    return;
  }

  if (_dnsServer) {
    _dnsServer->processNextRequest();
  }
  if (_server) {
    _server->handleClient();
  }

  unsigned long now = millis();
  if (_dirtyDisplay || (now - _lastAnimMillis >= ANIM_INTERVAL_MS)) {
    _lastAnimMillis = now;
    if (_dirtyDisplay) {
      drawScreen();
      _dirtyDisplay = false;
    } else {
      _animFrame = (_animFrame + 1) % 4;
      drawAntennaIcon(_animFrame);
    }
  }
}

void WifiClockface::teardown()
{
  bool wasActive = _active;
  _active = false;

  if (_server) {
    _server->stop();
    _server.reset();
  }
  if (_dnsServer) {
    _dnsServer->stop();
    _dnsServer.reset();
  }

  WiFi.scanDelete();
  WiFi.softAPdisconnect(true);
  WiFi.disconnect(true);
  WiFi.mode(WIFI_OFF);

  if (wasActive && _display) {
    _display->fillScreen(COLOR_BG);
  }
}

void WifiClockface::handleRoot()
{
  String dummyPass;
  WifiConfig::load(_savedSsid, dummyPass);

  String html = FPSTR(INDEX_HTML);
  String savedLabel = (_savedSsid.length() > 0) ? escapeHtml(_savedSsid) : String("Not configured");
  html.replace("__SAVED_SSID__", savedLabel);
  html.replace("__SAVED_SSID_ATTR__", escapeHtml(_savedSsid));

  uint8_t rot = WifiConfig::loadTftRotation();
  html.replace("__ROT_3__", (rot == 3) ? " selected" : "");
  html.replace("__ROT_0__", (rot == 0) ? " selected" : "");
  html.replace("__ROT_1__", (rot == 1) ? " selected" : "");
  html.replace("__ROT_2__", (rot == 2) ? " selected" : "");
  html.replace("__DIM_MIN__", String(WifiConfig::loadDimMinutes()));
  html.replace("__OFF_MIN__", String(WifiConfig::loadOffMinutes()));

  _server->send(200, "text/html", html);
}

void WifiClockface::handleScan()
{
  if (_server->hasArg("rescan") && _server->arg("rescan") == "1") {
    int state = WiFi.scanComplete();
    if (state != WIFI_SCAN_RUNNING) {
      WiFi.scanDelete();
      WiFi.scanNetworks(true);
    }
  }

  String json = buildScanJson();
  _server->send(200, "application/json", json);
}

void WifiClockface::handleSave()
{
  String ssid = _server->hasArg("ssid") ? _server->arg("ssid") : "";
  String password = _server->hasArg("password") ? _server->arg("password") : "";
  ssid.trim();

  if (ssid.length() == 0) {
    _server->send(400, "text/html",
      "<html><body style='font-family:sans-serif;background:#0f172a;color:#f87171;padding:20px'>"
      "<h2>Missing WiFi Name</h2><p>Please enter or select a WiFi SSID.</p>"
      "<a href='/' style='color:#38bdf8'>Back</a></body></html>");
    return;
  }

  bool saved = WifiConfig::save(ssid, password);
  if (!saved) {
    _server->send(500, "text/html",
      "<html><body style='font-family:sans-serif;background:#0f172a;color:#f87171;padding:20px'>"
      "<h2>Save Failed</h2><p>Could not write WiFi credentials to flash.</p>"
      "<a href='/' style='color:#38bdf8'>Back</a></body></html>");
    return;
  }

  _savedSsid = ssid;
  _statusLine = "TESTING...";
  _statusColor = COLOR_AMBER;
  drawScreen();

  WiFi.disconnect(false);
  WiFi.begin(ssid.c_str(), password.c_str());

  unsigned long start = millis();
  while (WiFi.status() != WL_CONNECTED && (millis() - start) < STA_TEST_TIMEOUT_MS) {
    delay(100);
  }

  bool connected = (WiFi.status() == WL_CONNECTED);
  bool ntpSynced = false;
  String staIp = "";
  if (connected) {
    staIp = WiFi.localIP().toString();
    if (_dateTime) {
      ntpSynced = _dateTime->syncTimeNtp();
    }
    _statusLine = "SAVED OK!";
    _statusColor = COLOR_GREEN;
  } else {
    _statusLine = "SAVED";
    _statusColor = COLOR_AMBER;
  }
  WiFi.disconnect(false);
  _dirtyDisplay = true;

  String page = "<!DOCTYPE html><html><head><meta charset='utf-8'>"
                "<meta name='viewport' content='width=device-width,initial-scale=1'>"
                "<title>WiFi Saved</title>"
                "<style>body{font-family:sans-serif;background:#0f172a;color:#e2e8f0;padding:20px;display:flex;justify-content:center}"
                ".card{max-width:420px;width:100%;background:#1e293b;border:1px solid #334155;border-radius:14px;padding:20px}"
                "a{display:inline-block;margin-top:16px;padding:10px 14px;background:#0284c7;color:#fff;text-decoration:none;border-radius:8px}</style>"
                "</head><body><div class='card'>";

  if (connected) {
    page += "<h2 style='color:#4ade80;margin-top:0'>Saved &amp; Connected!</h2>";
    page += "<p>Network: <strong>" + escapeHtml(ssid) + "</strong></p>";
    page += "<p>IP Address: <code>" + escapeHtml(staIp) + "</code></p>";
    page += "<p>Clock NTP sync: <strong>" + String(ntpSynced ? "Updated" : "Pending") + "</strong></p>";
  } else {
    page += "<h2 style='color:#fbbf24;margin-top:0'>Credentials Saved</h2>";
    page += "<p>Saved network: <strong>" + escapeHtml(ssid) + "</strong></p>";
    page += "<p>Could not confirm connection within 8 seconds. Double-check the password or switch away from Settings when done.</p>";
  }

  page += "<p style='color:#94a3b8;font-size:.9rem'>Turn the dial to any clock face when finished to turn off the WiFi antenna.</p>";
  page += "<a href='/'>Back to Settings</a></div></body></html>";

  _server->send(200, "text/html", page);
}

namespace {
bool parseNonNegativeIntArg(WebServer &server, const char *name, int maxVal, int &outVal)
{
  if (!server.hasArg(name)) {
    return false;
  }
  String raw = server.arg(name);
  raw.trim();
  if (raw.length() == 0) {
    return false;
  }
  for (size_t i = 0; i < raw.length(); ++i) {
    if (!isDigit(raw[i])) {
      return false;
    }
  }
  long val = raw.toInt();
  if (val < 0 || val > maxVal) {
    return false;
  }
  outVal = static_cast<int>(val);
  return true;
}
} // namespace

void WifiClockface::handleDisplay()
{
  int rot = -1;
  int dimMin = -1;
  int offMin = -1;

  bool rotOk = parseNonNegativeIntArg(*_server, "rotation", 3, rot);
  bool dimOk = parseNonNegativeIntArg(*_server, "dimMinutes", WifiConfig::MAX_TIMEOUT_MINUTES, dimMin);
  bool offOk = parseNonNegativeIntArg(*_server, "offMinutes", WifiConfig::MAX_TIMEOUT_MINUTES, offMin);

  if (!rotOk || !dimOk || !offOk ||
      !WifiConfig::saveTftRotation(static_cast<uint8_t>(rot)) ||
      !WifiConfig::savePowerTimeouts(static_cast<uint16_t>(dimMin), static_cast<uint16_t>(offMin))) {
    _server->send(400, "text/html",
      "<html><body style='font-family:sans-serif;background:#0f172a;color:#f87171;padding:20px'>"
      "<h2>Invalid Settings</h2><p>Please enter valid values (1 to 720 minutes).</p>"
      "<a href='/' style='color:#38bdf8'>Back</a></body></html>");
    return;
  }

  uint16_t normalizedDim = WifiConfig::normalizeTimeoutMinutes(static_cast<uint16_t>(dimMin));
  uint16_t normalizedOff = WifiConfig::normalizeTimeoutMinutes(static_cast<uint16_t>(offMin));

  if (_onRotationChanged) {
    _onRotationChanged(static_cast<uint8_t>(rot));
  }
  if (_onPowerTimeoutsChanged) {
    _onPowerTimeoutsChanged(normalizedDim, normalizedOff);
  }

  _statusLine = "SAVED";
  _statusColor = COLOR_GREEN;
  _dirtyDisplay = true;

  const char *degLabel = "0&deg; (Default)";
  if (rot == 0) degLabel = "90&deg; Clockwise";
  else if (rot == 1) degLabel = "180&deg; (Upside Down)";
  else if (rot == 2) degLabel = "270&deg; Clockwise";

  String dimLabel = String(normalizedDim) + " min";
  String offLabel = String(normalizedOff) + " min";

  String page = "<!DOCTYPE html><html><head><meta charset='utf-8'>"
                "<meta name='viewport' content='width=device-width,initial-scale=1'>"
                "<title>Settings Saved</title>"
                "<style>body{font-family:sans-serif;background:#0f172a;color:#e2e8f0;padding:20px;display:flex;justify-content:center}"
                ".card{max-width:420px;width:100%;background:#1e293b;border:1px solid #334155;border-radius:14px;padding:20px}"
                "a{display:inline-block;margin-top:16px;padding:10px 14px;background:#0284c7;color:#fff;text-decoration:none;border-radius:8px}</style>"
                "</head><body><div class='card'>"
                "<h2 style='color:#4ade80;margin-top:0'>Display &amp; Power Saved</h2>"
                "<p>Dial orientation: <strong>" + String(degLabel) + "</strong></p>"
                "<p>Dim clock after: <strong>" + dimLabel + "</strong></p>"
                "<p>Turn display off after: <strong>" + offLabel + "</strong></p>"
                "<p style='color:#94a3b8;font-size:.9rem'>Changes applied immediately and saved permanently across reboots.</p>"
                "<a href='/'>Back to Settings</a></div></body></html>";

  _server->send(200, "text/html", page);
}

void WifiClockface::handleNotFound()
{
  _server->sendHeader("Location", String("http://") + AP_IP.toString() + "/", true);
  _server->send(302, "text/plain", "");
}

void WifiClockface::drawScreen()
{
  if (!_display) {
    return;
  }

  _display->fillScreen(COLOR_BG);
  drawAntennaIcon(_animFrame);
  drawCenteredText(26, AP_SSID, COLOR_CYAN);
  drawCompactIp(36, "192.168.4.1", COLOR_WHITE);
  drawCenteredText(46, _statusLine, _statusColor);

  String footer = (_savedSsid.length() > 0) ? _savedSsid : String("NO WIFI");
  if (footer.length() > 10) {
    footer = footer.substring(0, 10);
  }
  drawCenteredText(55, footer, COLOR_GRAY);
}

void WifiClockface::drawAntennaIcon(uint8_t waveStep)
{
  if (!_display) {
    return;
  }

  _display->fillRect(16, 2, 32, 21, COLOR_BG);

  // Antenna mast + base
  _display->drawFastVLine(31, 9, 11, COLOR_WHITE);
  _display->drawFastVLine(32, 9, 11, COLOR_WHITE);
  _display->drawLine(27, 21, 31, 17, COLOR_GRAY);
  _display->drawLine(36, 21, 32, 17, COLOR_GRAY);
  _display->drawFastHLine(27, 21, 10, COLOR_GRAY);

  // Glowing feedpoint tip
  uint16_t tipColor = (waveStep == 0) ? COLOR_CYAN : COLOR_GREEN;
  _display->fillRect(31, 7, 2, 2, tipColor);

  // Animated left/right RF waves (3 concentric arcs)
  const uint8_t radii[3] = {4, 7, 10};
  for (uint8_t ring = 0; ring < 3; ++ring) {
    uint16_t color = (waveStep > ring) ? COLOR_CYAN : COLOR_DIM_BLUE;
    uint8_t r = radii[ring];
    for (int dy = -static_cast<int>(r); dy <= static_cast<int>(r); ++dy) {
      int dxSq = static_cast<int>(r) * static_cast<int>(r) - dy * dy;
      if (dxSq <= 0) {
        continue;
      }
      int dx = 0;
      while ((dx + 1) * (dx + 1) <= dxSq) {
        ++dx;
      }
      if (abs(dy) <= dx + 1) {
        _display->drawPixel(31 - dx, 8 + dy, color);
        _display->drawPixel(32 + dx, 8 + dy, color);
      }
    }
  }
}

void WifiClockface::drawCenteredText(int16_t y, const String &text, uint16_t color)
{
  if (!_display || text.length() == 0) {
    return;
  }

  String shown = text;
  if (shown.length() > 10) {
    shown = shown.substring(0, 10);
  }

  int16_t width = static_cast<int16_t>(shown.length()) * 6 - 1;
  int16_t x = (64 - width) / 2;
  if (x < 0) {
    x = 0;
  }

  _display->setFont();
  _display->setTextSize(1);
  _display->setTextWrap(false);
  _display->setTextColor(color);
  _display->setCursor(x, y);
  _display->print(shown);
}

void WifiClockface::drawCompactIp(int16_t y, const char *ipText, uint16_t color)
{
  if (!_display || !ipText) {
    return;
  }

  int16_t totalWidth = 0;
  for (const char *p = ipText; *p; ++p) {
    totalWidth += (*p == '.') ? 3 : 6;
  }
  if (totalWidth > 0) {
    totalWidth -= 1;
  }

  int16_t x = (64 - totalWidth) / 2;
  if (x < 0) {
    x = 0;
  }

  _display->setFont();
  _display->setTextSize(1);
  _display->setTextWrap(false);
  _display->setTextColor(color);

  for (const char *p = ipText; *p; ++p) {
    if (*p == '.') {
      _display->fillRect(x, y + 5, 2, 2, color);
      x += 3;
    } else {
      _display->setCursor(x, y);
      _display->print(*p);
      x += 6;
    }
  }
}

String WifiClockface::escapeHtml(const String &src)
{
  String out;
  out.reserve(src.length() + 8);
  for (size_t i = 0; i < src.length(); ++i) {
    char c = src[i];
    switch (c) {
      case '&':  out += "&amp;"; break;
      case '<':  out += "&lt;"; break;
      case '>':  out += "&gt;"; break;
      case '"':  out += "&quot;"; break;
      case '\'': out += "&#39;"; break;
      default:   out += c; break;
    }
  }
  return out;
}

String WifiClockface::escapeJson(const String &src)
{
  String out;
  out.reserve(src.length() + 8);
  for (size_t i = 0; i < src.length(); ++i) {
    char c = src[i];
    if (c == '\\' || c == '"') {
      out += '\\';
      out += c;
    } else if (static_cast<unsigned char>(c) >= 0x20) {
      out += c;
    }
  }
  return out;
}

String WifiClockface::buildScanJson()
{
  int n = WiFi.scanComplete();
  if (n == WIFI_SCAN_RUNNING) {
    return "{\"scanning\":true,\"networks\":[]}";
  }
  if (n < 0) {
    WiFi.scanNetworks(true);
    return "{\"scanning\":true,\"networks\":[]}";
  }

  std::vector<ScannedNetwork> uniqueNets;
  uniqueNets.reserve(n);

  for (int i = 0; i < n; ++i) {
    String ssid = WiFi.SSID(i);
    if (ssid.length() == 0) {
      continue;
    }
    int32_t rssi = WiFi.RSSI(i);
    bool secure = (WiFi.encryptionType(i) != WIFI_AUTH_OPEN);

    auto existing = std::find_if(uniqueNets.begin(), uniqueNets.end(),
      [&ssid](const ScannedNetwork &item) { return item.ssid == ssid; });

    if (existing != uniqueNets.end()) {
      if (rssi > existing->rssi) {
        existing->rssi = rssi;
        existing->secure = secure;
      }
    } else {
      uniqueNets.push_back({ssid, rssi, secure});
    }
  }

  std::sort(uniqueNets.begin(), uniqueNets.end(),
    [](const ScannedNetwork &a, const ScannedNetwork &b) { return a.rssi > b.rssi; });

  String json = "{\"scanning\":false,\"networks\":[";
  for (size_t i = 0; i < uniqueNets.size(); ++i) {
    if (i > 0) {
      json += ',';
    }
    json += "{\"ssid\":\"";
    json += escapeJson(uniqueNets[i].ssid);
    json += "\",\"rssi\":";
    json += String(uniqueNets[i].rssi);
    json += ",\"secure\":";
    json += uniqueNets[i].secure ? "true" : "false";
    json += "}";
  }
  json += "]}";
  return json;
}
