#pragma once

#include <array>
#include <string>

namespace settings {

// Auto-brightness response is defined by N (lux, brightness) control points
// interpolated piecewise-linearly. Lux below the first point clamps to the
// first point's brightness; lux above the last point clamps to the last.
constexpr int CURVE_POINTS = 5;

struct CurvePoint {
    float lux;
    int brightness;  // 0-100
};

struct AutoBrightnessSettings {
    std::array<CurvePoint, CURVE_POINTS> curve = {{
        {0.0f, 10},
        {50.0f, 30},
        {200.0f, 60},
        {800.0f, 90},
        {2000.0f, 100},
    }};
    int hysteresis = 5;     // minimum |current - target| in % before an adjustment is applied
    bool enabled = false;   // last-known Auto Brightness checkbox state
};

// Load settings from %APPDATA%\LGUltrafineBrightness\config.ini.
// Missing or malformed values fall back to defaults.
AutoBrightnessSettings load();

// Save settings to %APPDATA%\LGUltrafineBrightness\config.ini.
// Creates the directory if needed. Returns false on I/O failure.
bool save(const AutoBrightnessSettings& s);

// Vertical layout offsets for the right-column settings panel.
// Values are in device-independent pixels (96 DPI baseline) and scaled by
// the runtime DPI factor. Edit layout.ini next to the .exe and restart.
struct LayoutSettings {
    float windowHeightDp = 520.0f;            // overall client-area height
    float topMarginDp = 20.0f;                // gap from window top edge to first content
    float rightHeaderOffsetDp = 1.0f;         // gap between header and the range label
    float rightRangeToPlotDp = 25.0f;         // gap between the range label and the plot
    float rightPlotToPointsDp = 22.0f;        // gap between plot and P1
    float rightPointsToHysteresisDp = 20.0f;  // gap between P5 and the Hysteresis slider

    // Theme colors as 6-digit hex (with or without leading #). Strings so they
    // can be edited from layout.ini without depending on imgui types.
    std::string sliderHex     = "33b1ff";     // slider grab / accent (buttons hover, checkmark, brightness %)
    std::string textGreenHex  = "08bdba";     // "Connected" + ALS device name
    std::string textYellowHex = "08bdba";     // ALS lux readout
    std::string curveHex      = "08bdba";     // brightness curve line on the plot
};

// Load layout.ini from next to the executable. If the file does not exist,
// a default template (with comments) is written and defaults are returned.
LayoutSettings loadLayout();

} // namespace settings
