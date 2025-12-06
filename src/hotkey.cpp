#include "hotkey.h"

namespace hotkey {

HotkeyManager::HotkeyManager() = default;

HotkeyManager::~HotkeyManager() {
    shutdown();
}

bool HotkeyManager::initialize(HWND hwnd) {
    m_hwnd = hwnd;
    return true;
}

void HotkeyManager::shutdown() {
    unregisterAll();
}

bool HotkeyManager::registerDefaultHotkeys() {
    bool success = true;

    // Ctrl+Alt+Up - Brightness up
    if (RegisterHotKey(m_hwnd, static_cast<int>(HotkeyId::BrightnessUp),
                       MOD_CONTROL | MOD_ALT, VK_UP)) {
        m_registeredIds.push_back(static_cast<int>(HotkeyId::BrightnessUp));
    } else {
        success = false;
    }

    // Ctrl+Alt+Down - Brightness down
    if (RegisterHotKey(m_hwnd, static_cast<int>(HotkeyId::BrightnessDown),
                       MOD_CONTROL | MOD_ALT, VK_DOWN)) {
        m_registeredIds.push_back(static_cast<int>(HotkeyId::BrightnessDown));
    } else {
        success = false;
    }

    return success;
}

void HotkeyManager::unregisterAll() {
    for (int id : m_registeredIds) {
        UnregisterHotKey(m_hwnd, id);
    }
    m_registeredIds.clear();
}

void HotkeyManager::handleHotkey(WPARAM hotkeyId) {
    switch (static_cast<HotkeyId>(hotkeyId)) {
    case HotkeyId::BrightnessUp:
        if (m_brightnessUp) m_brightnessUp();
        break;
    case HotkeyId::BrightnessDown:
        if (m_brightnessDown) m_brightnessDown();
        break;
    case HotkeyId::BrightnessUpSmall:
        if (m_brightnessUpSmall) m_brightnessUpSmall();
        break;
    case HotkeyId::BrightnessDownSmall:
        if (m_brightnessDownSmall) m_brightnessDownSmall();
        break;
    }
}

} // namespace hotkey
