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
const unsigned long TZ_SYNC_TIMEOUT_MS  = 6000;

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
  .stack{width:100%;max-width:440px;display:flex;flex-direction:column;gap:12px}
  .card{width:100%;background:#1e293b;border:1px solid #334155;border-radius:14px;padding:20px;box-shadow:0 10px 25px rgba(0,0,0,.35)}
  h1{margin:0 0 10px;font-size:1.25rem;color:#38bdf8}
  h2{margin:0 0 8px;font-size:1.05rem;color:#38bdf8}
  p.sub{margin:0 0 14px;font-size:.9rem;color:#94a3b8}
  .tabs{display:flex;gap:6px;background:#1e293b;border:1px solid #334155;border-radius:12px;padding:6px}
  .tab{flex:1;padding:9px 6px;border:0;border-radius:8px;background:transparent;color:#94a3b8;font-size:.9rem;font-weight:600;cursor:pointer}
  .tab.active{background:#0284c7;color:#fff}
  .panel{display:none}
  .panel.active{display:block}
  .saved{background:#0f172a;border:1px solid #334155;border-radius:8px;padding:10px 12px;margin-bottom:14px;font-size:.88rem}
  label{display:block;font-size:.85rem;color:#cbd5e1;margin:12px 0 6px}
  select,input,textarea{width:100%;padding:10px 12px;border-radius:8px;border:1px solid #475569;background:#0f172a;color:#f8fafc;font-size:1rem}
  .row{display:flex;gap:8px;align-items:center}
  .btn{width:100%;margin-top:16px;padding:11px;border:0;border-radius:8px;font-size:1rem;font-weight:600;cursor:pointer;background:#0284c7;color:#fff}
  .btn:hover{background:#0369a1}
  .btn-sec{width:auto;margin-top:0;padding:10px 12px;background:#334155;color:#e2e8f0;white-space:nowrap}
  .btn-sec:hover{background:#475569}
  .btn-del{width:auto;margin-top:0;padding:9px 12px;background:#7f1d1d;color:#fecaca;border:0;border-radius:8px;cursor:pointer;font-weight:700}
  .btn-del:hover{background:#991b1b}
  .saying-list{display:flex;flex-direction:column;gap:8px;margin-top:6px}
  .chk{display:flex;align-items:center;gap:8px;margin-top:8px;font-size:.85rem;color:#94a3b8}
  .chk input{width:auto}
  .status{margin-top:10px;font-size:.85rem;color:#38bdf8;min-height:1.2em}
</style>
</head>
<body>
<div class="stack">
<div class="tabs" role="tablist">
  <button type="button" class="tab active" data-tab="wifi" onclick="switchTab('wifi')">WiFi</button>
  <button type="button" class="tab" data-tab="display" onclick="switchTab('display')">Display</button>
  <button type="button" class="tab" data-tab="time" onclick="switchTab('time')">Time</button>
  <button type="button" class="tab" data-tab="dad" onclick="switchTab('dad')">Dad</button>
</div>

<div class="card panel active" id="panel-wifi">
  <h1>MatrixClock WiFi</h1>
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

<div class="card panel" id="panel-display">
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
    <input id="dimMinutes" name="dimMinutes" type="number" min="1" max="720" required value="__DIM_MIN__">

    <label for="offMinutes">Turn Display Off After (1&ndash;720 min)</label>
    <input id="offMinutes" name="offMinutes" type="number" min="1" max="720" required value="__OFF_MIN__">

    <button type="submit" class="btn">Apply &amp; Save Settings</button>
  </form>
</div>

<div class="card panel" id="panel-time">
  <h2>Time Zone &amp; DST</h2>
  <p class="sub">Choose your time zone and whether Daylight Saving Time adjusts automatically.</p>
  <div class="saved">Clock display time: <strong id="clockTime">__CLOCK_TIME__</strong></div>
  <form method="POST" action="/timezone">
    <label for="tzIndex">Time Zone</label>
    <select id="tzIndex" name="tzIndex">
      __TZ_OPTIONS__
    </select>
    <label class="chk"><input type="checkbox" name="dst" value="1"__DST_CHECKED__> Automatic Daylight Saving Time (DST)</label>
    <button type="submit" class="btn">Save Time Zone</button>
  </form>
</div>

<div class="card panel" id="panel-dad">
  <h2>Dad Clock Face</h2>
  <p class="sub">Customize the menu name, header/footer text, and scrolling sayings.</p>
  <form method="POST" action="/dad" onsubmit="return prepareDadForm()">
    <label for="dadMenu">Menu Name</label>
    <input id="dadMenu" name="menuName" type="text" required maxlength="24" value="__DAD_MENU__" placeholder="Dad">

    <label for="dadTop">Top Text</label>
    <input id="dadTop" name="topText" type="text" maxlength="20" value="__DAD_TOP__" placeholder="To Logan:">

    <label for="dadBot">Bottom Text</label>
    <input id="dadBot" name="bottomText" type="text" maxlength="16" value="__DAD_BOT__" placeholder="- Dad">

    <label>Scrolling Sayings</label>
    <div id="sayingList" class="saying-list"></div>

    <label for="newSaying">Add Saying</label>
    <div class="row">
      <input id="newSaying" type="text" maxlength="120" placeholder="Enter a new saying...">
      <button type="button" class="btn btn-sec" onclick="addSaying()">Add</button>
    </div>

    <textarea id="sayingsHidden" name="sayings" style="display:none"></textarea>
    <div class="row" style="margin-top:16px">
      <button type="button" class="btn btn-sec" onclick="resetDadDefaults()">Reset Defaults</button>
      <button type="submit" class="btn" style="margin-top:0">Save Dad Settings</button>
    </div>
  </form>
</div>
</div>
<script>
var dadSayings=__DAD_SAYINGS_JSON__;
var dadDefaultSayings=__DAD_DEFAULT_SAYINGS_JSON__;
function switchTab(name){
  var tabs=document.querySelectorAll('.tab');
  var panels=document.querySelectorAll('.panel');
  tabs.forEach(function(t){t.classList.toggle('active',t.getAttribute('data-tab')===name);});
  panels.forEach(function(p){p.classList.toggle('active',p.id==='panel-'+name);});
}
function renderSayings(){
  var list=document.getElementById('sayingList');
  list.innerHTML='';
  dadSayings.forEach(function(text,idx){
    var row=document.createElement('div');
    row.className='row';
    var inp=document.createElement('input');
    inp.type='text';
    inp.maxLength=120;
    inp.value=text;
    inp.oninput=function(){dadSayings[idx]=this.value;};
    var del=document.createElement('button');
    del.type='button';
    del.className='btn-del';
    del.textContent='X';
    del.title='Remove saying';
    del.onclick=function(){dadSayings.splice(idx,1);renderSayings();};
    row.appendChild(inp);
    row.appendChild(del);
    list.appendChild(row);
  });
}
function addSaying(){
  var inp=document.getElementById('newSaying');
  var val=inp.value.trim();
  if(!val){return;}
  dadSayings.push(val);
  inp.value='';
  renderSayings();
}
function resetDadDefaults(){
  document.getElementById('dadMenu').value='Dad';
  document.getElementById('dadTop').value='To Logan:';
  document.getElementById('dadBot').value='- Dad';
  dadSayings=dadDefaultSayings.slice();
  renderSayings();
}
function prepareDadForm(){
  var cleaned=dadSayings.map(function(s){return s.replace(/[\r\n]+/g,' ').trim();}).filter(function(s){return s.length>0;});
  document.getElementById('sayingsHidden').value=cleaned.join('\n');
  return true;
}
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
function pollClockTime(){
  fetch('/time')
    .then(function(r){return r.json();})
    .then(function(data){
      if(data&&data.time){
        document.getElementById('clockTime').textContent=data.time;
      }
      setTimeout(pollClockTime,1000);
    })
    .catch(function(){
      setTimeout(pollClockTime,2000);
    });
}
renderSayings();
var params=new URLSearchParams(window.location.search);
var startTab=params.get('tab');
if(startTab&&document.getElementById('panel-'+startTab)){switchTab(startTab);}
loadScan(false);
setTimeout(pollClockTime,1000);
</script>
</body>
</html>)rawliteral";
}

WifiClockface::WifiClockface(Adafruit_GFX *display,
                             RotationCallback onRotationChanged,
                             PowerTimeoutCallback onPowerTimeoutsChanged,
                             DadSettingsCallback onDadSettingsChanged)
  : _display(display),
    _onRotationChanged(onRotationChanged),
    _onPowerTimeoutsChanged(onPowerTimeoutsChanged),
    _onDadSettingsChanged(onDadSettingsChanged)
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
  _server->on("/timezone", HTTP_POST, [this]() { handleTimeZone(); });
  _server->on("/dad", HTTP_POST, [this]() { handleDad(); });
  _server->on("/time", HTTP_GET, [this]() { handleTime(); });
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

  uint8_t tzIdx = WifiConfig::loadTimeZoneIndex();
  uint8_t tzCount = WifiConfig::getTimeZoneCount();
  String tzOptions;
  for (uint8_t i = 0; i < tzCount; ++i) {
    tzOptions += "<option value='";
    tzOptions += String(i);
    tzOptions += "'";
    if (i == tzIdx) {
      tzOptions += " selected";
    }
    tzOptions += ">";
    tzOptions += escapeHtml(WifiConfig::getTimeZoneLabel(i));
    tzOptions += "</option>";
  }
  html.replace("__TZ_OPTIONS__", tzOptions);
  html.replace("__DST_CHECKED__", WifiConfig::loadDstEnabled() ? " checked" : "");
  html.replace("__CLOCK_TIME__", escapeHtml(formatClockTime()));

  html.replace("__DAD_MENU__", escapeHtml(WifiConfig::loadDadMenuName()));
  html.replace("__DAD_TOP__", escapeHtml(WifiConfig::loadDadTopText()));
  html.replace("__DAD_BOT__", escapeHtml(WifiConfig::loadDadBottomText()));

  auto buildSayingsJson = [](const std::vector<String> &items) {
    String json = "[";
    for (size_t i = 0; i < items.size(); ++i) {
      if (i > 0) json += ",";
      json += "\"";
      json += escapeJson(items[i]);
      json += "\"";
    }
    json += "]";
    return json;
  };

  std::vector<String> dadSayings;
  WifiConfig::loadDadSayings(dadSayings);
  std::vector<String> dadDefaults;
  WifiConfig::loadDefaultDadSayings(dadDefaults);
  html.replace("__DAD_SAYINGS_JSON__", buildSayingsJson(dadSayings));
  html.replace("__DAD_DEFAULT_SAYINGS_JSON__", buildSayingsJson(dadDefaults));

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

  bool connected = false;
  String staIp;
  bool ntpSynced = syncNtpWithCredentials(ssid, password, STA_TEST_TIMEOUT_MS, connected, &staIp);
  if (connected) {
    _statusLine = "SAVED OK!";
    _statusColor = COLOR_GREEN;
  } else {
    _statusLine = "SAVED";
    _statusColor = COLOR_AMBER;
  }
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
    page += "<p>Clock display time: <strong>" + escapeHtml(formatClockTime()) + "</strong></p>";
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

bool WifiClockface::syncNtpWithCredentials(const String &ssid, const String &password,
                                           unsigned long timeoutMs, bool &connectedOut,
                                           String *staIpOut)
{
  WiFi.disconnect(false);
  WiFi.begin(ssid.c_str(), password.c_str());

  unsigned long start = millis();
  while (WiFi.status() != WL_CONNECTED && (millis() - start) < timeoutMs) {
    delay(100);
  }

  connectedOut = (WiFi.status() == WL_CONNECTED);
  bool ntpSynced = false;
  if (connectedOut) {
    if (staIpOut) {
      *staIpOut = WiFi.localIP().toString();
    }
    if (_dateTime) {
      ntpSynced = _dateTime->syncTimeNtp();
    }
  }
  WiFi.disconnect(false);
  return ntpSynced;
}

void WifiClockface::handleDad()
{
  String menuName = _server->hasArg("menuName") ? _server->arg("menuName") : "";
  String topText  = _server->hasArg("topText")  ? _server->arg("topText")  : "";
  String botText  = _server->hasArg("bottomText") ? _server->arg("bottomText") : "";
  String rawSayings = _server->hasArg("sayings") ? _server->arg("sayings") : "";

  menuName.trim();
  if (menuName.length() == 0) {
    menuName = WifiConfig::DEFAULT_DAD_MENU_NAME;
  }

  std::vector<String> sayings;
  int start = 0;
  while (start <= (int)rawSayings.length()) {
    int nl = rawSayings.indexOf('\n', start);
    String line = (nl < 0) ? rawSayings.substring(start) : rawSayings.substring(start, nl);
    line.replace("\r", "");
    line.trim();
    if (line.length() > 0) {
      sayings.push_back(line);
    }
    if (nl < 0) break;
    start = nl + 1;
  }

  if (sayings.empty()) {
    WifiConfig::loadDefaultDadSayings(sayings);
  }

  WifiConfig::saveDadMenuName(menuName);
  WifiConfig::saveDadTopText(topText);
  WifiConfig::saveDadBottomText(botText);
  WifiConfig::saveDadSayings(sayings);

  if (_onDadSettingsChanged) {
    _onDadSettingsChanged();
  }

  _statusLine = "SAVED";
  _statusColor = COLOR_GREEN;
  _dirtyDisplay = true;

  String page = "<!DOCTYPE html><html><head><meta charset='utf-8'>"
                "<meta name='viewport' content='width=device-width,initial-scale=1'>"
                "<title>Dad Settings Saved</title>"
                "<style>body{font-family:sans-serif;background:#0f172a;color:#e2e8f0;padding:20px;display:flex;justify-content:center}"
                ".card{max-width:420px;width:100%;background:#1e293b;border:1px solid #334155;border-radius:14px;padding:20px}"
                "a{display:inline-block;margin-top:16px;padding:10px 14px;background:#0284c7;color:#fff;text-decoration:none;border-radius:8px}</style>"
                "</head><body><div class='card'>"
                "<h2 style='color:#4ade80;margin-top:0'>Dad Settings Saved!</h2>"
                "<p>Menu Name: <strong>" + escapeHtml(WifiConfig::loadDadMenuName()) + "</strong></p>"
                "<p>Top Text: <strong>" + escapeHtml(topText) + "</strong></p>"
                "<p>Bottom Text: <strong>" + escapeHtml(botText) + "</strong></p>"
                "<p>Sayings saved: <strong>" + String(sayings.size()) + "</strong></p>"
                "<a href='/?tab=dad'>Back to Dad Settings</a></div></body></html>";

  _server->send(200, "text/html", page);
}

void WifiClockface::handleTimeZone()
{
  int tzIndex = -1;
  uint8_t maxIndex = (WifiConfig::getTimeZoneCount() > 0)
                       ? static_cast<uint8_t>(WifiConfig::getTimeZoneCount() - 1)
                       : 0;
  bool tzOk = parseNonNegativeIntArg(*_server, "tzIndex", maxIndex, tzIndex);
  bool dstEnabled = _server->hasArg("dst");

  if (!tzOk || !WifiConfig::saveTimeZone(static_cast<uint8_t>(tzIndex), dstEnabled)) {
    _server->send(400, "text/html",
      "<html><body style='font-family:sans-serif;background:#0f172a;color:#f87171;padding:20px'>"
      "<h2>Invalid Time Zone</h2><p>Please select a valid time zone.</p>"
      "<a href='/' style='color:#38bdf8'>Back</a></body></html>");
    return;
  }

  String ssid;
  String pass;
  bool hasWifi = WifiConfig::load(ssid, pass);
  bool connected = false;
  bool ntpSynced = false;
  bool updatedFromSystemClock = false;

  if (hasWifi) {
    _statusLine = "SYNCING...";
    _statusColor = COLOR_AMBER;
    drawScreen();
    ntpSynced = syncNtpWithCredentials(ssid, pass, TZ_SYNC_TIMEOUT_MS, connected);
  }
  if (!ntpSynced && _dateTime) {
    updatedFromSystemClock = _dateTime->applyTimeZone();
  }

  _statusLine = (ntpSynced || updatedFromSystemClock) ? "SYNCED!" : "SAVED";
  _statusColor = COLOR_GREEN;
  _dirtyDisplay = true;

  String tzLabel = escapeHtml(WifiConfig::getTimeZoneLabel(static_cast<uint8_t>(tzIndex)));
  String dstLabel = dstEnabled ? "Automatic (POSIX rule)" : "Off (Standard time year-round)";
  String clockTimeStr = escapeHtml(formatClockTime());
  String syncMsg;
  if (ntpSynced) {
    syncMsg = "Clock updated via NTP now.";
  } else if (updatedFromSystemClock) {
    syncMsg = "Clock adjusted to the new time zone immediately.";
  } else if (hasWifi) {
    syncMsg = "Saved, but WiFi/NTP could not be reached right now; will apply on next boot/NTP sync.";
  } else {
    syncMsg = "Saved. Connect WiFi above or restart once WiFi is configured to sync the clock.";
  }

  String page = "<!DOCTYPE html><html><head><meta charset='utf-8'>"
                "<meta name='viewport' content='width=device-width,initial-scale=1'>"
                "<title>Time Zone Saved</title>"
                "<style>body{font-family:sans-serif;background:#0f172a;color:#e2e8f0;padding:20px;display:flex;justify-content:center}"
                ".card{max-width:420px;width:100%;background:#1e293b;border:1px solid #334155;border-radius:14px;padding:20px}"
                "a{display:inline-block;margin-top:16px;padding:10px 14px;background:#0284c7;color:#fff;text-decoration:none;border-radius:8px}</style>"
                "</head><body><div class='card'>"
                "<h2 style='color:#4ade80;margin-top:0'>Time Zone Saved</h2>"
                "<p>Clock display time: <strong>" + clockTimeStr + "</strong></p>"
                "<p>Time zone: <strong>" + tzLabel + "</strong></p>"
                "<p>Daylight Saving Time: <strong>" + dstLabel + "</strong></p>"
                "<p>" + syncMsg + "</p>"
                "<a href='/'>Back to Settings</a></div></body></html>";

  _server->send(200, "text/html", page);
}

String WifiClockface::formatClockTime() const
{
  if (!_dateTime) {
    return "Unavailable";
  }
  char buf[40];
  _dateTime->formatDateTime(buf, sizeof(buf));
  return String(buf);
}

void WifiClockface::handleTime()
{
  String json = "{\"time\":\"" + escapeJson(formatClockTime()) + "\"}";
  _server->send(200, "application/json", json);
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
