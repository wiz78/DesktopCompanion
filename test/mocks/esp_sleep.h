#ifndef TEST_MOCKS_ESP_SLEEP_H
#define TEST_MOCKS_ESP_SLEEP_H

#include <cstdint>

enum gpio_num_t : int
{
    GPIO_NUM_NC = -1,
    GPIO_NUM_0 = 0,
    GPIO_NUM_1 = 1,
    GPIO_NUM_2 = 2,
    GPIO_NUM_3 = 3,
    GPIO_NUM_4 = 4,
    GPIO_NUM_5 = 5,
    GPIO_NUM_6 = 6,
    GPIO_NUM_7 = 7,
    GPIO_NUM_8 = 8,
    GPIO_NUM_9 = 9,
    GPIO_NUM_10 = 10,
    GPIO_NUM_11 = 11,
    GPIO_NUM_12 = 12
};

enum esp_sleep_ext1_wakeup_mode_t
{
    ESP_EXT1_WAKEUP_ALL_LOW = 0,
    ESP_EXT1_WAKEUP_ANY_HIGH = 1
};

inline uint64_t g_mockSleepWakeupPinMaskLow = 0;
inline uint64_t g_mockSleepWakeupPinMaskHigh = 0;
inline bool g_mockDeepSleepStarted = false;

inline void resetMockSleepState()
{
    g_mockSleepWakeupPinMaskLow = 0;
    g_mockSleepWakeupPinMaskHigh = 0;
    g_mockDeepSleepStarted = false;
}

inline int esp_sleep_enable_ext0_wakeup( const gpio_num_t gpio_num, const int level )
{
    if( level == 0 ) {
        g_mockSleepWakeupPinMaskLow |= ( 1ULL << static_cast<uint8_t>(gpio_num) );
    } else {
        g_mockSleepWakeupPinMaskHigh |= ( 1ULL << static_cast<uint8_t>(gpio_num) );
    }
    return 0;
}

inline int esp_sleep_enable_ext1_wakeup( const uint64_t mask, const esp_sleep_ext1_wakeup_mode_t mode )
{
    if( mode == ESP_EXT1_WAKEUP_ALL_LOW ) {
        g_mockSleepWakeupPinMaskLow |= mask;
    } else {
        g_mockSleepWakeupPinMaskHigh |= mask;
    }
    return 0;
}

inline void esp_deep_sleep_start()
{
    g_mockDeepSleepStarted = true;
}

inline int gpio_pullup_en( const gpio_num_t gpio_num )
{
    (void)gpio_num;
    return 0;
}

inline int gpio_pulldown_dis( const gpio_num_t gpio_num )
{
    (void)gpio_num;
    return 0;
}

#endif // TEST_MOCKS_ESP_SLEEP_H
