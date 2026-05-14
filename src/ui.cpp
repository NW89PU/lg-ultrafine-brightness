#include "ui.h"

#include <imgui.h>
#include <imgui_impl_win32.h>
#include <imgui_impl_dx11.h>
#include <dwmapi.h>
#include <algorithm>
#include <cmath>
#include <string>

#undef min
#undef max

// Parse a 6-digit hex color ("RRGGBB" with optional leading '#') into an
// ImVec4 with alpha=1. Returns fallback on malformed input.
static ImVec4 parseHexColor(const std::string& hex, ImVec4 fallback) {
    std::string s = hex;
    if (!s.empty() && s[0] == '#') s.erase(0, 1);
    if (s.size() != 6) return fallback;
    try {
        unsigned long v = std::stoul(s, nullptr, 16);
        return ImVec4(((v >> 16) & 0xFF) / 255.0f,
                      ((v >>  8) & 0xFF) / 255.0f,
                      ( v        & 0xFF) / 255.0f,
                      1.0f);
    } catch (...) {
        return fallback;
    }
}

// Slightly brighten a color (used for *Active variants of *Hovered slots).
static ImVec4 brighten(const ImVec4& c, float k = 0.10f) {
    return ImVec4(std::min(1.0f, c.x + k),
                  std::min(1.0f, c.y + k),
                  std::min(1.0f, c.z + k),
                  c.w);
}

// Same color with overridden alpha.
static ImVec4 withAlpha(const ImVec4& c, float a) {
    return ImVec4(c.x, c.y, c.z, a);
}

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

void UIRenderer::resizeSwapChain(UINT width, UINT height) {
    if (!m_swapChain || width == 0 || height == 0) return;
    cleanupRenderTarget();
    m_swapChain->ResizeBuffers(0, width, height, DXGI_FORMAT_UNKNOWN, 0);
    createRenderTarget();
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

    // Borders / rounding for children
    style.WindowBorderSize = 0.0f;
    style.FrameBorderSize = 0.0f;
    style.ChildBorderSize = 0.0f;
    style.ChildRounding = 0.0f;
    style.PopupBorderSize = 0.0f;

    // Colors - Modern dark theme with accent color
    ImVec4* colors = style.Colors;

    // Background colors — keep WindowBg and ChildBg identical so the seam
    // between the parent window and the child columns is invisible.
    colors[ImGuiCol_WindowBg] = ImVec4(0.14f, 0.14f, 0.16f, 1.0f);
    colors[ImGuiCol_ChildBg]  = ImVec4(0.14f, 0.14f, 0.16f, 1.0f);
    colors[ImGuiCol_PopupBg]  = ImVec4(0.14f, 0.14f, 0.16f, 0.95f);

    // Border colors
    colors[ImGuiCol_Border] = ImVec4(0.0f, 0.0f, 0.0f, 0.0f);
    colors[ImGuiCol_BorderShadow] = ImVec4(0.0f, 0.0f, 0.0f, 0.0f);

    // Frame colors
    colors[ImGuiCol_FrameBg] = ImVec4(0.18f, 0.18f, 0.20f, 1.0f);
    colors[ImGuiCol_FrameBgHovered] = ImVec4(0.22f, 0.22f, 0.25f, 1.0f);
    colors[ImGuiCol_FrameBgActive] = ImVec4(0.25f, 0.25f, 0.28f, 1.0f);

    // Title bar
    colors[ImGuiCol_TitleBg] = ImVec4(0.10f, 0.10f, 0.12f, 1.0f);
    colors[ImGuiCol_TitleBgActive] = ImVec4(0.12f, 0.12f, 0.14f, 1.0f);
    colors[ImGuiCol_TitleBgCollapsed] = ImVec4(0.10f, 0.10f, 0.12f, 1.0f);

    // Accent color (slider grab, button hover/active, header hover/active,
    // separator hover/active, check mark, plot line). Read from layout.ini.
    ImVec4 accent = parseHexColor(m_layout.sliderHex, ImVec4(0.95f, 0.55f, 0.15f, 1.0f));

    // Slider
    colors[ImGuiCol_SliderGrab] = accent;
    colors[ImGuiCol_SliderGrabActive] = brighten(accent);

    // Button
    colors[ImGuiCol_Button] = ImVec4(0.20f, 0.20f, 0.23f, 1.0f);
    colors[ImGuiCol_ButtonHovered] = withAlpha(accent, 0.8f);
    colors[ImGuiCol_ButtonActive] = accent;

    // Header
    colors[ImGuiCol_Header] = ImVec4(0.20f, 0.20f, 0.23f, 1.0f);
    colors[ImGuiCol_HeaderHovered] = withAlpha(accent, 0.6f);
    colors[ImGuiCol_HeaderActive] = withAlpha(accent, 0.8f);

    // Text
    colors[ImGuiCol_Text] = ImVec4(0.95f, 0.95f, 0.95f, 1.0f);
    colors[ImGuiCol_TextDisabled] = ImVec4(0.50f, 0.50f, 0.50f, 1.0f);

    // Separator
    colors[ImGuiCol_Separator] = ImVec4(0.25f, 0.25f, 0.28f, 1.0f);
    colors[ImGuiCol_SeparatorHovered] = withAlpha(accent, 0.8f);
    colors[ImGuiCol_SeparatorActive] = accent;

    // Scrollbar
    colors[ImGuiCol_ScrollbarBg] = ImVec4(0.12f, 0.12f, 0.14f, 1.0f);
    colors[ImGuiCol_ScrollbarGrab] = ImVec4(0.25f, 0.25f, 0.28f, 1.0f);
    colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.30f, 0.30f, 0.33f, 1.0f);
    colors[ImGuiCol_ScrollbarGrabActive] = ImVec4(0.35f, 0.35f, 0.38f, 1.0f);

    // Check mark
    colors[ImGuiCol_CheckMark] = accent;

    // Brightness curve on the plot
    colors[ImGuiCol_PlotLines] = parseHexColor(m_layout.curveHex,
                                               ImVec4(0.95f, 0.75f, 0.2f, 1.0f));
    colors[ImGuiCol_PlotLinesHovered] = brighten(colors[ImGuiCol_PlotLines]);
}

void UIRenderer::setMonitorName(const std::wstring& name) {
    int size = WideCharToMultiByte(CP_UTF8, 0, name.c_str(), -1, nullptr, 0, nullptr, nullptr);
    m_monitorName.resize(size);
    WideCharToMultiByte(CP_UTF8, 0, name.c_str(), -1, m_monitorName.data(), size, nullptr, nullptr);
}

void UIRenderer::setALSName(const std::wstring& name) {
    int size = WideCharToMultiByte(CP_UTF8, 0, name.c_str(), -1, nullptr, 0, nullptr, nullptr);
    m_alsName.resize(size);
    WideCharToMultiByte(CP_UTF8, 0, name.c_str(), -1, m_alsName.data(), size, nullptr, nullptr);
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

    // Zero out parent padding — each column has its own padding.
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
    ImGui::Begin("##Main", nullptr, window_flags);
    ImGui::PopStyleVar();

    // Top margin from window edge — tunable via layout.ini. Applied once before
    // both child columns so they remain horizontally aligned.
    if (m_layout.topMarginDp > 0.0f) {
        ImGui::SetCursorPosY(ImGui::GetCursorPosY() + m_layout.topMarginDp * s);
    }

    // Capture screen-Y positions of each left-column separator so we can
    // extend the lines across the right column once it's been drawn.
    float sepYs[8];
    int sepCount = 0;

    ImGui::BeginChild("##LeftCol", ImVec2(MAIN_COL_DP * s, 0), 0,
                      ImGuiWindowFlags_NoScrollbar);

    // Center content within the left column
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
            parseHexColor(m_layout.textGreenHex, ImVec4(0.3f, 0.85f, 0.4f, 1.0f)) :
            ImVec4(0.85f, 0.3f, 0.3f, 1.0f);

        float textWidth = ImGui::CalcTextSize(status).x;
        ImGui::SetCursorPosX((windowWidth - textWidth) * 0.5f);
        ImGui::TextColored(statusColor, "%s", status);
    }

    ImGui::Spacing();
    if (sepCount < 8) sepYs[sepCount++] = ImGui::GetCursorScreenPos().y;
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
        if (m_autoBrightnessEnabled) {
            ImGui::BeginDisabled();
        }
        if (ImGui::SliderInt("##brightness", &brightness, 0, 100, "")) {
            if (!m_autoBrightnessEnabled) {
                m_currentBrightness = brightness;
                if (m_brightnessCallback) {
                    m_brightnessCallback(brightness);
                }
            }
        }
        if (m_autoBrightnessEnabled) {
            ImGui::EndDisabled();
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
            ImGui::PushStyleColor(ImGuiCol_Text, parseHexColor(m_layout.sliderHex, ImVec4(0.95f, 0.55f, 0.15f, 1.0f)));
            ImGui::Text("%s", buf);
            ImGui::PopStyleColor();
        }

        ImGui::Spacing();
        ImGui::Spacing();

        // Auto-brightness section
        if (sepCount < 8) sepYs[sepCount++] = ImGui::GetCursorScreenPos().y;
        ImGui::Separator();
        ImGui::Spacing();

        if (m_hasALS) {
            // Show ALS sensor info
            {
                const char* alsTitle = "Ambient Light Sensor";
                float textWidth = ImGui::CalcTextSize(alsTitle).x;
                ImGui::SetCursorPosX((windowWidth - textWidth) * 0.5f);
                ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "%s", alsTitle);
            }

            ImGui::Spacing();

            // ALS device name
            if (!m_alsName.empty()) {
                float textWidth = ImGui::CalcTextSize(m_alsName.c_str()).x;
                ImGui::SetCursorPosX((windowWidth - textWidth) * 0.5f);
                ImGui::TextColored(parseHexColor(m_layout.textGreenHex, ImVec4(0.3f, 0.85f, 0.4f, 1.0f)),
                                   "%s", m_alsName.c_str());
            }

            ImGui::Spacing();

            // Ambient light value (always show, even if 0)
            {
                char luxBuf[64];
                snprintf(luxBuf, sizeof(luxBuf), "%.1f lux", m_ambientLight);
                float textWidth = ImGui::CalcTextSize(luxBuf).x;
                ImGui::SetCursorPosX((windowWidth - textWidth) * 0.5f);
                ImGui::PushStyleColor(ImGuiCol_Text, parseHexColor(m_layout.textYellowHex, ImVec4(0.95f, 0.75f, 0.2f, 1.0f)));
                ImGui::Text("%s", luxBuf);
                ImGui::PopStyleColor();
            }

            ImGui::Spacing();

            // Auto-brightness checkbox
            {
                bool autoEnabled = m_autoBrightnessEnabled;
                float checkboxWidth = ImGui::CalcTextSize("Auto Brightness").x + 30.0f * s;
                ImGui::SetCursorPosX((windowWidth - checkboxWidth) * 0.5f);
                if (ImGui::Checkbox("Auto Brightness", &autoEnabled)) {
                    m_autoBrightnessEnabled = autoEnabled;
                    if (m_autoBrightnessCallback) {
                        m_autoBrightnessCallback(autoEnabled);
                    }
                }
            }

            ImGui::Spacing();

            // Settings toggle — opens a side panel to the right with the auto-brightness sliders
            {
                const char* label = m_settingsExpanded ? "Hide Settings <" : "Settings >";
                float btnWidth = ImGui::CalcTextSize(label).x + 24.0f * s;
                ImGui::SetCursorPosX((windowWidth - btnWidth) * 0.5f);
                if (ImGui::Button(label, ImVec2(btnWidth, 0))) {
                    m_settingsExpanded = !m_settingsExpanded;
                    if (m_resizeCallback) {
                        m_resizeCallback(getClientWidthPx(), getClientHeightPx());
                    }
                }
            }
        } else {
            // ALS not supported
            const char* alsTitle = "Ambient Light Sensor";
            float textWidth = ImGui::CalcTextSize(alsTitle).x;
            ImGui::SetCursorPosX((windowWidth - textWidth) * 0.5f);
            ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "%s", alsTitle);

            ImGui::Spacing();

            const char* notSupported = "Not Supported";
            textWidth = ImGui::CalcTextSize(notSupported).x;
            ImGui::SetCursorPosX((windowWidth - textWidth) * 0.5f);
            ImGui::TextColored(ImVec4(0.85f, 0.3f, 0.3f, 1.0f), "%s", notSupported);
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
    if (sepCount < 8) sepYs[sepCount++] = ImGui::GetCursorScreenPos().y;
    ImGui::Separator();
    ImGui::Spacing();

    // Hotkey info
    {
        const char* info = "Hotkeys: Ctrl+Alt Up/Down";
        float textWidth = ImGui::CalcTextSize(info).x;
        ImGui::SetCursorPosX((windowWidth - textWidth) * 0.5f);
        ImGui::TextColored(ImVec4(0.5f, 0.5f, 0.5f, 1.0f), "%s", info);
    }

    ImGui::EndChild();  // ##LeftCol

    // Right column: brightness curve editor + hysteresis
    if (m_settingsExpanded) {
        ImGui::SameLine(0.0f, 0.0f);
        ImGui::BeginChild("##RightCol", ImVec2(SETTINGS_COL_DP * s, 0), 0,
                          ImGuiWindowFlags_NoScrollbar);

        float colWidth = ImGui::GetWindowWidth();

        // Header
        {
            const char* title = "Brightness Curve";
            float textWidth = ImGui::CalcTextSize(title).x;
            ImGui::SetCursorPosX((colWidth - textWidth) * 0.5f);
            ImGui::TextColored(ImVec4(0.95f, 0.95f, 0.95f, 1.0f), "%s", title);
        }

        // Vertical offset before the range label — tunable via layout.ini.
        // SetCursorPosY (rather than Dummy) avoids ImGui's extra ItemSpacing
        // before/after a dummy item, so offset=0 means truly tight spacing.
        ImGui::SetCursorPosY(ImGui::GetCursorPosY() + m_layout.rightHeaderOffsetDp * s);

        // Build a sorted copy of the curve for plotting / interpolation preview.
        auto sortedCurve = m_autoSettings.curve;
        std::sort(sortedCurve.begin(), sortedCurve.end(),
                  [](const settings::CurvePoint& a, const settings::CurvePoint& b) {
                      return a.lux < b.lux;
                  });

        // Sample brightness across a fixed lux domain for the plot.
        constexpr int SAMPLES = 64;
        const float plotMaxLux = std::max(2500.0f, sortedCurve.back().lux * 1.1f);
        float samples[SAMPLES];
        for (int i = 0; i < SAMPLES; ++i) {
            float lux = plotMaxLux * (static_cast<float>(i) / (SAMPLES - 1));
            int b;
            if (lux <= sortedCurve.front().lux) {
                b = sortedCurve.front().brightness;
            } else if (lux >= sortedCurve.back().lux) {
                b = sortedCurve.back().brightness;
            } else {
                b = sortedCurve.back().brightness;
                for (size_t k = 0; k + 1 < sortedCurve.size(); ++k) {
                    if (lux >= sortedCurve[k].lux && lux <= sortedCurve[k + 1].lux) {
                        float span = sortedCurve[k + 1].lux - sortedCurve[k].lux;
                        float t = (span > 0.0f) ? (lux - sortedCurve[k].lux) / span : 0.0f;
                        b = static_cast<int>(sortedCurve[k].brightness
                              + t * (sortedCurve[k + 1].brightness - sortedCurve[k].brightness));
                        break;
                    }
                }
            }
            samples[i] = static_cast<float>(b);
        }

        // X-axis hint (above plot, since PlotLines has no axis labels)
        const ImGuiStyle& style = ImGui::GetStyle();
        float plotWidth = colWidth - 2.0f * style.WindowPadding.x;
        {
            char rangeLabel[64];
            snprintf(rangeLabel, sizeof(rangeLabel), "0 lux ............... %.0f lux", plotMaxLux);
            float textWidth = ImGui::CalcTextSize(rangeLabel).x;
            ImGui::SetCursorPosX((colWidth - textWidth) * 0.5f);
            ImGui::TextColored(ImVec4(0.5f, 0.5f, 0.5f, 1.0f), "%s", rangeLabel);
        }

        // Gap between the range label and the plot — tunable via layout.ini.
        ImGui::SetCursorPosY(ImGui::GetCursorPosY() + m_layout.rightRangeToPlotDp * s);

        // Plot — render without the default top-left overlay text, then draw
        // it manually centered over the plot area.
        ImVec2 plotScreenPos = ImGui::GetCursorScreenPos();
        ImVec2 plotSize(plotWidth, 90.0f * s);
        ImGui::PlotLines("##curve", samples, SAMPLES, 0, nullptr,
                         0.0f, 100.0f, plotSize);

        char overlay[64];
        snprintf(overlay, sizeof(overlay), "now: %.0f lux -> %d%%",
                 m_ambientLight, m_currentBrightness);
        ImVec2 textSize = ImGui::CalcTextSize(overlay);
        ImVec2 textPos(plotScreenPos.x + (plotSize.x - textSize.x) * 0.5f,
                       plotScreenPos.y + (plotSize.y - textSize.y) * 0.5f);
        ImGui::GetWindowDrawList()->AddText(textPos,
                                            ImGui::GetColorU32(ImGuiCol_PlotLines),
                                            overlay);

        // Gap between plot and first curve point — tunable via layout.ini.
        ImGui::SetCursorPosY(ImGui::GetCursorPosY() + m_layout.rightPlotToPointsDp * s);

        // Curve points: 5 rows of (lux, brightness)
        bool changed = false;
        const float pointLabelW = ImGui::CalcTextSize("P5").x + 8.0f * s;
        const float luxLabelW = ImGui::CalcTextSize("lux").x + style.ItemInnerSpacing.x;
        const float pctLabelW = ImGui::CalcTextSize("%").x + style.ItemInnerSpacing.x;
        const float remaining = plotWidth - pointLabelW - luxLabelW - pctLabelW
                                - 3.0f * style.ItemSpacing.x;
        const float luxFieldW = remaining * 0.55f;
        const float briFieldW = remaining * 0.45f;

        for (int i = 0; i < settings::CURVE_POINTS; ++i) {
            ImGui::PushID(i);
            // Align the P-label baseline with the input-field vertical center.
            ImGui::AlignTextToFramePadding();
            ImGui::Text("P%d", i + 1);
            ImGui::SameLine();
            ImGui::SetNextItemWidth(luxFieldW);
            changed |= ImGui::DragFloat("lux", &m_autoSettings.curve[i].lux,
                                        1.0f, 0.0f, 10000.0f, "%.0f");
            ImGui::SameLine();
            ImGui::SetNextItemWidth(briFieldW);
            changed |= ImGui::DragInt("%", &m_autoSettings.curve[i].brightness,
                                      0.5f, 0, 100);
            ImGui::PopID();
        }

        // Gap between curve points and Hysteresis — tunable via layout.ini.
        ImGui::SetCursorPosY(ImGui::GetCursorPosY() + m_layout.rightPointsToHysteresisDp * s);

        // Hysteresis on its own row
        ImGui::SetNextItemWidth(plotWidth - ImGui::CalcTextSize("Hysteresis %").x
                                - style.ItemInnerSpacing.x);
        changed |= ImGui::SliderInt("Hysteresis %", &m_autoSettings.hysteresis, 0, 30);

        if (changed && m_autoSettingsCallback) {
            m_autoSettingsCallback(m_autoSettings);
        }

        ImGui::EndChild();

        // Extend the left column's horizontal separators across the right
        // column using the foreground draw list (so they render on top of the
        // right child's background). Y positions captured earlier inside the
        // left column align with the separators' actual draw position.
        ImDrawList* fg = ImGui::GetForegroundDrawList();
        ImU32 col = ImGui::GetColorU32(ImGuiCol_Separator);
        ImVec2 parentPos = ImGui::GetWindowPos();
        float xStart = parentPos.x + (MAIN_COL_DP - 20.0f) * s;
        float xEnd   = parentPos.x + (MAIN_COL_DP + SETTINGS_COL_DP) * s;
        for (int i = 0; i < sepCount; ++i) {
            fg->AddLine(ImVec2(xStart, sepYs[i]), ImVec2(xEnd, sepYs[i]), col, 1.0f);
        }
    }

    ImGui::End();
}

int UIRenderer::getClientWidthPx() const {
    int dp = m_settingsExpanded ? (MAIN_COL_DP + SETTINGS_COL_DP) : MAIN_COL_DP;
    return static_cast<int>(dp * m_dpiScale);
}

int UIRenderer::getClientHeightPx() const {
    return static_cast<int>(m_layout.windowHeightDp * m_dpiScale);
}

} // namespace ui
