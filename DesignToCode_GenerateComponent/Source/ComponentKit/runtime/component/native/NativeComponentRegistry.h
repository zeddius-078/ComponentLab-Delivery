#pragma once

#include "../ComponentFactory.h"

class QWidget;

namespace commonWidget {

// Compiled registry generated from native-components.json. This is product
// source: callers can inspect and debug it like any other C++ implementation.
QList<NativeComponentDescriptor> nativeComponentDescriptors();
QWidget* createGeneratedNativeComponent(const QString& sdkClass,
                                        QWidget* parent = nullptr);

} // namespace commonWidget
