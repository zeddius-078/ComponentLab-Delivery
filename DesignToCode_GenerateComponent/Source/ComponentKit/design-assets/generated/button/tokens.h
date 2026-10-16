// GENERATED. Edit tokens/button-contract.json or refresh the locked source.
#pragma once
#include "Z_PushButton.h"
#include <stdexcept>
namespace button_generated {
inline constexpr const char* sourceHash = "e5f54101838db093d897c8735533e7d79b9caafd72ab24098070124ef4995e8f";
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
