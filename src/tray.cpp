#include "tray.h"
#include "autostart.h"
#include "resource.h"
#include <shellapi.h>
#include <cmath>

namespace tray {

// True if Windows is set to a dark app theme.
static bool isDarkMode() {
    DWORD value = 1;
    DWORD size = sizeof(value);
    if (RegGetValueW(HKEY_CURRENT_USER,
                     L"Software\\Microsoft\\Windows\\CurrentVersion\\Themes\\Personalize",
                     L"SystemUsesLightTheme",
                     RRF_RT_REG_DWORD, nullptr, &value, &size) != ERROR_SUCCESS) {
        return false;
    }
    return value == 0;
}

// Build a 32x32 ARGB icon: white filled disc + 8 short rays, on a transparent
// background. Used in dark mode where the bundled brightness.ico is too dark
// to see against a black taskbar.
static HICON createWhiteSunIcon() {
    constexpr int N = 32;

    BITMAPV5HEADER hdr = {};
    hdr.bV5Size = sizeof(BITMAPV5HEADER);
    hdr.bV5Width = N;
    hdr.bV5Height = N;
    hdr.bV5Planes = 1;
    hdr.bV5BitCount = 32;
    hdr.bV5Compression = BI_BITFIELDS;
    hdr.bV5RedMask   = 0x00FF0000;
    hdr.bV5GreenMask = 0x0000FF00;
    hdr.bV5BlueMask  = 0x000000FF;
    hdr.bV5AlphaMask = 0xFF000000;

    HDC hdcScreen = GetDC(nullptr);
    void* bits = nullptr;
    HBITMAP hbmColor = CreateDIBSection(hdcScreen, reinterpret_cast<BITMAPINFO*>(&hdr),
                                        DIB_RGB_COLORS, &bits, nullptr, 0);
    ReleaseDC(nullptr, hdcScreen);
    if (!hbmColor || !bits) return nullptr;

    DWORD* px = static_cast<DWORD*>(bits);
    for (int i = 0; i < N * N; ++i) px[i] = 0;

    const float cx = N * 0.5f;
    const float cy = N * 0.5f;
    const float discR = N * 0.28f;
    const float rayInner = N * 0.36f;
    const float rayOuter = N * 0.48f;
    const float rayWidth = 1.6f;
    constexpr DWORD WHITE = 0xFFFFFFFFu;

    auto setPx = [&](int x, int y) {
        if (x < 0 || x >= N || y < 0 || y >= N) return;
        // DIB rows are bottom-up: flip y.
        px[(N - 1 - y) * N + x] = WHITE;
    };

    // Disc
    for (int y = 0; y < N; ++y) {
        for (int x = 0; x < N; ++x) {
            float dx = x + 0.5f - cx;
            float dy = y + 0.5f - cy;
            if (dx * dx + dy * dy <= discR * discR) setPx(x, y);
        }
    }

    // 8 rays
    for (int i = 0; i < 8; ++i) {
        float a = 3.14159265f * 2.0f * i / 8.0f;
        float ca = std::cos(a), sa = std::sin(a);
        for (float r = rayInner; r <= rayOuter; r += 0.5f) {
            float x = cx + r * ca;
            float y = cy + r * sa;
            // Stamp a small square so the ray has visible width.
            for (int oy = -1; oy <= 1; ++oy) {
                for (int ox = -1; ox <= 1; ++ox) {
                    float dx = (x + ox) - (cx + r * ca);
                    float dy = (y + oy) - (cy + r * sa);
                    if (dx * dx + dy * dy <= rayWidth * rayWidth) {
                        setPx(static_cast<int>(x + ox), static_cast<int>(y + oy));
                    }
                }
            }
        }
    }

    HBITMAP hbmMask = CreateBitmap(N, N, 1, 1, nullptr);
    ICONINFO ii = {};
    ii.fIcon = TRUE;
    ii.hbmColor = hbmColor;
    ii.hbmMask = hbmMask;
    HICON hIcon = CreateIconIndirect(&ii);
    DeleteObject(hbmColor);
    DeleteObject(hbmMask);
    return hIcon;
}

TrayIcon::TrayIcon() = default;

TrayIcon::~TrayIcon() {
    shutdown();
    if (m_ownedIcon) {
        DestroyIcon(m_ownedIcon);
        m_ownedIcon = nullptr;
    }
}

bool TrayIcon::initialize(HWND hwnd, HINSTANCE hInstance, UINT callbackMsg) {
    m_hwnd = hwnd;

    ZeroMemory(&m_nid, sizeof(m_nid));
    m_nid.cbSize = sizeof(NOTIFYICONDATAW);
    m_nid.hWnd = hwnd;
    m_nid.uID = 1;
    m_nid.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP;
    m_nid.uCallbackMessage = callbackMsg;

    // On dark Windows themes the bundled brightness.ico is hard to see against
    // a black taskbar, so substitute a procedurally drawn white sun icon.
    if (isDarkMode()) {
        m_ownedIcon = createWhiteSunIcon();
        m_nid.hIcon = m_ownedIcon;
    }
    if (!m_nid.hIcon) {
        m_nid.hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_APPICON));
    }
    if (!m_nid.hIcon) {
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
    UINT autostartFlags = MF_STRING |
        (autostart::isEnabled() ? MF_CHECKED : MF_UNCHECKED);
    AppendMenuW(hMenu, autostartFlags, 3, L"Start with Windows");
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
    case 3:
        autostart::setEnabled(!autostart::isEnabled());
        break;
    }
}

} // namespace tray
