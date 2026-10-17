#include "ProfileContext.h"

#include <QCoreApplication>
#include <QObject>
#include <QVariant>
#include <stdexcept>

namespace profile_context {
namespace {
constexpr auto propertyName = "designSystemProfile";
}

QString activeProfile() {
    const auto* application = QCoreApplication::instance();
    return application ? application->property(propertyName).toString() : QString{};
}

QString resolve(const QObject* object, const QString& fallback) {
    for (auto* current = object; current; current = current->parent()) {
        const auto value = current->property(propertyName).toString();
        if (!value.isEmpty())
            return value;
    }
    const auto installed = activeProfile();
    return installed.isEmpty() ? fallback : installed;
}

void install(QCoreApplication& application, const QString& profile) {
    if (profile.isEmpty())
        throw std::invalid_argument("Design-system profile must not be empty");
    const auto existing = application.property(propertyName).toString();
    if (!existing.isEmpty())
        throw std::logic_error("Design-system profile is already installed");
    application.setProperty(propertyName, profile);
}

} // namespace profile_context
