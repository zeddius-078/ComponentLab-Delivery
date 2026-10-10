#pragma once
#include <QIcon>
#include <QString>

namespace commonWidget {
// Shared by both icon-only buttons and the leading/trailing text-button slots.
class IconSource final {
public:
    static QString resolve(const QString& source);
    // Static SVG or transparent PNG. Empty clears; invalid input throws.
    static QIcon load(const QString& source);
};
}
