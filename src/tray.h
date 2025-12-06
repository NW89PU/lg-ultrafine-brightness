#pragma once

#include <Windows.h>
#include <functional>

namespace tray {

class TrayIcon {
public:
    TrayIcon();
    ~TrayIcon();

    // Initialize tray icon
    bool initialize(HWND hwnd, HINSTANCE hInstance, UINT callbackMsg);
    void shutdown();

    // Show/hide tray icon
    void show();
    void hide();

    // Update tooltip
    void setTooltip(const wchar_t* tooltip);

    // Show notification
    void showNotification(const wchar_t* title, const wchar_t* message);

    // Menu callbacks
    using MenuCallback = std::function<void()>;
    void setShowCallback(MenuCallback callback) { m_showCallback = callback; }
    void setExitCallback(MenuCallback callback) { m_exitCallback = callback; }

    // Handle tray messages
    void handleMessage(WPARAM wParam, LPARAM lParam);

    // Custom message ID
    static constexpr UINT WM_TRAYICON = WM_USER + 1;

private:
    void showContextMenu();

    HWND m_hwnd = nullptr;
    NOTIFYICONDATAW m_nid = {};
    bool m_visible = false;

    MenuCallback m_showCallback;
    MenuCallback m_exitCallback;
};

} // namespace tray
