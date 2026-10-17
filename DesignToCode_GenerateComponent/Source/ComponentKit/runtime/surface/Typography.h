#pragma once

#include <QString>
#include <QStringList>

namespace typography {

// Global UI text uses the pan-language family. Simplified-Chinese glyphs use
// the SC family as the explicit fallback; no unrelated font family is allowed
// into the SDK typography chain.
const QString& globalFamily();
const QString& simplifiedChineseFamily();
QStringList applicationFamilies();

// Registers SDK-owned font assets and validates their internal family names.
// Safe to call more than once.
void installApplicationFonts();

} // namespace typography
