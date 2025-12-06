# LG Ultrafine Brightness Control

A sleek Windows application for controlling LG Ultrafine 4K/5K monitor brightness with a beautiful modern UI.

## ✨ Features

- 🎨 **Beautiful Modern UI** - Dark-themed interface built with ImGui and DirectX 11
- 🖥️ **Native Brightness Control** - Direct HID communication with LG Ultrafine monitors
- ⌨️ **Global Hotkeys** - Adjust brightness from anywhere using Ctrl+Volume Up/Down
- 🎯 **System Tray Integration** - Minimal footprint with quick access from tray icon
- 🚀 **Single Instance** - Smart window management prevents multiple instances
- 📊 **Real-time Display** - Live brightness percentage with smooth slider control

## 📸 Screenshots

> *Note: Add screenshots of your application here*

## 🔧 Supported Monitors

- LG Ultrafine 5K Display (27MD5KL-B, 27MD5KA-B)
- LG Ultrafine 4K Display (24MD4KL-B, 22MD4KA-B)

## 📥 Installation

1. Download the latest release from [Releases](../../releases)
2. Extract the ZIP file
3. Run `LGUltrafineBrightness.exe`
4. The app will start in the system tray

## 🎮 Usage

### Adjusting Brightness

- **Slider**: Drag the slider in the main window
- **Hotkeys**:
  - `Ctrl + Alt + Up` - Increase brightness.
  - `Ctrl + Alt + Down` - Decrease brightness.

## 🛠️ Building from Source

### Prerequisites

- Windows 10/11
- Visual Studio 2019 or later (with C++ desktop development)
- CMake 3.20+
- Git
- Ninja

### Build Steps

```bash
# Clone the repository
git clone https://github.com/yourusername/lg-ultrafine-brightness.git
cd lg-ultrafine-brightness

# Initialize submodules
git submodule update --init --recursive

# Configure (from Visual Studio Developer Command Prompt)
mkdir build
cd build

cmake ..

cmake --build . --config Release

```

**Alternative with Ninja:**

```bash
# From Visual Studio Developer Command Prompt
mkdir build
cd build
cmake .. -G Ninja -DCMAKE_BUILD_TYPE=Release

# Build
ninja

# Output binary
# build/Release/LGUltrafineBrightness.exe
```
