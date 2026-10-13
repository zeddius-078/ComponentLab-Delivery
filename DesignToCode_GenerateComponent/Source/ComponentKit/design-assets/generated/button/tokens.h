// GENERATED. Edit tokens/button-contract.json or refresh the locked source.
#pragma once
#include "Z_PushButton.h"
#include <stdexcept>
namespace button_generated {
inline constexpr const char* sourceHash = "79059ec4189ce0c4194a0cd6aa3160963870be5d42e024543b1c83fbf8bf395e";
inline constexpr const char* fontFamily = "HarmonyOS Sans SC";
inline constexpr int bodyFontSize = 14;
inline constexpr double demoGlyphOccupancy = 0.75;
inline commonWidget::button_detail::Runtime metrics(const QString& profile) {
    commonWidget::button_detail::Runtime r;
    if (profile == "light-comfortable") {
        r.sizes = {{
            commonWidget::button_detail::Metrics{32,8,0,4,1,20,24,20,1.5,32},
            commonWidget::button_detail::Metrics{32,8,0,4,1,20,24,20,1.5,32},
            commonWidget::button_detail::Metrics{32,12,0,4,1,24,28,24,1.5,32},
            commonWidget::button_detail::Metrics{40,16,0,4,1,32,36,32,2,40}
        }};
        r.cycleMs = 800; r.frameMs = 16; r.sweepDegrees = 270;
        return r;
    }
    if (profile == "light-compact") {
        r.sizes = {{
            commonWidget::button_detail::Metrics{32,8,0,4,1,20,24,20,1.5,32},
            commonWidget::button_detail::Metrics{32,8,0,4,1,20,24,20,1.5,32},
            commonWidget::button_detail::Metrics{32,12,0,4,1,24,28,24,1.5,32},
            commonWidget::button_detail::Metrics{40,16,0,4,1,32,36,32,2,40}
        }};
        r.cycleMs = 800; r.frameMs = 16; r.sweepDegrees = 270;
        return r;
    }
    if (profile == "dark-comfortable") {
        r.sizes = {{
            commonWidget::button_detail::Metrics{32,8,0,4,1,20,24,20,1.5,32},
            commonWidget::button_detail::Metrics{32,8,0,4,1,20,24,20,1.5,32},
            commonWidget::button_detail::Metrics{32,12,0,4,1,24,28,24,1.5,32},
            commonWidget::button_detail::Metrics{40,16,0,4,1,32,36,32,2,40}
        }};
        r.cycleMs = 800; r.frameMs = 16; r.sweepDegrees = 270;
        return r;
    }
    if (profile == "dark-compact") {
        r.sizes = {{
            commonWidget::button_detail::Metrics{32,8,0,4,1,20,24,20,1.5,32},
            commonWidget::button_detail::Metrics{32,8,0,4,1,20,24,20,1.5,32},
            commonWidget::button_detail::Metrics{32,12,0,4,1,24,28,24,1.5,32},
            commonWidget::button_detail::Metrics{40,16,0,4,1,32,36,32,2,40}
        }};
        r.cycleMs = 800; r.frameMs = 16; r.sweepDegrees = 270;
        return r;
    }
    throw std::invalid_argument("Unknown Button Profile");
}
}
