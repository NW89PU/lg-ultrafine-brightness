#include "tray.h"
#include "resource.h"
#include <shellapi.h>

namespace tray {

TrayIcon::TrayIcon() = default;

TrayIcon::~TrayIcon() {
    shutdown();
}

bool TrayIcon::initialize(HWND hwnd, HINSTANCE hInstance, UINT callbackMsg) {
    m_hwnd = hwnd;

    ZeroMemory(&m_nid, sizeof(m_nid));
    m_nid.cbSize = sizeof(NOTIFYICONDATAW);
    m_nid.hWnd = hwnd;
    m_nid.uID = 1;
    m_nid.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP;
    m_nid.uCallbackMessage = callbackMsg;

    // Load application icon from resources
    m_nid.hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_APPICON));
    if (!m_nid.hIcon) {
        // Fallback to default icon if loading fails
        m_nid.hIcon = LoadIcon(nullptr, IDI_APPLICATION);
    }

    wcscpy_s(m_nid.szTip, L"LG Ultrafine Brightness");

    return true;
}

void TrayIcon::shutdown() {
    if (m_visible) {
        hide();
    }
}

void TrayIcon::show() {
    if (!m_visible) {
        Shell_NotifyIconW(NIM_ADD, &m_nid);
        m_visible = true;
    }
}

void TrayIcon::hide() {
    if (m_visible) {
        Shell_NotifyIconW(NIM_DELETE, &m_nid);
        m_visible = false;
    }
}

void TrayIcon::setTooltip(const wchar_t* tooltip) {
    wcscpy_s(m_nid.szTip, tooltip);
    if (m_visible) {
        Shell_NotifyIconW(NIM_MODIFY, &m_nid);
    }
}

void TrayIcon::showNotification(const wchar_t* title, const wchar_t* message) {
    m_nid.uFlags |= NIF_INFO;
    wcscpy_s(m_nid.szInfoTitle, title);
    wcscpy_s(m_nid.szInfo, message);
    m_nid.dwInfoFlags = NIIF_INFO;

    Shell_NotifyIconW(NIM_MODIFY, &m_nid);

    // Reset info flags
    m_nid.uFlags &= ~NIF_INFO;
}

void TrayIcon::handleMessage(WPARAM wParam, LPARAM lParam) {
    switch (LOWORD(lParam)) {
    case WM_LBUTTONDBLCLK:
    case WM_LBUTTONUP:
        if (m_showCallback) {
            m_showCallback();
        }
        break;

    case WM_RBUTTONUP:
        showContextMenu();
        break;
    }
}

void TrayIcon::showContextMenu() {
    POINT pt;
    GetCursorPos(&pt);

    HMENU hMenu = CreatePopupMenu();
    AppendMenuW(hMenu, MF_STRING, 1, L"Show");
    AppendMenuW(hMenu, MF_SEPARATOR, 0, nullptr);
    AppendMenuW(hMenu, MF_STRING, 2, L"Exit");

    // Required for menu to work properly
    SetForegroundWindow(m_hwnd);

    UINT cmd = TrackPopupMenu(
        hMenu,
        TPM_RETURNCMD | TPM_NONOTIFY | TPM_RIGHTBUTTON,
        pt.x, pt.y, 0, m_hwnd, nullptr);

    DestroyMenu(hMenu);

    switch (cmd) {
    case 1:
        if (m_showCallback) m_showCallback();
        break;
    case 2:
        if (m_exitCallback) m_exitCallback();
        break;
    }
}

} // namespace tray
