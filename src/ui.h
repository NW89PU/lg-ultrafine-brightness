#pragma once

#include "settings.h"

#include <Windows.h>
#include <d3d11.h>
#include <functional>
#include <string>

namespace ui {

// Layout constants in device-independent pixels (96 DPI baseline).
// Window dimensions are derived from these, scaled by current DPI.
constexpr int MAIN_COL_DP = 350;
constexpr int SETTINGS_COL_DP = 350;
constexpr int CONTENT_HEIGHT_DP = 580;

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

    // Set close callback
    using CloseCallback = std::function<void()>;
    void setCloseCallback(CloseCallback callback) {
        m_closeCallback = callback;
    }

    // Set auto-brightness callback
    using AutoBrightnessCallback = std::function<void(bool)>;
    void setAutoBrightnessCallback(AutoBrightnessCallback callback) {
        m_autoBrightnessCallback = callback;
    }

    // Update displayed brightness
    void setBrightness(int brightness) { m_currentBrightness = brightness; }
    int getBrightness() const { return m_currentBrightness; }

    // Set auto-brightness state
    void setAutoBrightnessEnabled(bool enabled) { m_autoBrightnessEnabled = enabled; }
    void setAmbientLight(float lux) { m_ambientLight = lux; }
    void setHasALS(bool hasALS) { m_hasALS = hasALS; }
    void setALSName(const std::wstring& name);

    // Set connected status
    void setConnected(bool connected) { m_connected = connected; }
    void setMonitorName(const std::wstring& name);

    // Auto-brightness settings (read by sliders, written when sliders move)
    void setAutoBrightnessSettings(const settings::AutoBrightnessSettings& s) { m_autoSettings = s; }
    const settings::AutoBrightnessSettings& getAutoBrightnessSettings() const { return m_autoSettings; }
    using AutoBrightnessSettingsChangedCallback = std::function<void(const settings::AutoBrightnessSettings&)>;
    void setAutoBrightnessSettingsCallback(AutoBrightnessSettingsChangedCallback cb) {
        m_autoSettingsCallback = cb;
    }

    // Window-resize callback. Fired when the settings panel is toggled.
    // Receives the desired client-area size in pixels (already DPI-scaled).
    using ResizeCallback = std::function<void(int clientWidthPx, int clientHeightPx)>;
    void setResizeCallback(ResizeCallback cb) { m_resizeCallback = cb; }

    // Pixel-size getters for the host window (in DPI-scaled pixels).
    int getClientWidthPx() const;
    int getClientHeightPx() const;

    // Layout offsets read from layout.ini at startup.
    void setLayout(const settings::LayoutSettings& l) { m_layout = l; }

    // Render main UI
    void renderMainUI();

    // Resize the D3D11 swap chain to match the new client area.
    // Call from WM_SIZE after a window resize.
    void resizeSwapChain(UINT width, UINT height);

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
    float m_dpiScale = 1.0f;

    bool m_autoBrightnessEnabled = false;
    float m_ambientLight = 0.0f;
    bool m_hasALS = false;
    std::string m_alsName;

    settings::AutoBrightnessSettings m_autoSettings;
    settings::LayoutSettings m_layout;
    bool m_settingsExpanded = false;

    BrightnessChangedCallback m_brightnessCallback;
    CloseCallback m_closeCallback;
    AutoBrightnessCallback m_autoBrightnessCallback;
    AutoBrightnessSettingsChangedCallback m_autoSettingsCallback;
    ResizeCallback m_resizeCallback;
};

} // namespace ui
