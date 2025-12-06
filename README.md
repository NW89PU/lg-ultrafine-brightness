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
  - `Ctrl + Volume Up` - Increase brightness by 5%
  - `Ctrl + Volume Down` - Decrease brightness by 5%
- **Tray Icon**: Double-click to show/hide the window

### Window Management

- **Close Button (X)**: Hides to system tray (app keeps running)
- **Minimize Button**: Minimizes to taskbar
- **ESC Key**: Hides window to system tray
- **Tray Menu**: Right-click tray icon for Show/Exit options

## 🛠️ Building from Source

### Prerequisites

- Windows 10/11
- Visual Studio 2019 or later (with C++ desktop development)
- CMake 3.20+
- Git

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
cmake .. -G "Visual Studio 17 2022" -A x64

# Build Debug version
cmake --build . --config Debug

# Or build Release version
cmake --build . --config Release

# Output binaries
# Debug: build/Debug/LGUltrafineBrightness.exe
# Release: build/Release/LGUltrafineBrightness.exe
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
# build/LGUltrafineBrightness.exe
```

## 🏗️ Architecture

The application is built with a modular architecture:

- **HID Communication** - Direct USB HID control using hidapi
- **UI Rendering** - Modern dark theme with ImGui + DirectX 11
- **System Integration** - Tray icon, hotkeys, and single-instance management
- **Brightness Control** - Smart stepped adjustment with 0-100% range

## 🔌 Dependencies

- [hidapi](https://github.com/libusb/hidapi) - USB HID communication
- [Dear ImGui](https://github.com/ocornut/imgui) - Immediate mode GUI

## 🤝 Contributing

Contributions are welcome! Please feel free to submit a Pull Request.

## 📄 License

This project is licensed under the MIT License - see the LICENSE file for details.

## 🙏 Acknowledgments

- Thanks to the hidapi and Dear ImGui communities
- Inspired by macOS's native brightness control for LG Ultrafine displays

## ⚠️ Troubleshooting

**Monitor not detected:**
- Ensure your LG Ultrafine monitor is connected via USB-C/Thunderbolt
- Try unplugging and reconnecting the monitor
- Check Windows Device Manager for HID devices

**Hotkeys not working:**
- Make sure no other application is using the same key combination
- Try restarting the application with administrator privileges

## 📞 Support

If you encounter any issues, please [open an issue](../../issues) on GitHub.

---

Made with ❤️ for LG Ultrafine users on Windows
