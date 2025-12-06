# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

Windows application for controlling LG Ultrafine monitor brightness via HID. Uses ImGui with DirectX 11 for the UI, hidapi for USB HID communication, and includes system tray integration and global hotkey support.

## Build Commands

Build requires Windows with MSVC or Clang, CMake 3.20+, and Ninja.

```bash
# Initialize submodules first
git submodule update --init --recursive

# Configure (from Visual Studio Developer Command Prompt or with vcvars64.bat sourced)
cmake --preset x64-debug    # Debug build with MSVC
cmake --preset x64-release  # Release build with MSVC

# Build
cmake --build --preset x64-debug
cmake --build --preset x64-release

# Output binary location
build/x64-debug/LGUltrafineBrightness.exe
build/x64-release/LGUltrafineBrightness.exe
```

## Architecture

The application is organized into these components:

- **app** (`app.cpp/h`) - Main application class orchestrating all components. Creates window, runs message loop, coordinates between brightness control and UI.

- **brightness** (`brightness.cpp/h`) - HID communication with LG Ultrafine monitors using hidapi. Handles raw brightness values (0x0190-0xd2f0), percent conversion, and stepped adjustments.

- **ui** (`ui.cpp/h`) - DirectX 11 rendering with ImGui. Manages D3D11 device, swap chain, and renders the brightness slider UI.

- **tray** (`tray.cpp/h`) - Windows system tray icon with context menu. Allows minimize-to-tray behavior.

- **hotkey** (`hotkey.cpp/h`) - Global hotkey registration for brightness up/down controls (both large and small steps).

## Dependencies

External libraries are Git submodules in `external/`:
- **hidapi** - USB HID communication library
- **imgui** - Immediate mode GUI (built with Win32 + DX11 backends)

System libraries: d3d11, dxgi, dwmapi

## Key Constants

LG Ultrafine brightness range: `0x0190` (min) to `0xd2f0` (max), vendor ID `0x043e`.
