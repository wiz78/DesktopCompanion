#ifndef TEST_MOCKS_WIFI_H
#define TEST_MOCKS_WIFI_H

#include <cstdint>

static constexpr uint8_t WIFI_OFF = 0;
static constexpr uint8_t WIFI_STA = 1;
static constexpr uint8_t WIFI_AP = 2;
static constexpr uint8_t WIFI_AP_STA = 3;

static constexpr uint8_t WL_CONNECTED = 3;
static constexpr uint8_t WL_DISCONNECTED = 6;

struct MockIPAddress
{
    [[nodiscard]] std::string toString() const
    {
        return "192.168.4.1";
    }
};

class MockWiFiClass
{
public:
    uint8_t currentMode = WIFI_STA;

    void mode( const uint8_t m )
    {
        currentMode = m;
    }

    [[nodiscard]] uint8_t getMode() const
    {
        return currentMode;
    }

    [[nodiscard]] uint8_t status() const
    {
        return WL_CONNECTED;
    }

    [[nodiscard]] MockIPAddress localIP() const
    {
        return MockIPAddress{};
    }

    [[nodiscard]] uint8_t softAPgetStationNum() const
    {
        return 0;
    }
};

inline MockWiFiClass WiFi;

#endif // TEST_MOCKS_WIFI_H
