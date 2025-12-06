#pragma once

#include <Windows.h>
#include <functional>
#include <vector>

namespace hotkey {

// Hotkey identifiers
enum class HotkeyId {
    BrightnessUp = 1,
    BrightnessDown = 2,
    BrightnessUpSmall = 3,
    BrightnessDownSmall = 4
};

class HotkeyManager {
public:
    HotkeyManager();
    ~HotkeyManager();

    // Initialize with window handle
    bool initialize(HWND hwnd);
    void shutdown();

    // Register/unregister hotkeys
    bool registerDefaultHotkeys();
    void unregisterAll();

    // Handle hotkey message
    void handleHotkey(WPARAM hotkeyId);

    // Callbacks
    using HotkeyCallback = std::function<void()>;
    void setBrightnessUpCallback(HotkeyCallback callback) { m_brightnessUp = callback; }
    void setBrightnessDownCallback(HotkeyCallback callback) { m_brightnessDown = callback; }
    void setBrightnessUpSmallCallback(HotkeyCallback callback) { m_brightnessUpSmall = callback; }
    void setBrightnessDownSmallCallback(HotkeyCallback callback) { m_brightnessDownSmall = callback; }

private:
    HWND m_hwnd = nullptr;
    std::vector<int> m_registeredIds;

    HotkeyCallback m_brightnessUp;
    HotkeyCallback m_brightnessDown;
    HotkeyCallback m_brightnessUpSmall;
    HotkeyCallback m_brightnessDownSmall;
};

} // namespace hotkey
