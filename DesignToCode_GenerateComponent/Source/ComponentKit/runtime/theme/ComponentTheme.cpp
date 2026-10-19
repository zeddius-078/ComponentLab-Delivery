#include "ComponentTheme.h"
#include "FieldChrome.h"
#include "FocusIndicator.h"
#include "ProfileContext.h"
#include "TokenResources.h"
#include "Typography.h"
#include "Z_PushButton.h"
#include "Z_TableView.h"
#include <QApplication>
#include <QJsonArray>
#include <QJsonDocument>
#include <QtMath>
#include <stdexcept>

static void initializeComponentResources() { Q_INIT_RESOURCE(component_resources); }

namespace component_theme {
namespace {
QJsonObject readObject(const QString &path) {
    QJsonParseError error;
    const auto document = QJsonDocument::fromJson(token_resources::read(path), &error);
    // 损坏的 Token 产物必须在创建页面前暴露，避免静默使用空参数。
    if (error.error != QJsonParseError::NoError || !document.isObject())
        throw std::runtime_error(("Invalid Token object: " + path).toStdString());
    return document.object();
}

} // namespace

QString activeProfile() { return profile_context::activeProfile(); }

void install(QApplication &application, const QString &profile) {
    initializeComponentResources();
    if (!activeProfile().isEmpty())
        throw std::logic_error("Component theme is already installed");
    const auto runtimeDocument = readObject(":/tokens/tokens_qt.json");
    const auto selected = runtimeDocument["profiles"].toObject()[profile].toObject();
    if (selected.isEmpty())
        throw std::invalid_argument("Unknown component theme profile");

    const auto value = [&selected](const QString &path) -> QJsonValue {
        for (const auto &category : {"color", "layout", "number", "font", "boolean", "string", "asset"}) {
            const auto values = selected[category].toObject();
            if (values.contains(path))
                return values[path];
        }
        throw std::runtime_error(("Missing generated V2 Token: " + path).toStdString());
    };
    const auto number = [&value](const QString &path) {
        const auto result = value(path);
        if (!result.isDouble())
            throw std::runtime_error(("Non-numeric generated V2 Token: " + path).toStdString());
        return result.toDouble();
    };
    const auto tokenColor = [&value](const QString &path) {
        const QColor result(value(path).toString());
        if (!result.isValid())
            throw std::runtime_error(("Invalid generated V2 color: " + path).toStdString());
        return result;
    };

    commonWidget::button_detail::Runtime runtime;
    runtime.values = selected;
    commonWidget::table_detail::Runtime tableRuntime;
    tableRuntime.values = selected;
    const QStringList sizes{"small", "medium", "large"};
    for (int index = 0; index < sizes.size(); ++index) {
        const auto name = sizes[index];
        const auto prefix = "component.button.base.size." + name + ".slot.";
        const auto minimumHeight = qRound(number(prefix + "container.min-height"));
        const auto iconSize = qRound(number(prefix + "icon.size"));
        runtime.sizes[index] = {
            minimumHeight,
            qRound(number(prefix + "container.padding-inline")),
            qRound(number(prefix + "container.padding-block")),
            qRound(number("component.button.base.content.icon-left-text.slot.content.gap")),
            qRound(number("component.button.base.shared.slot.border.border-width")),
            iconSize,
            iconSize,
            qRound(number(prefix + "spinner.size")),
            number("component.button.base.shared.slot.spinner.stroke-width"),
            minimumHeight};
    }
    const auto motionDocument = readObject(":/composites/qt.json");
    const auto spinnerMotion = motionDocument["motion"].toObject()["loading.spinner"].toObject();
    if (spinnerMotion["status"] != "active")
        throw std::runtime_error("Released Spinner Motion is unavailable");
    runtime.cycleMs = spinnerMotion["timing"].toObject()["durationMs"].toInt();
    runtime.frameMs = 16; // Qt repaint cadence; visual timing remains Token-owned.
    runtime.spinnerStartDegrees = number("component.button.base.shared.slot.spinner.arc-start-angle");
    runtime.spinnerSweepDegrees = number("component.button.base.shared.slot.spinner.arc-sweep-angle");
    runtime.spinnerTrackOpacity = number("component.button.base.shared.slot.spinner.track-opacity");
    // 图标沿用组件库已有的透明蒙版绘制，状态颜色由组件传入；不引入演示图标枚举。
    runtime.renderIcon = [](const QIcon &icon, int canvas, const QColor &color, QIcon::Mode,
                            qreal dpr) {
        if (icon.isNull() || !color.isValid() || canvas <= 0 || dpr <= 0)
            throw std::invalid_argument("Invalid component icon request");
        return field::iconPixmap(icon, canvas, dpr, color);
    };
    // 全局文本使用 HarmonyOS Sans；缺失的简体中文字形显式回退到 SC。
    // 字体资产由 SDK 注册，不依赖操作系统是否预装字体。
    typography::installApplicationFonts();
    QFont font;
    font.setFamilies(typography::applicationFamilies());
    font.setPixelSize(qRound(number("component.button.base.typography.slot.label.font-size")));
    application.setFont(font);
    auto palette = application.palette();
    palette.setColor(QPalette::Window, tokenColor("semantic.color.background.page"));
    palette.setColor(QPalette::WindowText, tokenColor("semantic.color.text.primary"));
    palette.setColor(QPalette::Base, tokenColor("semantic.color.background.control-default"));
    palette.setColor(QPalette::AlternateBase, tokenColor("semantic.color.background.hover"));
    palette.setColor(QPalette::Text, tokenColor("semantic.color.text.primary"));
    palette.setColor(QPalette::Button, tokenColor("semantic.color.background.control-default"));
    palette.setColor(QPalette::ButtonText, tokenColor("semantic.color.text.primary"));
    palette.setColor(QPalette::Highlight, tokenColor("semantic.color.interactive.brand.default"));
    palette.setColor(QPalette::HighlightedText, tokenColor("semantic.color.text.invert"));
    palette.setColor(QPalette::Disabled, QPalette::Text, tokenColor("semantic.color.text.disabled"));
    palette.setColor(QPalette::Disabled, QPalette::ButtonText, tokenColor("semantic.color.text.disabled"));
    palette.setColor(QPalette::Disabled, QPalette::WindowText, tokenColor("semantic.color.text.disabled"));
    application.setPalette(palette);
    const auto nativeQss = QString::fromUtf8(
        token_resources::read(":/native/" + profile + "/native-components.qss"));
    if (nativeQss.trimmed().isEmpty())
        throw std::runtime_error("Missing generated native component QSS");
    application.setStyleSheet(application.styleSheet() + nativeQss);
    commonWidget::FocusIndicator::instance().configure(
        value("adapter.qt.focus-indicator.mode").toString(),
        number("adapter.qt.focus-indicator.opacity"));
    commonWidget::button_detail::installButtonRuntime(application, runtime);
    commonWidget::table_detail::installTableRuntime(application, tableRuntime);
    profile_context::install(application, profile);
}
} // namespace component_theme
