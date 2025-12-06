#pragma once

#include <Windows.h>
#include <d3d11.h>
#include <functional>
#include <string>

namespace ui {

class UIRenderer {
public:
    UIRenderer();
    ~UIRenderer();

    // Initialize DirectX and ImGui
    bool initialize(HWND hwnd);
    void shutdown();

    // Render frame
    void beginFrame();
    void endFrame();

    // Apply custom theme
    void applyDarkTheme();

    // Set brightness callback
    using BrightnessChangedCallback = std::function<void(int)>;
    void setBrightnessCallback(BrightnessChangedCallback callback) {
        m_brightnessCallback = callback;
    }

    // Update displayed brightness
    void setBrightness(int brightness) { m_currentBrightness = brightness; }
    int getBrightness() const { return m_currentBrightness; }

    // Set connected status
    void setConnected(bool connected) { m_connected = connected; }
    void setMonitorName(const std::wstring& name);

    // Render main UI
    void renderMainUI();

    // Get DirectX device for window transparency
    ID3D11Device* getDevice() const { return m_device; }
    ID3D11DeviceContext* getContext() const { return m_context; }

private:
    bool createDeviceD3D(HWND hwnd);
    void cleanupDeviceD3D();
    void createRenderTarget();
    void cleanupRenderTarget();

    ID3D11Device* m_device = nullptr;
    ID3D11DeviceContext* m_context = nullptr;
    IDXGISwapChain* m_swapChain = nullptr;
    ID3D11RenderTargetView* m_renderTargetView = nullptr;

    HWND m_hwnd = nullptr;
    int m_currentBrightness = 50;
    bool m_connected = false;
    std::string m_monitorName = "LG Ultrafine";

    BrightnessChangedCallback m_brightnessCallback;
};

} // namespace ui
