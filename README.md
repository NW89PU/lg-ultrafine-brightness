# LG Ultrafine Brightness Control

Windows app for controlling LG Ultrafine 4K/5K monitor brightness, with auto-brightness via the monitor's built-in ambient light sensor.

## Changes in this fork

Extends [mengzhisy/lg-ultrafine-brightness](https://github.com/mengzhisy/lg-ultrafine-brightness):

- The auto-brightness response is defined by 5 editable (lux, brightness) points with piecewise-linear interpolation, instead of the original hardcoded 5-step mapping. A plot under the editor shows the curve with a marker for the current ambient lux.
- A Settings button opens a side panel with the curve editor. The window grows horizontally to fit it instead of scrolling.
- Curve points and hysteresis are saved to %APPDATA%\LGUltrafineBrightness\config.ini.
- layout.ini next to the executable controls vertical offsets, window height, and theme colors (accent, two text colors, curve line). It is created on first run.
- The original repo's submodule pins point at commits that no longer exist in libusb/hidapi and ocornut/imgui. This fork pins hidapi-0.15.0 and imgui v1.92.8.
- The original missed committing src/resource.h. Added.

## Features

- Auto-brightness via the monitor's built-in ambient light sensor.
- HID-based brightness control (no DDC/CI).
- Global hotkeys: Ctrl+Alt+Up / Ctrl+Alt+Down.
- System tray integration.
- ImGui + DirectX 11 UI.

## Screenshot

<img src="img.png" width="50%" alt="LG Ultrafine Brightness Control Interface">

## Supported monitors

- LG Ultrafine 5K Display (27MD5KL-B, 27MD5KA-B)
- LG Ultrafine 4K Display (24MD4KL-B, 22MD4KA-B)

## Usage

### Auto-brightness

If the monitor has a built-in ambient light sensor, the app detects it and exposes auto-brightness:

1. The UI shows the current ambient light in lux.
2. Check "Auto Brightness" to let the app drive brightness from the sensor reading.
3. Click "Settings >" to open the curve editor. Edit the 5 (lux, %) points; values are interpolated piecewise-linearly. Lux below the first point clamps to the first point's brightness, lux above the last clamps to the last.
4. Hysteresis is the minimum |target - current| in percent before an update is applied. Increase it if the sensor is jittery, decrease it for a more responsive curve.

If no sensor is detected, the UI shows "Not Supported" and manual control still works.

### Manual brightness

- Drag the slider in the main window.
- Hotkeys:
  - Ctrl+Alt+Up: increase brightness by 5%.
  - Ctrl+Alt+Down: decrease brightness by 5%.

When Auto Brightness is on, the manual slider is disabled. Uncheck it to regain manual control.

## Building from source

### Prerequisites

- Windows 10/11
- Visual Studio 2019 or later (with C++ desktop development)
- CMake 3.20+
- Git

### Build steps

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