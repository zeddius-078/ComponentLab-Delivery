#include "DemoRuntime.h"
#include "ComponentTheme.h"
#include <QApplication>
#include <QStyle>
#include <QStyleFactory>
#include <stdexcept>

namespace button_demo {
QIcon icon(const QString& assetId) {
    const QMap<QString, QStyle::StandardPixmap> ids{{"arrow-left", QStyle::SP_ArrowLeft},
                                                    {"arrow-right", QStyle::SP_ArrowRight},
                                                    {"start", QStyle::SP_MediaPlay},
                                                    {"pause", QStyle::SP_MediaPause},
                                                    {"abort", QStyle::SP_DialogCancelButton}};
    if (!ids.contains(assetId))
        throw std::invalid_argument(("Unknown demo assetId: " + assetId).toStdString());
    const std::unique_ptr<QStyle> style(QStyleFactory::create("Fusion"));
    return style->standardIcon(ids[assetId]);
}

void install(QApplication& app, const QString& profile) {
    component_theme::install(app, profile);
}

QString validate(const commonWidget::Z_PushButton& button) {
    bool iconSlot = false;
    for (const auto* name : {"LEADING", "TRAILING"}) {
        const auto* host = button.findChild<QWidget*>(name, Qt::FindDirectChildrenOnly);
        iconSlot = iconSlot || (host && !host->isHidden());
    }
    if (!button.property("loading").toBool() && button.text().isEmpty() && !iconSlot)
        return QString::fromUtf8("配置错误：Label、Leading、Trailing 不能同时为空");
    if ((button.isIconOnly() || button.text().isEmpty()) &&
        button.accessibleName().trimmed().isEmpty())
        return QString::fromUtf8("配置错误：纯图标按钮需要 accessibleName");
    return {};
}
} // namespace button_demo
