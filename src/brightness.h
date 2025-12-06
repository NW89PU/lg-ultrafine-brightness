#pragma once

#include <cstdint>
#include <string>
#include <vector>
#include <functional>
#include <memory>

// Forward declaration
namespace als {
    class ALSSensor;
}

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

    // Ambient Light Sensor (ALS) support
    bool initializeALS();
    bool hasALS() const;
    float getAmbientLight();  // Returns lux value
    std::wstring getALSName() const;

    // Auto-brightness control
    void setAutoBrightness(bool enabled);
    bool isAutoBrightnessEnabled() const { return m_autoBrightnessEnabled; }
    void updateAutoBrightness();  // Call periodically to adjust brightness based on ambient light

private:
    void* m_handle = nullptr;
    bool m_connected = false;
    std::wstring m_monitorName;
    BrightnessCallback m_callback;

    // ALS members
    std::unique_ptr<als::ALSSensor> m_alsSensor;
    bool m_autoBrightnessEnabled = false;
    float m_lastAmbientLight = 0.0f;

    // Cached brightness value to avoid redundant HID reads
    uint16_t m_cachedBrightness = MIN_BRIGHTNESS;
    bool m_hasCachedValue = false;

    uint16_t findNextStep(uint16_t val, const std::vector<uint16_t>& steps);
    uint16_t findPrevStep(uint16_t val, const std::vector<uint16_t>& steps);
    int rawToPercent(uint16_t raw);
    uint16_t percentToRaw(int percent);
};

} // namespace brightness
