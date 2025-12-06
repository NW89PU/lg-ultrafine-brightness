#include "app.h"
#include "resource.h"
#include <imgui.h>
#include <imgui_impl_win32.h>
#include <algorithm>

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

    if (!createWindow(hInstance)) {
        return false;
    }

    if (!m_ui->initialize(m_hwnd)) {
        return false;
    }

    // Initialize brightness controller
    bool connected = m_brightness->initialize();
    m_ui->setConnected(connected);
    if (connected) {
        m_ui->setMonitorName(m_brightness->getMonitorName());
        m_ui->setBrightness(m_brightness->getBrightness());
    }

    // Initialize tray icon
    m_tray->initialize(m_hwnd, hInstance, tray::TrayIcon::WM_TRAYICON);
    m_tray->show();

    // Initialize hotkeys
    m_hotkey->initialize(m_hwnd);
    m_hotkey->registerDefaultHotkeys();

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
    int clientWidth = static_cast<int>(350 * dpiScale);
    int clientHeight = static_cast<int>(260 * dpiScale);

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

    ShowWindow(m_hwnd, SW_SHOWDEFAULT);
    UpdateWindow(m_hwnd);

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
            refreshBrightness();
        }
    });

    m_hotkey->setBrightnessDownCallback([this]() {
        if (m_brightness->isConnected()) {
            int current = m_brightness->getBrightness();
            int newVal = std::max(0, current - 5);
            m_brightness->setBrightness(newVal);
            refreshBrightness();
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

        // Only render when window is visible
        if (m_windowVisible) {
            m_ui->beginFrame();
            m_ui->renderMainUI();
            m_ui->endFrame();
        } else {
            // Sleep a bit to reduce CPU usage when hidden
            Sleep(100);
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
            // Handle resize if needed
        }
        return 0;

    case WM_SYSCOMMAND:
        // Minimize to tray instead of taskbar
        if ((wParam & 0xFFF0) == SC_MINIMIZE) {
            if (app) app->hideWindow();
            return 0;
        }
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

} // namespace app
