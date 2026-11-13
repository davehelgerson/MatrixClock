#ifndef WifiClockface_h
#define WifiClockface_h

#include <Adafruit_GFX.h>
#include <WebServer.h>
#include <DNSServer.h>
#include <memory>
#include "../core/IClockface.h"
#include "../core/CWDateTime.h"

/**
 * Clockface/mode for the WIFI menu option.
 * Turns on a SoftAP + captive-portal web server when active so the user can
 * scan nearby WiFi networks, enter a password, and save home WiFi credentials
 * to NVS, and turns the WiFi radio completely off on teardown().
 */
class WifiClockface : public IClockface {
  private:
    static constexpr const char *AP_SSID = "MatrixWiFi";

    Adafruit_GFX *_display;
    CWDateTime *_dateTime = nullptr;
    std::unique_ptr<WebServer> _server;
    std::unique_ptr<DNSServer> _dnsServer;

    bool _active = false;
    uint8_t _animFrame = 0;
    unsigned long _lastAnimMillis = 0;

    String _statusLine;
    uint16_t _statusColor = 0x07FF; // cyan
    String _savedSsid;
    bool _dirtyDisplay = true;

    void handleRoot();
    void handleScan();
    void handleSave();
    void handleNotFound();

    void drawScreen();
    void drawAntennaIcon(uint8_t waveStep);
    void drawCenteredText(int16_t y, const String &text, uint16_t color);
    void drawCompactIp(int16_t y, const char *ipText, uint16_t color);

    static String escapeHtml(const String &src);
    static String escapeJson(const String &src);
    static String buildScanJson();

  public:
    WifiClockface(Adafruit_GFX *display);
    ~WifiClockface() override;

    void setup(CWDateTime *dateTime) override;
    void update() override;
    void teardown() override;
};

#endif

