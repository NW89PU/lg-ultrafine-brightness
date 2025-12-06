#include "brightness.h"

#include "app.h"
#include "als_sensor.h"

#include <algorithm>
#include <cstring>
#include <hidapi.h>
#include <iostream>
#include <iomanip>

#undef min
#undef max

namespace brightness {

// Step tables from reference implementation
const std::vector<uint16_t> SMALL_STEPS = {
    0x0190, 0x01af, 0x01d2, 0x01f7,
    0x021f, 0x024a, 0x0279, 0x02ac,
    0x02e2, 0x031d, 0x035c, 0x03a1,
    0x03eb, 0x043b, 0x0491, 0x04ee,
    0x0553, 0x05c0, 0x0635, 0x06b3,
    0x073c, 0x07d0, 0x086f, 0x091b,
    0x09d5, 0x0a9d, 0x0b76, 0x0c60,
    0x0d5c, 0x0e6c, 0x0f93, 0x10d0,
    0x1227, 0x1399, 0x1529, 0x16d9,
    0x18aa, 0x1aa2, 0x1cc1, 0x1f0b,
    0x2184, 0x2430, 0x2712, 0x2a2e,
    0x2d8b, 0x312b, 0x3516, 0x3951,
    0x3de2, 0x42cf, 0x4822, 0x4de1,
    0x5415, 0x5ac8, 0x6203, 0x69d2,
    0x7240, 0x7b5a, 0x852d, 0x8fc9,
    0x9b3d, 0xa79b, 0xb4f5, 0xc35f,
    0xd2f0,
};

const std::vector<uint16_t> BIG_STEPS = {
    0x0190, 0x021f, 0x02e2, 0x03eb,
    0x0553, 0x073c, 0x09d5, 0x0d5c,
    0x1227, 0x18aa, 0x2184, 0x2d8b,
    0x3de2, 0x5415, 0x7240, 0x9b3d,
    0xd2f0,
};

BrightnessController::BrightnessController() = default;

BrightnessController::~BrightnessController() {
    shutdown();
}

bool BrightnessController::initialize() {
    if (hid_init() != 0) {
        return false;
    }

    // Enumerate HID devices to find LG Ultrafine monitor
    hid_device_info* devs = hid_enumerate(0x0, 0x0);
    hid_device_info* cur_dev = devs;
    char* monitor_path = nullptr;

    while (cur_dev) {
        if (cur_dev->vendor_id == LG_VENDOR_ID) {
            if (cur_dev->product_string &&
                wcsstr(cur_dev->product_string, L"BRIGHTNESS")) {
                monitor_path = cur_dev->path;
                m_monitorName = cur_dev->product_string;
                break;
            }
        }
        cur_dev = cur_dev->next;
    }

    if (!monitor_path) {
        hid_free_enumeration(devs);
        return false;
    }

    // Open the device
    m_handle = hid_open_path(monitor_path);
    hid_free_enumeration(devs);

    if (!m_handle) {
        return false;
    }

    m_connected = true;
    return true;
}

void BrightnessController::shutdown() {
    if (m_alsSensor) {
        m_alsSensor->shutdown();
        m_alsSensor.reset();
    }

    if (m_handle) {
        hid_close(static_cast<hid_device*>(m_handle));
        m_handle = nullptr;
    }
    m_connected = false;
    hid_exit();
}

uint16_t BrightnessController::getRawBrightness() {
    if (!m_handle) return MIN_BRIGHTNESS;

    uint8_t data[7] = { 0 };
    int res = hid_get_feature_report(static_cast<hid_device*>(m_handle), data, sizeof(data));
    if (res < 0) {
        return MIN_BRIGHTNESS;
    }

    return data[1] + (data[2] << 8);
}

void BrightnessController::setRawBrightness(uint16_t val) {
    if (!m_handle) return;

    // Clamp value
    val = std::clamp(val, MIN_BRIGHTNESS, MAX_BRIGHTNESS);

    uint8_t data[7] = {
        0x00,
        static_cast<uint8_t>(val & 0x00ff),
        static_cast<uint8_t>((val >> 8) & 0x00ff),
        0x00, 0x00, 0x00, 0x00
    };

    hid_send_feature_report(static_cast<hid_device*>(m_handle), data, sizeof(data));

    if (m_callback) {
        m_callback(rawToPercent(val));
    }
}

int BrightnessController::getBrightness() {
    return rawToPercent(getRawBrightness());
}

void BrightnessController::setBrightness(int percent) {
    setRawBrightness(percentToRaw(percent));
}

void BrightnessController::stepUp(bool bigStep) {
    uint16_t current = getRawBrightness();
    const auto& steps = bigStep ? BIG_STEPS : SMALL_STEPS;
    uint16_t next = findNextStep(current, steps);
    setRawBrightness(next);
}

void BrightnessController::stepDown(bool bigStep) {
    uint16_t current = getRawBrightness();
    const auto& steps = bigStep ? BIG_STEPS : SMALL_STEPS;
    uint16_t prev = findPrevStep(current, steps);
    setRawBrightness(prev);
}

uint16_t BrightnessController::findNextStep(uint16_t val, const std::vector<uint16_t>& steps) {
    auto it = std::upper_bound(steps.begin(), steps.end(), val);
    if (it != steps.end()) {
        return *it;
    }
    return steps.back();
}

uint16_t BrightnessController::findPrevStep(uint16_t val, const std::vector<uint16_t>& steps) {
    auto it = std::lower_bound(steps.begin(), steps.end(), val);
    if (it != steps.begin()) {
        --it;
        return *it;
    }
    return steps.front();
}

int BrightnessController::rawToPercent(uint16_t raw) {
    float range = MAX_BRIGHTNESS - MIN_BRIGHTNESS;
    float normalized = static_cast<float>(raw - MIN_BRIGHTNESS) / range;
    return static_cast<int>(normalized * 100.0f);
}

uint16_t BrightnessController::percentToRaw(int percent) {
    percent = std::clamp(percent, 0, 100);
    float range = MAX_BRIGHTNESS - MIN_BRIGHTNESS;
    return static_cast<uint16_t>(MIN_BRIGHTNESS + (range * percent / 100.0f));
}

// ============================================================================
// Ambient Light Sensor (ALS) Support - Using Windows Sensor API
// ============================================================================

bool BrightnessController::initializeALS() {
    if (m_alsSensor && m_alsSensor->isAvailable()) {
        return true;  // Already initialized
    }

    m_alsSensor = std::make_unique<als::ALSSensor>();
    return m_alsSensor->initialize();
}

bool BrightnessController::hasALS() const {
    return m_alsSensor && m_alsSensor->isAvailable();
}

std::wstring BrightnessController::getALSName() const {
    if (m_alsSensor && m_alsSensor->isAvailable()) {
        return m_alsSensor->getSensorName();
    }
    return L"No ALS";
}

float BrightnessController::getAmbientLight() {
    if (!m_alsSensor || !m_alsSensor->isAvailable()) {
        return m_lastAmbientLight;
    }

    m_lastAmbientLight = m_alsSensor->getLux();
    return m_lastAmbientLight;
}

void BrightnessController::setAutoBrightness(bool enabled) {
    if (enabled && (!m_alsSensor || !m_alsSensor->isAvailable())) {
        // Try to initialize ALS if not already done
        initializeALS();
    }
    m_autoBrightnessEnabled = enabled && m_alsSensor && m_alsSensor->isAvailable();
}

void BrightnessController::updateAutoBrightness() {
    if (!m_autoBrightnessEnabled || !m_alsSensor || !m_alsSensor->isAvailable()) {
        return;
    }

    float lux = getAmbientLight();

    // Auto-brightness algorithm: map lux to brightness percentage
    // Based on typical indoor/outdoor lighting levels:
    // - 0-50 lux: very dark (10-20% brightness)
    // - 50-200 lux: dim indoor (20-40% brightness)
    // - 200-500 lux: normal indoor (40-70% brightness)
    // - 500-1000 lux: bright indoor (70-90% brightness)
    // - 1000+ lux: very bright/outdoor (90-100% brightness)

    int targetBrightness = 10;  // Default minimum

    if (lux < 50.0f) {
        // Very dark: 10-20%
        targetBrightness = 10 + static_cast<int>(lux * 0.2f);
    } else if (lux < 200.0f) {
        // Dim indoor: 20-40%
        targetBrightness = 20 + static_cast<int>((lux - 50.0f) * 0.133f);
    } else if (lux < 500.0f) {
        // Normal indoor: 40-70%
        targetBrightness = 40 + static_cast<int>((lux - 200.0f) * 0.1f);
    } else if (lux < 1000.0f) {
        // Bright indoor: 70-90%
        targetBrightness = 70 + static_cast<int>((lux - 500.0f) * 0.04f);
    } else {
        // Very bright/outdoor: 90-100%
        float excess = std::min(lux - 1000.0f, 1000.0f);
        targetBrightness = 90 + static_cast<int>(excess * 0.01f);
    }

    targetBrightness = std::clamp(targetBrightness, 10, 100);

    // Apply smooth adjustment: only change if difference is significant (>5%)
    int currentBrightness = getBrightness();
    if (std::abs(currentBrightness - targetBrightness) > 5) {
        setBrightness(targetBrightness);
    }
}

} // namespace brightness
