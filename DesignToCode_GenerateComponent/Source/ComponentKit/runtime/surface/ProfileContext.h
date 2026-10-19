#pragma once

#include <QString>

class QCoreApplication;
class QObject;

namespace profile_context {

// Process profile selected by the theme installer. This module is below both
// Theme and Components so visual adapters never need to depend back on Theme.
QString activeProfile();

// Resolves the nearest explicit object/ancestor override, then the process
// profile. The fallback only applies before a theme has been installed.
QString resolve(const QObject* object,
                const QString& fallback = QStringLiteral("standard-light"));

// Called by ComponentTheme only after the requested profile has been validated
// and installed successfully.
void install(QCoreApplication& application, const QString& profile);

} // namespace profile_context
