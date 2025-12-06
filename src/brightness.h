#pragma once

#include <cstdint>
#include <string>
#include <vector>
#include <functional>

namespace brightness {

// LG Ultrafine brightness constants
constexpr uint16_t LG_VENDOR_ID = 0x043e;
constexpr uint16_t MIN_BRIGHTNESS = 0x0190;
constexpr uint16_t MAX_BRIGHTNESS = 0xd2f0;

// Brightness step tables for smooth adjustment
extern const std::vector<uint16_t> SMALL_STEPS;
extern const std::vector<uint16_t> BIG_STEPS;

class BrightnessController {
public:
    BrightnessController();
    ~BrightnessController();

    // Prevent copying
    BrightnessController(const BrightnessController&) = delete;
    BrightnessController& operator=(const BrightnessController&) = delete;

    // Initialize and find the monitor
    bool initialize();
    void shutdown();

    // Check if monitor is connected
    bool isConnected() const { return m_connected; }

    // Get current brightness (0-100%)
    int getBrightness();

    // Set brightness (0-100%)
    void setBrightness(int percent);

    // Get raw brightness value
    uint16_t getRawBrightness();

    // Set raw brightness value
    void setRawBrightness(uint16_t value);

    // Step brightness up/down
    void stepUp(bool bigStep = false);
    void stepDown(bool bigStep = false);

    // Get monitor info
    std::wstring getMonitorName() const { return m_monitorName; }

    // Callback for brightness changes
    using BrightnessCallback = std::function<void(int)>;
    void setCallback(BrightnessCallback callback) { m_callback = callback; }

private:
    void* m_handle = nullptr;
    bool m_connected = false;
    std::wstring m_monitorName;
    BrightnessCallback m_callback;

    uint16_t findNextStep(uint16_t val, const std::vector<uint16_t>& steps);
    uint16_t findPrevStep(uint16_t val, const std::vector<uint16_t>& steps);
    int rawToPercent(uint16_t raw);
    uint16_t percentToRaw(int percent);
};

} // namespace brightness
