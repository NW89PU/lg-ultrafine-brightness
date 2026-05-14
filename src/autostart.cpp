#include "autostart.h"

#include <Windows.h>
#include <string>

namespace autostart {

static constexpr wchar_t kRunKey[]   = L"Software\\Microsoft\\Windows\\CurrentVersion\\Run";
static constexpr wchar_t kValueName[] = L"LGUltrafineBrightness";

static std::wstring quotedExePath() {
    wchar_t buf[MAX_PATH];
    DWORD n = GetModuleFileNameW(nullptr, buf, MAX_PATH);
    if (n == 0 || n == MAX_PATH) return L"";
    std::wstring p;
    p.reserve(n + 2);
    p.push_back(L'"');
    p.append(buf);
    p.push_back(L'"');
    return p;
}

bool isEnabled() {
    HKEY hKey;
    if (RegOpenKeyExW(HKEY_CURRENT_USER, kRunKey, 0, KEY_READ, &hKey) != ERROR_SUCCESS) {
        return false;
    }
    LONG res = RegQueryValueExW(hKey, kValueName, nullptr, nullptr, nullptr, nullptr);
    RegCloseKey(hKey);
    return res == ERROR_SUCCESS;
}

void setEnabled(bool enabled) {
    HKEY hKey;
    if (RegOpenKeyExW(HKEY_CURRENT_USER, kRunKey, 0,
                      KEY_SET_VALUE | KEY_READ, &hKey) != ERROR_SUCCESS) {
        return;
    }
    if (enabled) {
        auto path = quotedExePath();
        if (!path.empty()) {
            RegSetValueExW(hKey, kValueName, 0, REG_SZ,
                           reinterpret_cast<const BYTE*>(path.c_str()),
                           static_cast<DWORD>((path.size() + 1) * sizeof(wchar_t)));
        }
    } else {
        RegDeleteValueW(hKey, kValueName);
    }
    RegCloseKey(hKey);
}

} // namespace autostart
