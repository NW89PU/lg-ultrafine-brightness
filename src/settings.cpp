#include "settings.h"

#define NOMINMAX
#include <Windows.h>
#include <ShlObj.h>
#include <algorithm>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <string>

#pragma comment(lib, "shell32.lib")

namespace settings {

static std::filesystem::path configPath() {
    PWSTR roaming = nullptr;
    if (FAILED(SHGetKnownFolderPath(FOLDERID_RoamingAppData, 0, nullptr, &roaming))) {
        return {};
    }
    std::filesystem::path p(roaming);
    CoTaskMemFree(roaming);
    p /= L"LGUltrafineBrightness";
    p /= L"config.ini";
    return p;
}

static void parseLine(const std::string& line, AutoBrightnessSettings& s) {
    auto eq = line.find('=');
    if (eq == std::string::npos) return;
    std::string key = line.substr(0, eq);
    std::string val = line.substr(eq + 1);

    auto trim = [](std::string& str) {
        size_t a = str.find_first_not_of(" \t\r\n");
        size_t b = str.find_last_not_of(" \t\r\n");
        str = (a == std::string::npos) ? "" : str.substr(a, b - a + 1);
    };
    trim(key);
    trim(val);

    auto matchIndexed = [&](const char* prefix, int& idxOut) -> bool {
        std::string p = prefix;
        if (key.rfind(p, 0) != 0) return false;
        try {
            idxOut = std::stoi(key.substr(p.size()));
            return idxOut >= 0 && idxOut < CURVE_POINTS;
        } catch (...) { return false; }
    };

    try {
        if (key == "hysteresis") {
            s.hysteresis = std::clamp(std::stoi(val), 0, 50);
            return;
        }
        if (key == "enabled") {
            s.enabled = (val == "true" || val == "1" || val == "yes");
            return;
        }
        int idx = -1;
        if (matchIndexed("curve_lux_", idx)) {
            s.curve[idx].lux = std::max(0.0f, std::stof(val));
        } else if (matchIndexed("curve_brightness_", idx)) {
            s.curve[idx].brightness = std::clamp(std::stoi(val), 0, 100);
        }
    } catch (...) {
        // Ignore malformed values; keep default.
    }
}

AutoBrightnessSettings load() {
    AutoBrightnessSettings s;
    auto path = configPath();
    if (path.empty()) return s;

    std::ifstream in(path);
    if (!in) return s;

    std::string line;
    while (std::getline(in, line)) {
        if (line.empty() || line[0] == '#' || line[0] == ';') continue;
        parseLine(line, s);
    }
    return s;
}

bool save(const AutoBrightnessSettings& s) {
    auto path = configPath();
    if (path.empty()) return false;

    std::error_code ec;
    std::filesystem::create_directories(path.parent_path(), ec);
    if (ec) return false;

    std::ofstream out(path, std::ios::trunc);
    if (!out) return false;

    for (int i = 0; i < CURVE_POINTS; ++i) {
        out << "curve_lux_" << i << "=" << s.curve[i].lux << "\n";
        out << "curve_brightness_" << i << "=" << s.curve[i].brightness << "\n";
    }
    out << "hysteresis=" << s.hysteresis << "\n";
    out << "enabled=" << (s.enabled ? "true" : "false") << "\n";
    return out.good();
}

static std::filesystem::path layoutPath() {
    wchar_t buf[MAX_PATH];
    DWORD n = GetModuleFileNameW(nullptr, buf, MAX_PATH);
    if (n == 0 || n == MAX_PATH) return {};
    return std::filesystem::path(buf).parent_path() / L"layout.ini";
}

static void writeLayoutTemplate(const std::filesystem::path& path, const LayoutSettings& s) {
    std::ofstream out(path, std::ios::trunc);
    if (!out) return;
    out << "# Layout settings for the app window.\n";
    out << "# Values are in dp (96-DPI baseline); the app scales by current DPI.\n";
    out << "# Restart the app after editing.\n";
    out << "\n";
    out << "# Overall window client-area height.\n";
    out << "window_height=" << s.windowHeightDp << "\n";
    out << "\n";
    out << "# Gap from the top edge of the window to the first content (both columns).\n";
    out << "top_margin=" << s.topMarginDp << "\n";
    out << "\n";
    out << "# Space between \"Brightness Curve\" header and the range label.\n";
    out << "right_header_offset=" << s.rightHeaderOffsetDp << "\n";
    out << "\n";
    out << "# Space between the range label (\"0 lux ... X lux\") and the plot.\n";
    out << "right_range_to_plot=" << s.rightRangeToPlotDp << "\n";
    out << "\n";
    out << "# Space between the plot and the first curve point row (P1).\n";
    out << "right_plot_to_points=" << s.rightPlotToPointsDp << "\n";
    out << "\n";
    out << "# Space between the last curve point row (P5) and the Hysteresis slider.\n";
    out << "right_points_to_hysteresis=" << s.rightPointsToHysteresisDp << "\n";
    out << "\n";
    out << "# Theme colors. Values are 6-digit hex (leading # optional).\n";
    out << "\n";
    out << "# Slider grab / accent color (also: buttons hover, checkmark, brightness %).\n";
    out << "slider_color=" << s.sliderHex << "\n";
    out << "\n";
    out << "# \"Connected\" status + ALS device name.\n";
    out << "text_green=" << s.textGreenHex << "\n";
    out << "\n";
    out << "# ALS lux readout text.\n";
    out << "text_yellow=" << s.textYellowHex << "\n";
    out << "\n";
    out << "# Brightness curve line color on the plot.\n";
    out << "curve_color=" << s.curveHex << "\n";
}

LayoutSettings loadLayout() {
    LayoutSettings s;
    auto path = layoutPath();
    if (path.empty()) return s;

    std::ifstream in(path);
    if (!in) {
        writeLayoutTemplate(path, s);
        return s;
    }

    auto trim = [](std::string& str) {
        size_t a = str.find_first_not_of(" \t\r\n");
        size_t b = str.find_last_not_of(" \t\r\n");
        str = (a == std::string::npos) ? "" : str.substr(a, b - a + 1);
    };

    std::string line;
    while (std::getline(in, line)) {
        if (line.empty() || line[0] == '#' || line[0] == ';') continue;
        auto eq = line.find('=');
        if (eq == std::string::npos) continue;
        std::string key = line.substr(0, eq);
        std::string val = line.substr(eq + 1);
        trim(key);
        trim(val);
        try {
            if (key == "window_height")            s.windowHeightDp = std::max(100.0f, std::stof(val));
            else if (key == "top_margin")          s.topMarginDp = std::max(0.0f, std::stof(val));
            else if (key == "right_header_offset") s.rightHeaderOffsetDp = std::stof(val);
            else if (key == "right_range_to_plot") s.rightRangeToPlotDp = std::stof(val);
            else if (key == "right_plot_to_points") s.rightPlotToPointsDp = std::stof(val);
            else if (key == "right_points_to_hysteresis") s.rightPointsToHysteresisDp = std::stof(val);
            else if (key == "slider_color")        s.sliderHex = val;
            else if (key == "text_green")          s.textGreenHex = val;
            else if (key == "text_yellow")         s.textYellowHex = val;
            else if (key == "curve_color")         s.curveHex = val;
        } catch (...) {
            // Ignore malformed values; keep default.
        }
    }
    return s;
}

} // namespace settings
