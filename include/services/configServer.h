//
// ©2026 by Simone Tellini - https://tellini.info
//
// Licensed under the Creative Commons Attribution-NonCommercial-ShareAlike 4.0
// International License (CC BY-NC-SA 4.0).
//

#ifndef FRIENDLYBOT_SERVICES_CONFIGSERVER_H
#define FRIENDLYBOT_SERVICES_CONFIGSERVER_H

#include "i18n.h"
#include "sentenceEngine.h"
#include "services/webUtils.h"

#include <atomic>
#include <memory>
#include <mutex>
#include <string>

#ifdef ARDUINO
#include <DNSServer.h>
#include <Update.h>
#include <WebServer.h>
#include <WiFi.h>
#endif

class NetworkWorker;

class ConfigServer
{
public:
    static ConfigServer& instance();

    void begin( SentenceEngine *sentences, NetworkWorker *network );
    void handleClient();

    void startAp( const char *ssid = "Console50-Setup", const char *pass = nullptr );
    void stopAp();

    [[nodiscard]] bool isApActive() const;
    [[nodiscard]] std::string getApSsid() const;
    [[nodiscard]] std::string getApIp() const;

private:
    ConfigServer() = default;

    SentenceEngine *sentenceEngine = nullptr;
    NetworkWorker *networkWorker = nullptr;

    std::atomic<bool> apActive{ false };
    mutable std::mutex apMutex;
    std::string apSsid = "Console50-Setup";
    std::string apIp = "192.168.4.1";

#ifdef ARDUINO
    WebServer server{ 80 };
    DNSServer dnsServer;

    void registerRoutes();
    void handleRoot();
    void handleCaptiveRedirect();
    void handleApiStatus();
    void handleApiWifiScan();
    void handleApiWifiSave();
    void handleApiWifiClear();
    void handleApiConfigSave();
    void handleApiSentencesGet();
    void handleApiSentencesSave();
    void handleApiSentencesDelete();
    void handleApiOtaUpload();
    void handleApiRestart();
    void handleApiOrientationDefault();
#endif
};

#endif // FRIENDLYBOT_SERVICES_CONFIGSERVER_H
