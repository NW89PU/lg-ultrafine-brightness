#pragma once

#include <Windows.h>
#include <memory>

#include "brightness.h"
#include "ui.h"
#include "tray.h"
#include "hotkey.h"

namespace app {

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

    HINSTANCE m_hInstance = nullptr;
    HWND m_hwnd = nullptr;
    bool m_running = true;
    bool m_windowVisible = true;

    std::unique_ptr<brightness::BrightnessController> m_brightness;
    std::unique_ptr<ui::UIRenderer> m_ui;
    std::unique_ptr<tray::TrayIcon> m_tray;
    std::unique_ptr<hotkey::HotkeyManager> m_hotkey;

    static Application* s_instance;
};

} // namespace app
