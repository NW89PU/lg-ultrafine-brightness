#pragma once

#include <Windows.h>
#include <memory>

#include "brightness.h"
#include "ui.h"
#include "tray.h"
#include "hotkey.h"
#include "settings.h"

namespace app {

// Custom message for showing window from another instance
constexpr UINT WM_SHOWWINDOW_FROM_INSTANCE = WM_USER + 100;

class Application {
public:
    Application();
    ~Application();

    // Initialize the application
    bool initialize(HINSTANCE hInstance);

    // Run the main loop
    int run();

    // Shutdown
    void shutdown();

    // Window procedure (static for Win32)
    static LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

private:
    bool createWindow(HINSTANCE hInstance);
    void setupCallbacks();
    void updateBrightness(int percent);
    void refreshBrightness();

    void showWindow();
    void hideWindow();
    void toggleWindow();

    void updateAutoBrightness();

    HINSTANCE m_hInstance = nullptr;
    HWND m_hwnd = nullptr;
    bool m_running = true;
    bool m_windowVisible = false;  // start minimized to tray

    DWORD m_lastAutoBrightnessUpdate = 0;
    static constexpr DWORD AUTO_BRIGHTNESS_INTERVAL_MS = 500;  // Update every 500ms

    std::unique_ptr<brightness::BrightnessController> m_brightness;
    std::unique_ptr<ui::UIRenderer> m_ui;
    std::unique_ptr<tray::TrayIcon> m_tray;
    std::unique_ptr<hotkey::HotkeyManager> m_hotkey;

    settings::LayoutSettings m_layout;

    static Application* s_instance;
};

} // namespace app
