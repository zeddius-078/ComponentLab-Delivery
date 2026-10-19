#pragma once

#include <QJsonArray>
#include <QJsonValue>
#include <QList>
#include <QString>
#include <QStringList>
#include <memory>

class QWidget;
namespace surface { class Catalog; }
namespace field { class Tokens; }

namespace commonWidget {

struct NativeComponentDescriptor {
    QString qtClass;
    QString sdkClass;
    QString designComponent;
    QString mappingKind;
};

// Single SDK-owned construction and property-application boundary. Consumers
// request registered classes; they do not duplicate constructors or setter maps.
class ComponentFactory {
  public:
    explicit ComponentFactory(const QString& profile = "standard-light");
    QWidget* create(const QString& sdkClass, QWidget* parent = nullptr) const;
    void applyCalls(QWidget* widget, const QJsonArray& calls) const;
    bool apply(QWidget* widget, const QString& api, const QJsonValue& value,
               QString* error = nullptr) const;

    static QList<NativeComponentDescriptor> descriptors();
    static QStringList registeredClasses();
    static bool isRegistered(const QString& sdkClass);

  private:
    QString profile;
    std::shared_ptr<const surface::Catalog> catalog;
    std::shared_ptr<const field::Tokens> fields;
};

} // namespace commonWidget
