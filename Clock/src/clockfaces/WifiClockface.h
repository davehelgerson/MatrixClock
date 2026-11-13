#ifndef WifiClockface_h
#define WifiClockface_h

#include <Adafruit_GFX.h>
#include <WebServer.h>
#include <DNSServer.h>
#include <memory>
#include "../core/IClockface.h"
#include "../core/CWDateTime.h"

/**
 * Clockface/mode for the Settings menu option.
 * Turns on a SoftAP + captive-portal web server when active so the user can
 * configure home WiFi credentials and menu dial display rotation in NVS, and
 * turns the WiFi radio completely off on teardown().
 */
class WifiClockface : public IClockface {
  public:
    using RotationCallback = void (*)(uint8_t);
    using PowerTimeoutCallback = void (*)(uint16_t, uint16_t);

  private:
    static constexpr const char *AP_SSID = "MatrixWiFi";

    Adafruit_GFX *_display;
    CWDateTime *_dateTime = nullptr;
    RotationCallback _onRotationChanged = nullptr;
    PowerTimeoutCallback _onPowerTimeoutsChanged = nullptr;
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
    void handleDisplay();
    void handleTimeZone();
    void handleTime();
    void handleNotFound();

    void drawScreen();
    void drawAntennaIcon(uint8_t waveStep);
    void drawCenteredText(int16_t y, const String &text, uint16_t color);
    void drawCompactIp(int16_t y, const char *ipText, uint16_t color);

    static String escapeHtml(const String &src);
    static String escapeJson(const String &src);
    static String buildScanJson();
    String formatClockTime() const;
    bool syncNtpWithCredentials(const String &ssid, const String &password,
                                unsigned long timeoutMs, bool &connectedOut,
                                String *staIpOut = nullptr);

  public:
    WifiClockface(Adafruit_GFX *display,
                  RotationCallback onRotationChanged = nullptr,
                  PowerTimeoutCallback onPowerTimeoutsChanged = nullptr);
    ~WifiClockface() override;

    void setup(CWDateTime *dateTime) override;
    void update() override;
    void teardown() override;
};

#endif

