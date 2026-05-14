# LG Ultrafine Brightness Control

A sleek Windows application for controlling LG Ultrafine 4K/5K monitor brightness with a beautiful modern UI and intelligent auto-brightness.

## 🔱 Changes in this fork

This fork extends [mengzhisy/lg-ultrafine-brightness](https://github.com/mengzhisy/lg-ultrafine-brightness):

- **Editable brightness curve** — the fixed lux→% algorithm is replaced by 5 user-editable `(lux, brightness)` control points with piecewise-linear interpolation. A live preview plot shows the curve with a "now" marker for your current lux/brightness.
- **Side settings panel** — a `Settings >` button toggles a right-side panel; the window resizes horizontally so nothing scrolls.
- **Persistent configuration** — curve points and hysteresis are saved to `%APPDATA%\LGUltrafineBrightness\config.ini`.
- **Tunable layout & theme** — `layout.ini` next to the executable controls vertical offsets, window height, accent color, text colors, and the curve line color. Auto-created on first run as a commented template.
- **Buildable out of the box** — upstream's submodule pins referenced commits that no longer exist in `libusb/hidapi` / `ocornut/imgui`; this fork pins both to stable releases (`hidapi-0.15.0`, `imgui v1.92.8`). Also adds the missing `src/resource.h` that wasn't committed upstream.

## ✨ Features

- 🌟 **Auto-Brightness** - Automatically adjusts screen brightness based on ambient light sensor (ALS) built into your LG Ultrafine display
- 🎨 **Beautiful Modern UI** - Dark-themed interface built with ImGui and DirectX 11
- 🖥️ **Native Brightness Control** - Direct HID communication with LG Ultrafine monitors
- ⌨️ **Global Hotkeys** - Adjust brightness from anywhere using Ctrl+Alt+Up/Down
- 🎯 **System Tray Integration** - Minimal footprint with quick access from tray icon

## 📸 Screenshots

<img src="img.png" width="50%" alt="LG Ultrafine Brightness Control Interface">

## 🔧 Supported Monitors

- LG Ultrafine 5K Display (27MD5KL-B, 27MD5KA-B)
- LG Ultrafine 4K Display (24MD4KL-B, 22MD4KA-B)

## 🎮 Usage

### Auto-Brightness

If your LG Ultrafine monitor has a built-in ambient light sensor (ALS), the app will automatically detect it and enable auto-brightness features:

1. **View Real-time Ambient Light** - The UI displays the current ambient light level in lux
2. **Enable Auto-Brightness** - Check the "Auto Brightness" checkbox to let the app automatically adjust brightness based on room lighting
3. **Editable Curve** - Open the side panel (`Settings >`) to edit the 5 `(lux, brightness %)` control points. Points are interpolated piecewise-linearly. Below the first point and above the last, brightness is clamped to the endpoint values. A live plot shows the resulting curve with a marker for your current ambient lux.
4. **Hysteresis** - Minimum |target − current| in % before a change is actually applied, to avoid jitter from a noisy sensor.

> **Note**: If no ALS is detected, the UI will show "Not Supported" and you can still manually control brightness.

### Manual Brightness Control

- **Slider**: Drag the slider in the main window
- **Hotkeys**:
  - `Ctrl + Alt + Up` - Increase brightness by 5%
  - `Ctrl + Alt + Down` - Decrease brightness by 5%

> **Tip**: When auto-brightness is enabled, the manual slider is disabled. Uncheck "Auto Brightness" to regain manual __control.

## 🛠️ Building from Source

### Prerequisites

- Windows 10/11
- Visual Studio 2019 or later (with C++ desktop development)
- CMake 3.20+
- Git

### Build Steps

```bash
# Clone the repository
git clone --recurse-submodules https://github.com/NW89PU/lg-ultrafine-brightness.git
cd lg-ultrafine-brightness

# Initialize submodules
git submodule update --init --recursive

# Configure (from Visual Studio Developer Command Prompt)
mkdir build
cd build

cmake ..

cmake --build . --config Release

```