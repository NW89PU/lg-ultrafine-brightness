#pragma once

#include <Windows.h>

namespace als {

// Forward declare event sink class
class ALSSensorEventSink;

class ALSSensor {
public:
    ALSSensor();
    ~ALSSensor();

    // Prevent copying
    ALSSensor(const ALSSensor&) = delete;
    ALSSensor& operator=(const ALSSensor&) = delete;

    // Initialize the sensor
    bool initialize();
    void shutdown();

    // Check if sensor is available
    bool isAvailable() const { return m_available; }

    // Get current ambient light level in lux
    float getLux();

    // Get sensor name
    const wchar_t* getSensorName() const { return m_sensorName; }

    // Called by event sink when new data arrives
    void updateLux(float lux);

private:
    void* m_sensorManager = nullptr;  // ISensorManager*
    void* m_sensor = nullptr;          // ISensor*
    void* m_eventSink = nullptr;       // ALSSensorEventSink*
    bool m_available = false;
    wchar_t m_sensorName[256] = L"Ambient Light Sensor";
    float m_lastLux = 0.0f;
};

} // namespace als
