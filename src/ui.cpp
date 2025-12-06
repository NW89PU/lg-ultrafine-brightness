#include "ui.h"

#include <imgui.h>
#include <imgui_impl_win32.h>
#include <imgui_impl_dx11.h>
#include <dwmapi.h>
#include <cmath>

#pragma comment(lib, "dwmapi.lib")

namespace ui {

// Get DPI scale factor for a window
static float GetDpiScale(HWND hwnd) {
    // Try GetDpiForWindow (Windows 10 1607+)
    HMODULE user32 = GetModuleHandleW(L"user32.dll");
    if (user32) {
        typedef UINT (WINAPI *GetDpiForWindowFunc)(HWND);
        auto getDpiForWindow = (GetDpiForWindowFunc)GetProcAddress(user32, "GetDpiForWindow");
        if (getDpiForWindow) {
            UINT dpi = getDpiForWindow(hwnd);
            return dpi / 96.0f;
        }
    }

    // Fallback: use DC
    HDC hdc = GetDC(hwnd);
    float scale = GetDeviceCaps(hdc, LOGPIXELSX) / 96.0f;
    ReleaseDC(hwnd, hdc);
    return scale;
}

UIRenderer::UIRenderer() = default;

UIRenderer::~UIRenderer() {
    shutdown();
}

bool UIRenderer::initialize(HWND hwnd) {
    m_hwnd = hwnd;
    m_dpiScale = GetDpiScale(hwnd);

    if (!createDeviceD3D(hwnd)) {
        return false;
    }

    // Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    io.IniFilename = nullptr; // Disable ini file

    // Setup Platform/Renderer backends
    ImGui_ImplWin32_Init(hwnd);
    ImGui_ImplDX11_Init(m_device, m_context);

    // Load font with DPI-scaled size
    float fontSize = 18.0f * m_dpiScale;
    io.Fonts->AddFontFromFileTTF("C:\\Windows\\Fonts\\segoeui.ttf", fontSize);

    applyDarkTheme();

    return true;
}

void UIRenderer::shutdown() {
    ImGui_ImplDX11_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();
    cleanupDeviceD3D();
}

bool UIRenderer::createDeviceD3D(HWND hwnd) {
    DXGI_SWAP_CHAIN_DESC sd = {};
    sd.BufferCount = 2;
    sd.BufferDesc.Width = 0;
    sd.BufferDesc.Height = 0;
    sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    sd.BufferDesc.RefreshRate.Numerator = 60;
    sd.BufferDesc.RefreshRate.Denominator = 1;
    sd.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;
    sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    sd.OutputWindow = hwnd;
    sd.SampleDesc.Count = 1;
    sd.SampleDesc.Quality = 0;
    sd.Windowed = TRUE;
    sd.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;

    UINT createDeviceFlags = 0;
    D3D_FEATURE_LEVEL featureLevel;
    const D3D_FEATURE_LEVEL featureLevelArray[2] = {
        D3D_FEATURE_LEVEL_11_0,
        D3D_FEATURE_LEVEL_10_0,
    };

    HRESULT res = D3D11CreateDeviceAndSwapChain(
        nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, createDeviceFlags,
        featureLevelArray, 2, D3D11_SDK_VERSION, &sd,
        &m_swapChain, &m_device, &featureLevel, &m_context);

    if (res != S_OK) {
        return false;
    }

    createRenderTarget();
    return true;
}

void UIRenderer::cleanupDeviceD3D() {
    cleanupRenderTarget();
    if (m_swapChain) { m_swapChain->Release(); m_swapChain = nullptr; }
    if (m_context) { m_context->Release(); m_context = nullptr; }
    if (m_device) { m_device->Release(); m_device = nullptr; }
}

void UIRenderer::createRenderTarget() {
    ID3D11Texture2D* backBuffer;
    m_swapChain->GetBuffer(0, IID_PPV_ARGS(&backBuffer));
    m_device->CreateRenderTargetView(backBuffer, nullptr, &m_renderTargetView);
    backBuffer->Release();
}

void UIRenderer::cleanupRenderTarget() {
    if (m_renderTargetView) {
        m_renderTargetView->Release();
        m_renderTargetView = nullptr;
    }
}

void UIRenderer::beginFrame() {
    ImGui_ImplDX11_NewFrame();
    ImGui_ImplWin32_NewFrame();
    ImGui::NewFrame();
}

void UIRenderer::endFrame() {
    ImGui::Render();

    const float clear_color[4] = { 0.1f, 0.1f, 0.12f, 1.0f };
    m_context->OMSetRenderTargets(1, &m_renderTargetView, nullptr);
    m_context->ClearRenderTargetView(m_renderTargetView, clear_color);

    ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());

    m_swapChain->Present(1, 0); // VSync
}

void UIRenderer::applyDarkTheme() {
    ImGuiStyle& style = ImGui::GetStyle();
    float s = m_dpiScale; // Scale factor

    // Rounding (scaled)
    style.WindowRounding = 10.0f * s;
    style.FrameRounding = 6.0f * s;
    style.GrabRounding = 6.0f * s;
    style.PopupRounding = 6.0f * s;
    style.ScrollbarRounding = 6.0f * s;

    // Spacing (scaled)
    style.WindowPadding = ImVec2(20 * s, 20 * s);
    style.FramePadding = ImVec2(10 * s, 8 * s);
    style.ItemSpacing = ImVec2(10 * s, 10 * s);
    style.ItemInnerSpacing = ImVec2(8 * s, 6 * s);

    // Sizes (scaled)
    style.ScrollbarSize = 14.0f * s;
    style.GrabMinSize = 12.0f * s;

    // Borders
    style.WindowBorderSize = 0.0f;
    style.FrameBorderSize = 0.0f;

    // Colors - Modern dark theme with accent color
    ImVec4* colors = style.Colors;

    // Background colors
    colors[ImGuiCol_WindowBg] = ImVec4(0.12f, 0.12f, 0.14f, 1.0f);
    colors[ImGuiCol_ChildBg] = ImVec4(0.14f, 0.14f, 0.16f, 1.0f);
    colors[ImGuiCol_PopupBg] = ImVec4(0.14f, 0.14f, 0.16f, 0.95f);

    // Border colors
    colors[ImGuiCol_Border] = ImVec4(0.25f, 0.25f, 0.28f, 1.0f);
    colors[ImGuiCol_BorderShadow] = ImVec4(0.0f, 0.0f, 0.0f, 0.0f);

    // Frame colors
    colors[ImGuiCol_FrameBg] = ImVec4(0.18f, 0.18f, 0.20f, 1.0f);
    colors[ImGuiCol_FrameBgHovered] = ImVec4(0.22f, 0.22f, 0.25f, 1.0f);
    colors[ImGuiCol_FrameBgActive] = ImVec4(0.25f, 0.25f, 0.28f, 1.0f);

    // Title bar
    colors[ImGuiCol_TitleBg] = ImVec4(0.10f, 0.10f, 0.12f, 1.0f);
    colors[ImGuiCol_TitleBgActive] = ImVec4(0.12f, 0.12f, 0.14f, 1.0f);
    colors[ImGuiCol_TitleBgCollapsed] = ImVec4(0.10f, 0.10f, 0.12f, 1.0f);

    // Slider - Orange accent
    colors[ImGuiCol_SliderGrab] = ImVec4(0.95f, 0.55f, 0.15f, 1.0f);
    colors[ImGuiCol_SliderGrabActive] = ImVec4(1.0f, 0.65f, 0.25f, 1.0f);

    // Button
    colors[ImGuiCol_Button] = ImVec4(0.20f, 0.20f, 0.23f, 1.0f);
    colors[ImGuiCol_ButtonHovered] = ImVec4(0.95f, 0.55f, 0.15f, 0.8f);
    colors[ImGuiCol_ButtonActive] = ImVec4(0.95f, 0.55f, 0.15f, 1.0f);

    // Header
    colors[ImGuiCol_Header] = ImVec4(0.20f, 0.20f, 0.23f, 1.0f);
    colors[ImGuiCol_HeaderHovered] = ImVec4(0.95f, 0.55f, 0.15f, 0.6f);
    colors[ImGuiCol_HeaderActive] = ImVec4(0.95f, 0.55f, 0.15f, 0.8f);

    // Text
    colors[ImGuiCol_Text] = ImVec4(0.95f, 0.95f, 0.95f, 1.0f);
    colors[ImGuiCol_TextDisabled] = ImVec4(0.50f, 0.50f, 0.50f, 1.0f);

    // Separator
    colors[ImGuiCol_Separator] = ImVec4(0.25f, 0.25f, 0.28f, 1.0f);
    colors[ImGuiCol_SeparatorHovered] = ImVec4(0.95f, 0.55f, 0.15f, 0.8f);
    colors[ImGuiCol_SeparatorActive] = ImVec4(0.95f, 0.55f, 0.15f, 1.0f);

    // Scrollbar
    colors[ImGuiCol_ScrollbarBg] = ImVec4(0.12f, 0.12f, 0.14f, 1.0f);
    colors[ImGuiCol_ScrollbarGrab] = ImVec4(0.25f, 0.25f, 0.28f, 1.0f);
    colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.30f, 0.30f, 0.33f, 1.0f);
    colors[ImGuiCol_ScrollbarGrabActive] = ImVec4(0.35f, 0.35f, 0.38f, 1.0f);

    // Check mark
    colors[ImGuiCol_CheckMark] = ImVec4(0.95f, 0.55f, 0.15f, 1.0f);
}

void UIRenderer::setMonitorName(const std::wstring& name) {
    int size = WideCharToMultiByte(CP_UTF8, 0, name.c_str(), -1, nullptr, 0, nullptr, nullptr);
    m_monitorName.resize(size);
    WideCharToMultiByte(CP_UTF8, 0, name.c_str(), -1, m_monitorName.data(), size, nullptr, nullptr);
}

void UIRenderer::renderMainUI() {
    ImGuiIO& io = ImGui::GetIO();
    float s = m_dpiScale;

    // Full window ImGui frame
    ImGui::SetNextWindowPos(ImVec2(0, 0));
    ImGui::SetNextWindowSize(io.DisplaySize);

    ImGuiWindowFlags window_flags =
        ImGuiWindowFlags_NoTitleBar |
        ImGuiWindowFlags_NoResize |
        ImGuiWindowFlags_NoMove |
        ImGuiWindowFlags_NoCollapse |
        ImGuiWindowFlags_NoScrollbar |
        ImGuiWindowFlags_NoBringToFrontOnFocus;

    ImGui::Begin("##Main", nullptr, window_flags);

    // Center content
    float windowWidth = ImGui::GetWindowWidth();
    float contentWidth = windowWidth - 40.0f * s;

    // Title
    {
        const char* title = "LG Ultrafine Brightness";
        float textWidth = ImGui::CalcTextSize(title).x;
        ImGui::SetCursorPosX((windowWidth - textWidth) * 0.5f);
        ImGui::TextColored(ImVec4(0.95f, 0.95f, 0.95f, 1.0f), "%s", title);
    }

    // Connection status
    {
        const char* status = m_connected ? "Connected" : "Disconnected";
        ImVec4 statusColor = m_connected ?
            ImVec4(0.3f, 0.85f, 0.4f, 1.0f) :
            ImVec4(0.85f, 0.3f, 0.3f, 1.0f);

        float textWidth = ImGui::CalcTextSize(status).x;
        ImGui::SetCursorPosX((windowWidth - textWidth) * 0.5f);
        ImGui::TextColored(statusColor, "%s", status);
    }

    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();

    // Brightness control section
    if (m_connected) {
        // Sun icons and slider
        ImGui::SetCursorPosX(20.0f * s);

        // Left sun icon (dim)
        ImGui::TextColored(ImVec4(0.5f, 0.5f, 0.5f, 1.0f), " ");
        ImGui::SameLine();

        // Brightness slider
        ImGui::SetNextItemWidth(contentWidth - 60.0f * s);
        int brightness = m_currentBrightness;
        if (ImGui::SliderInt("##brightness", &brightness, 0, 100, "")) {
            m_currentBrightness = brightness;
            if (m_brightnessCallback) {
                m_brightnessCallback(brightness);
            }
        }

        ImGui::SameLine();
        // Right sun icon (bright)
        ImGui::TextColored(ImVec4(0.95f, 0.75f, 0.2f, 1.0f), " ");

        ImGui::Spacing();

        // Percentage display
        {
            char buf[32];
            snprintf(buf, sizeof(buf), "%d%%", m_currentBrightness);
            float textWidth = ImGui::CalcTextSize(buf).x;
            ImGui::SetCursorPosX((windowWidth - textWidth) * 0.5f);
            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.95f, 0.55f, 0.15f, 1.0f));
            ImGui::Text("%s", buf);
            ImGui::PopStyleColor();
        }
    } else {
        // Not connected message
        const char* msg = "Monitor not detected";
        float textWidth = ImGui::CalcTextSize(msg).x;
        ImGui::SetCursorPosX((windowWidth - textWidth) * 0.5f);
        ImGui::TextColored(ImVec4(0.6f, 0.6f, 0.6f, 1.0f), "%s", msg);

        ImGui::Spacing();

        const char* hint = "Please connect your LG Ultrafine monitor";
        textWidth = ImGui::CalcTextSize(hint).x;
        ImGui::SetCursorPosX((windowWidth - textWidth) * 0.5f);
        ImGui::TextColored(ImVec4(0.5f, 0.5f, 0.5f, 1.0f), "%s", hint);
    }

    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();

    // Hotkey info
    {
        const char* info = "Hotkeys: Ctrl+Volume Up/Down";
        float textWidth = ImGui::CalcTextSize(info).x;
        ImGui::SetCursorPosX((windowWidth - textWidth) * 0.5f);
        ImGui::TextColored(ImVec4(0.5f, 0.5f, 0.5f, 1.0f), "%s", info);
    }

    ImGui::End();
}

} // namespace ui
