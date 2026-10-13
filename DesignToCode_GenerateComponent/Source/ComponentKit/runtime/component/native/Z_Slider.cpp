#include "Z_Slider.h"
#include "FocusIndicator.h"
#include <QVariant>

namespace commonWidget {

Z_Slider::Z_Slider(QWidget* parent) : QSlider(parent) {
    setProperty("designSystemNative", true);
    setProperty("designSystemClass", "commonWidget::Z_Slider");
    setProperty("nativeQtClass", "QSlider");
    setProperty("designSystemStyleOwner", "tokens");
    FocusIndicator::instance().watch(this);
}

} // namespace commonWidget
