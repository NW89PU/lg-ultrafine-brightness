#include "app.h"
#include "resource.h"
#include "settings.h"
#include "ui.h"
#include <imgui.h>
#include <imgui_impl_win32.h>
#include <algorithm>
#include <iostream>

#undef min
#undef max

// Forward declare message handler from imgui_impl_win32.cpp
extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

namespace app {

Application* Application::s_instance = nullptr;

Application::Application() {
    s_instance = this;
    m_brightness = std::make_unique<brightness::BrightnessController>();
    m_ui = std::make_unique<ui::UIRenderer>();
    m_tray = std::make_unique<tray::TrayIcon>();
    m_hotkey = std::make_unique<hotkey::HotkeyManager>();
}

Application::~Application() {
    shutdown();
    s_instance = nullptr;
}

bool Application::initialize(HINSTANCE hInstance) {
    m_hInstance = hInstance;

    // Load layout first — createWindow uses the configured window height.
    m_layout = settings::loadLayout();

    if (!createWindow(hInstance)) {
        return false;
    }

    // Layout / theme must be set before UI initialize, because applyDarkTheme()
    // reads accent/curve color hex values from it.
    m_ui->setLayout(m_layout);

    if (!m_ui->initialize(m_hwnd)) {
        return false;
    }

    // Initialize brightness controller
    bool connected = m_brightness->initialize();
    m_ui->setConnected(connected);
    if (connected) {
        m_ui->setMonitorName(m_brightness->getMonitorName());
        m_ui->setBrightness(m_brightness->getBrightness());

        // Try to initialize ALS
#ifdef _DEBUG
        std::wcout << L"[App] Attempting to initialize ALS..." << std::endl;
#endif
        bool hasALS = m_brightness->initializeALS();
#ifdef _DEBUG
        std::wcout << L"[App] ALS initialization result: " << (hasALS ? L"SUCCESS" : L"FAILED") << std::endl;
#endif
        m_ui->setHasALS(hasALS);
        if (hasALS) {
            std::wstring alsName = m_brightness->getALSName();
#ifdef _DEBUG
            std::wcout << L"[App] ALS Name: " << alsName << std::endl;
#endif
            m_ui->setALSName(alsName);
        }
    }

    // Initialize tray icon
    m_tray->initialize(m_hwnd, hInstance, tray::TrayIcon::WM_TRAYICON);
    m_tray->show();

    // Initialize hotkeys
    m_hotkey->initialize(m_hwnd);
    m_hotkey->registerDefaultHotkeys();

    // Load persisted auto-brightness settings and propagate to controller + UI,
    // including the last-known Auto Brightness checkbox state.
    {
        auto s = settings::load();
        m_brightness->setAutoBrightnessSettings(s);
        m_ui->setAutoBrightnessSettings(s);
        if (s.enabled) {
            m_brightness->setAutoBrightness(true);
        }
        m_ui->setAutoBrightnessEnabled(m_brightness->isAutoBrightnessEnabled());
    }

    // Layout already propagated to UI before initialize().

    setupCallbacks();

    return true;
}

bool Application::createWindow(HINSTANCE hInstance) {
    WNDCLASSEXW wc = {};
    wc.cbSize = sizeof(WNDCLASSEXW);
    wc.style = CS_CLASSDC;
    wc.lpfnWndProc = WndProc;
    wc.hInstance = hInstance;
    wc.hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_APPICON));
    wc.hIconSm = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_APPICON));
    wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wc.lpszClassName = L"LGUltrafineBrightnessClass";

    RegisterClassExW(&wc);

    // Get DPI scale
    HDC hdc = GetDC(nullptr);
    float dpiScale = GetDeviceCaps(hdc, LOGPIXELSX) / 96.0f;
    ReleaseDC(nullptr, hdc);

    // Calculate DPI-scaled client area size
    int clientWidth = static_cast<int>(ui::MAIN_COL_DP * dpiScale);
    int clientHeight = static_cast<int>(m_layout.windowHeightDp * dpiScale);

    // Adjust for window frame and title bar
    RECT rect = { 0, 0, clientWidth, clientHeight };
    DWORD style = WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX;
    AdjustWindowRect(&rect, style, FALSE);
    int width = rect.right - rect.left;
    int height = rect.bottom - rect.top;

    int screenWidth = GetSystemMetrics(SM_CXSCREEN);
    int screenHeight = GetSystemMetrics(SM_CYSCREEN);
    int x = (screenWidth - width) / 2;
    int y = (screenHeight - height) / 2;

    m_hwnd = CreateWindowExW(
        0,
        wc.lpszClassName,
        L"LG Ultrafine Brightness",
        WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX,
        x, y, width, height,
        nullptr, nullptr, hInstance, nullptr);

    if (!m_hwnd) {
        return false;
    }

    // Start hidden — only the tray icon is visible. User shows the window
    // via the tray (left-click or context menu).
    ShowWindow(m_hwnd, SW_HIDE);

    return true;
}

void Application::setupCallbacks() {
    // UI brightness change callback
    m_ui->setBrightnessCallback([this](int percent) {
        updateBrightness(percent);
    });

    // UI close button callback
    m_ui->setCloseCallback([this]() {
        hideWindow();
    });

    // Auto-brightness checkbox toggled: apply, sync UI, persist.
    m_ui->setAutoBrightnessCallback([this](bool enabled) {
        if (!m_brightness) return;
        m_brightness->setAutoBrightness(enabled);
        m_ui->setAutoBrightnessEnabled(m_brightness->isAutoBrightnessEnabled());
        auto s = m_brightness->getAutoBrightnessSettings();
        s.enabled = m_brightness->isAutoBrightnessEnabled();
        settings::save(s);
    });

    // Curve / hysteresis edited in the side panel: persist while preserving
    // the current enabled state (the slider callback doesn't know about it).
    m_ui->setAutoBrightnessSettingsCallback([this](const settings::AutoBrightnessSettings& s) {
        if (!m_brightness) return;
        auto merged = s;
        merged.enabled = m_brightness->isAutoBrightnessEnabled();
        m_brightness->setAutoBrightnessSettings(merged);
        settings::save(merged);
    });

    // Window-resize request from the UI (Settings panel toggled)
    m_ui->setResizeCallback([this](int clientWidthPx, int clientHeightPx) {
        if (!m_hwnd) return;
        RECT rect = { 0, 0, clientWidthPx, clientHeightPx };
        DWORD style = static_cast<DWORD>(GetWindowLongPtrW(m_hwnd, GWL_STYLE));
        AdjustWindowRect(&rect, style, FALSE);
        SetWindowPos(m_hwnd, nullptr, 0, 0,
                     rect.right - rect.left, rect.bottom - rect.top,
                     SWP_NOMOVE | SWP_NOZORDER);
    });

    // Autostart is controlled from the tray context menu.

    // Tray callbacks
    m_tray->setShowCallback([this]() {
        showWindow();
    });

    m_tray->setExitCallback([this]() {
        m_running = false;
        PostQuitMessage(0);
    });

    // Hotkey callbacks - fixed 5% step
    m_hotkey->setBrightnessUpCallback([this]() {
        if (m_brightness->isConnected()) {
            int current = m_brightness->getBrightness();
            int newVal = std::min(100, current + 5);
            m_brightness->setBrightness(newVal);
            // Update UI directly with the new value (no HID read needed)
            m_ui->setBrightness(newVal);
            // Update tray tooltip
            wchar_t tooltip[64];
            swprintf_s(tooltip, L"LG Ultrafine Brightness: %d%%", newVal);
            m_tray->setTooltip(tooltip);
        }
    });

    m_hotkey->setBrightnessDownCallback([this]() {
        if (m_brightness->isConnected()) {
            int current = m_brightness->getBrightness();
            int newVal = std::max(0, current - 5);
            m_brightness->setBrightness(newVal);
            // Update UI directly with the new value (no HID read needed)
            m_ui->setBrightness(newVal);
            // Update tray tooltip
            wchar_t tooltip[64];
            swprintf_s(tooltip, L"LG Ultrafine Brightness: %d%%", newVal);
            m_tray->setTooltip(tooltip);
        }
    });
}

void Application::updateBrightness(int percent) {
    if (m_brightness->isConnected()) {
        m_brightness->setBrightness(percent);
    }
}

void Application::refreshBrightness() {
    if (m_brightness->isConnected()) {
        int brightness = m_brightness->getBrightness();
        m_ui->setBrightness(brightness);

        // Update tray tooltip
        wchar_t tooltip[64];
        swprintf_s(tooltip, L"LG Ultrafine Brightness: %d%%", brightness);
        m_tray->setTooltip(tooltip);
    }
}

void Application::showWindow() {
    ShowWindow(m_hwnd, SW_SHOW);
    SetForegroundWindow(m_hwnd);
    m_windowVisible = true;
    refreshBrightness();
}

void Application::hideWindow() {
    ShowWindow(m_hwnd, SW_HIDE);
    m_windowVisible = false;
}

void Application::toggleWindow() {
    if (m_windowVisible) {
        hideWindow();
    } else {
        showWindow();
    }
}

int Application::run() {
    MSG msg;
    ZeroMemory(&msg, sizeof(msg));

    while (m_running) {
        while (PeekMessage(&msg, nullptr, 0U, 0U, PM_REMOVE)) {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
            if (msg.message == WM_QUIT) {
                m_running = false;
            }
        }

        if (!m_running) break;

        // Update auto-brightness if enabled
        updateAutoBrightness();

        // Only render when window is visible
        if (m_windowVisible) {
            m_ui->beginFrame();
            m_ui->renderMainUI();
            m_ui->endFrame();
            // Limit to ~60 FPS to reduce CPU usage
            Sleep(16);
        } else {
            // Sleep a bit to reduce CPU usage when hidden
            Sleep(10);  // 10ms for responsive hotkeys
        }
    }

    return static_cast<int>(msg.wParam);
}

void Application::shutdown() {
    m_hotkey->shutdown();
    m_tray->shutdown();
    m_ui->shutdown();
    m_brightness->shutdown();

    if (m_hwnd) {
        DestroyWindow(m_hwnd);
        m_hwnd = nullptr;
    }
}

LRESULT WINAPI Application::WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    if (ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam))
        return true;

    Application* app = s_instance;

    switch (msg) {
    case WM_SHOWWINDOW_FROM_INSTANCE:
        // Another instance wants to show this window
        if (app) {
            app->showWindow();
        }
        return 0;

    case WM_SIZE:
        if (app && app->m_ui && app->m_ui->getDevice() != nullptr && wParam != SIZE_MINIMIZED) {
            app->m_ui->resizeSwapChain(LOWORD(lParam), HIWORD(lParam));
        }
        return 0;

    case WM_SYSCOMMAND:
        // Allow normal minimize behavior
        // Only intercept close, not minimize
        break;

    case WM_CLOSE:
        // Hide to tray instead of closing
        if (app) {
            app->hideWindow();
            return 0;
        }
        break;

    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;

    case WM_HOTKEY:
        if (app && app->m_hotkey) {
            app->m_hotkey->handleHotkey(wParam);
        }
        return 0;

    case tray::TrayIcon::WM_TRAYICON:
        if (app && app->m_tray) {
            app->m_tray->handleMessage(wParam, lParam);
        }
        return 0;

    case WM_KEYDOWN:
        if (wParam == VK_ESCAPE) {
            if (app) app->hideWindow();
            return 0;
        }
        break;
    }

    return DefWindowProc(hWnd, msg, wParam, lParam);
}

void Application::updateAutoBrightness() {
#ifdef _DEBUG
    static int callCount = 0;
    if (++callCount % 10 == 0) {  // Only log every 10th call to avoid spam
        std::wcout << L"[App] updateAutoBrightness called " << callCount << L" times" << std::endl;
    }
#endif

    if (!m_brightness || !m_brightness->hasALS()) {
#ifdef _DEBUG
        if (callCount % 10 == 0) {
            std::wcout << L"[App] No ALS available, hasALS=" << (m_brightness ? m_brightness->hasALS() : false) << std::endl;
        }
#endif
        return;  // No ALS available
    }

    DWORD currentTime = GetTickCount();
    if (currentTime - m_lastAutoBrightnessUpdate >= AUTO_BRIGHTNESS_INTERVAL_MS) {
        // Always update ambient light reading in UI (even if auto-brightness is off)
        float ambientLight = m_brightness->getAmbientLight();
#ifdef _DEBUG
        std::wcout << L"[App] Updating UI with lux: " << ambientLight << std::endl;
#endif
        m_ui->setAmbientLight(ambientLight);

        // Only adjust brightness if auto-brightness is enabled
        if (m_brightness->isAutoBrightnessEnabled()) {
            m_brightness->updateAutoBrightness();
            // Update UI to reflect brightness changes
            refreshBrightness();
        }

        m_lastAutoBrightnessUpdate = currentTime;
    }
}

} // namespace app
