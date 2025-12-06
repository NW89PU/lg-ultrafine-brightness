#include "als_sensor.h"
#include <sensorsapi.h>
#include <sensors.h>
#include <iostream>

#pragma comment(lib, "sensorsapi.lib")
#pragma comment(lib, "ole32.lib")
#pragma comment(lib, "oleaut32.lib")

namespace als {

// ============================================================================
// Event Sink Implementation - receives sensor data updates
// ============================================================================
class ALSSensorEventSink : public ISensorEvents {
public:
    ALSSensorEventSink(ALSSensor* owner) : m_refCount(1), m_owner(owner) {}

    // IUnknown methods
    STDMETHODIMP QueryInterface(REFIID riid, void** ppv) {
        if (ppv == NULL) return E_POINTER;

        if (riid == __uuidof(IUnknown)) {
            *ppv = static_cast<IUnknown*>(this);
        } else if (riid == __uuidof(ISensorEvents)) {
            *ppv = static_cast<ISensorEvents*>(this);
        } else {
            *ppv = NULL;
            return E_NOINTERFACE;
        }

        AddRef();
        return S_OK;
    }

    STDMETHODIMP_(ULONG) AddRef() {
        return InterlockedIncrement(&m_refCount);
    }

    STDMETHODIMP_(ULONG) Release() {
        ULONG count = InterlockedDecrement(&m_refCount);
        if (count == 0) {
            delete this;
        }
        return count;
    }

    // ISensorEvents methods
    STDMETHODIMP OnStateChanged(ISensor* pSensor, SensorState state) {
#ifdef _DEBUG
        std::wcout << L"[ALS Event] OnStateChanged: " << state << std::endl;
#endif
        return S_OK;
    }

    STDMETHODIMP OnDataUpdated(ISensor* pSensor, ISensorDataReport* pReport) {
        if (!pReport || !m_owner) return E_INVALIDARG;

        // Get lux value
        PROPVARIANT var;
        PropVariantInit(&var);

        HRESULT hr = pReport->GetSensorValue(SENSOR_DATA_TYPE_LIGHT_LEVEL_LUX, &var);

        if (SUCCEEDED(hr)) {
            float lux = 0.0f;
            if (var.vt == VT_R4) {
                lux = var.fltVal;
            } else if (var.vt == VT_R8) {
                lux = static_cast<float>(var.dblVal);
            }

#ifdef _DEBUG
            std::wcout << L"[ALS Event] OnDataUpdated: " << lux << L" lux" << std::endl;
#endif
            m_owner->updateLux(lux);
        }

        PropVariantClear(&var);
        return S_OK;
    }

    STDMETHODIMP OnEvent(ISensor* pSensor, REFGUID eventID, IPortableDeviceValues* pData) {
        return S_OK;
    }

    STDMETHODIMP OnLeave(REFSENSOR_ID sensorID) {
#ifdef _DEBUG
        std::wcout << L"[ALS Event] OnLeave" << std::endl;
#endif
        return S_OK;
    }

private:
    LONG m_refCount;
    ALSSensor* m_owner;
};

// ============================================================================
// ALSSensor Implementation
// ============================================================================

ALSSensor::ALSSensor() = default;

ALSSensor::~ALSSensor() {
    shutdown();
}

bool ALSSensor::initialize() {
    if (m_available) {
        return true;  // Already initialized
    }

#ifdef _DEBUG
    std::wcout << L"=== Initializing Windows Sensor API for ALS ===\n" << std::endl;
#endif

    // Initialize COM
    HRESULT hr = CoInitializeEx(NULL, COINIT_MULTITHREADED);
    if (FAILED(hr) && hr != RPC_E_CHANGED_MODE) {
#ifdef _DEBUG
        std::wcout << L"Failed to initialize COM: 0x" << std::hex << hr << std::dec << std::endl;
#endif
        return false;
    }

    // Create Sensor Manager
    ISensorManager* pSensorManager = NULL;
    hr = CoCreateInstance(CLSID_SensorManager, NULL, CLSCTX_INPROC_SERVER,
                          IID_PPV_ARGS(&pSensorManager));

    if (FAILED(hr)) {
#ifdef _DEBUG
        std::wcout << L"Failed to create SensorManager: 0x" << std::hex << hr << std::dec << std::endl;
#endif
        return false;
    }

    m_sensorManager = pSensorManager;

    // Get Ambient Light Sensors
    ISensorCollection* pSensorCollection = NULL;
    hr = pSensorManager->GetSensorsByType(SENSOR_TYPE_AMBIENT_LIGHT, &pSensorCollection);

    if (FAILED(hr)) {
#ifdef _DEBUG
        std::wcout << L"Failed to get sensors by type: 0x" << std::hex << hr << std::dec << std::endl;
#endif
        pSensorManager->Release();
        m_sensorManager = nullptr;
        return false;
    }

    ULONG count = 0;
    pSensorCollection->GetCount(&count);
#ifdef _DEBUG
    std::wcout << L"Found " << count << L" ambient light sensor(s)" << std::endl;
#endif

    if (count == 0) {
#ifdef _DEBUG
        std::wcout << L"No ambient light sensors found!" << std::endl;
#endif
        pSensorCollection->Release();
        pSensorManager->Release();
        m_sensorManager = nullptr;
        return false;
    }

    // Get the first sensor
    ISensor* pSensor = NULL;
    hr = pSensorCollection->GetAt(0, &pSensor);
    pSensorCollection->Release();

    if (FAILED(hr)) {
#ifdef _DEBUG
        std::wcout << L"Failed to get sensor: 0x" << std::hex << hr << std::dec << std::endl;
#endif
        pSensorManager->Release();
        m_sensorManager = nullptr;
        return false;
    }

    m_sensor = pSensor;

    // Get sensor friendly name
    BSTR friendlyName = NULL;
    hr = pSensor->GetFriendlyName(&friendlyName);
    if (SUCCEEDED(hr) && friendlyName) {
        wcsncpy_s(m_sensorName, friendlyName, _TRUNCATE);
        SysFreeString(friendlyName);
    }

    // Create and register event sink
    ALSSensorEventSink* pEventSink = new ALSSensorEventSink(this);
    hr = pSensor->SetEventSink(pEventSink);

    if (FAILED(hr)) {
#ifdef _DEBUG
        std::wcout << L"Failed to set event sink: 0x" << std::hex << hr << std::dec << std::endl;
#endif
        pEventSink->Release();
        pSensor->Release();
        m_sensor = nullptr;
        pSensorManager->Release();
        m_sensorManager = nullptr;
        return false;
    }

    m_eventSink = pEventSink;
#ifdef _DEBUG
    std::wcout << L"Event sink registered successfully" << std::endl;
#endif

    // Set report interval to 500ms
    IPortableDeviceValues* pValues = NULL;
    hr = CoCreateInstance(__uuidof(PortableDeviceValues), NULL, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&pValues));
    if (SUCCEEDED(hr)) {
        PROPVARIANT var;
        PropVariantInit(&var);
        var.vt = VT_UI4;
        var.ulVal = 500;  // 500ms report interval

        hr = pValues->SetValue(SENSOR_PROPERTY_CURRENT_REPORT_INTERVAL, &var);
        if (SUCCEEDED(hr)) {
            hr = pSensor->SetProperties(pValues, NULL);
            if (SUCCEEDED(hr)) {
#ifdef _DEBUG
                std::wcout << L"Set report interval to 500ms" << std::endl;
#endif
            } else {
#ifdef _DEBUG
                std::wcout << L"SetProperties failed: 0x" << std::hex << hr << std::dec << std::endl;
#endif
            }
        }

        PropVariantClear(&var);
        pValues->Release();
    }

    // Get initial reading
    ISensorDataReport* pReport = NULL;
    hr = pSensor->GetData(&pReport);
    if (SUCCEEDED(hr)) {
        PROPVARIANT var;
        PropVariantInit(&var);

        hr = pReport->GetSensorValue(SENSOR_DATA_TYPE_LIGHT_LEVEL_LUX, &var);
        if (SUCCEEDED(hr)) {
            if (var.vt == VT_R4) {
                m_lastLux = var.fltVal;
            } else if (var.vt == VT_R8) {
                m_lastLux = static_cast<float>(var.dblVal);
            }
#ifdef _DEBUG
            std::wcout << L"Initial lux reading: " << m_lastLux << L" lux" << std::endl;
#endif
        }

        PropVariantClear(&var);
        pReport->Release();
    }

#ifdef _DEBUG
    std::wcout << L"Successfully initialized ALS: " << m_sensorName << std::endl;
#endif

    m_available = true;
    return true;
}

void ALSSensor::shutdown() {
    if (m_eventSink) {
        // Unregister event sink
        if (m_sensor) {
            static_cast<ISensor*>(m_sensor)->SetEventSink(NULL);
        }
        static_cast<ALSSensorEventSink*>(m_eventSink)->Release();
        m_eventSink = nullptr;
    }

    if (m_sensor) {
        static_cast<ISensor*>(m_sensor)->Release();
        m_sensor = nullptr;
    }

    if (m_sensorManager) {
        static_cast<ISensorManager*>(m_sensorManager)->Release();
        m_sensorManager = nullptr;
    }

    m_available = false;
}

float ALSSensor::getLux() {
    return m_lastLux;
}

void ALSSensor::updateLux(float lux) {
    m_lastLux = lux;
}

} // namespace als
