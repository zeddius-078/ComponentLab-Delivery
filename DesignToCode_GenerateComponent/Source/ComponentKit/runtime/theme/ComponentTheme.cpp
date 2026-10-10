#include "ComponentTheme.h"
#include "FieldChrome.h"
#include "FocusIndicator.h"
#include "TokenResources.h"
#include "Z_PushButton.h"
#include <QApplication>
#include <QFontDatabase>
#include <QJsonArray>
#include <QJsonDocument>
#include <QtMath>
#include <stdexcept>

static void initializeComponentResources() { Q_INIT_RESOURCE(component_resources); }

namespace component_theme {
namespace {
QString installedProfile;

QJsonObject readObject(const QString &path) {
    QJsonParseError error;
    const auto document = QJsonDocument::fromJson(token_resources::read(path), &error);
    // 损坏的 Token 产物必须在创建页面前暴露，避免静默使用空参数。
    if (error.error != QJsonParseError::NoError || !document.isObject())
        throw std::runtime_error(("Invalid Token object: " + path).toStdString());
    return document.object();
}

} // namespace

QString activeProfile() { return installedProfile; }

void install(QApplication &application, const QString &profile) {
    initializeComponentResources();
    // 现有 Button 运行配置为进程级单次安装；沿用此约束，不在本轮改写主题生命周期。
    if (!installedProfile.isEmpty())
        throw std::logic_error("Component theme is already installed");
    const auto manifest = readObject(":/button/manifest.json");
    QJsonObject selected;
    // Profile 名称来自既有产物，未知名称直接失败，避免主题与组件参数混配。
    for (const auto &entry : manifest["profiles"].toArray()) {
        if (entry.toObject()["profile"] == profile)
            selected = entry.toObject();
    }
    if (selected.isEmpty())
        throw std::invalid_argument("Unknown component theme profile");

    commonWidget::button_detail::Runtime runtime;
    const auto sizes = selected["sizes"].toArray();
    if (sizes.size() != int(runtime.sizes.size()))
        throw std::runtime_error("Incomplete Button size catalog");
    // 保持既有 XS/S/M/L 顺序和字段绑定，组件内部尺寸计算继续使用同一组 Token。
    for (int index = 0; index < sizes.size(); ++index) {
        const auto size = sizes[index].toObject();
        runtime.sizes[index] = {size["minimumHeight"].toInt(),      size["paddingInline"].toInt(),
                                size["paddingBlock"].toInt(),       size["gap"].toInt(),
                                size["borderWidth"].toInt(),        size["iconCanvas"].toInt(),
                                size["iconContainer"].toInt(),      size["indicatorSize"].toInt(),
                                size["indicatorStroke"].toDouble(), size["iconOnlySize"].toInt()};
    }
    const auto motion = manifest["loading"].toObject();
    runtime.cycleMs = motion["cycleMs"].toInt();
    runtime.frameMs = motion["frameMs"].toInt();
    runtime.sweepDegrees = motion["sweepDegrees"].toInt();
    runtime.generatedQss = QString::fromUtf8(token_resources::read(":/button/" + profile + ".qss"));
    // 图标沿用组件库已有的透明蒙版绘制，状态颜色由组件传入；不引入演示图标枚举。
    runtime.renderIcon = [](const QIcon &icon, int canvas, const QColor &color, QIcon::Mode,
                            qreal dpr) {
        if (icon.isNull() || !color.isValid() || canvas <= 0 || dpr <= 0)
            throw std::invalid_argument("Invalid component icon request");
        return field::iconPixmap(icon, canvas, dpr, color);
    };
    const auto colors = selected["palette"].toObject();
    const auto color = [&colors](const char *key) {
        const auto value = colors[key].toString();
        if (value.isEmpty())
            throw std::runtime_error("Missing component palette entry");
        return surface::color(value);
    };
    runtime.focusColor = color("focus");
    runtime.dangerFocusColor = color("danger");
    runtime.ghostFocusColor = color("onBrand");

    // 字体与基础调色板属于主题安装；页面只选择 profile，避免自行填写字体与色值。
    for (const auto *file : {"Poppins-Regular.ttf", "Poppins-Medium.ttf", "Poppins-SemiBold.ttf"}) {
        if (QFontDatabase::addApplicationFont(":/button/" + QString::fromLatin1(file)) < 0)
            throw std::runtime_error("Cannot load component font");
    }
    QFont font(manifest["fontFamily"].toString());
    font.setPixelSize(manifest["bodyFontSize"].toInt());
    application.setFont(font);
    auto palette = application.palette();
    palette.setColor(QPalette::Window, color("app"));
    palette.setColor(QPalette::WindowText, color("foreground"));
    palette.setColor(QPalette::Base, color("surface"));
    palette.setColor(QPalette::AlternateBase, color("neutralHover"));
    palette.setColor(QPalette::Text, color("foreground"));
    palette.setColor(QPalette::Button, color("surface"));
    palette.setColor(QPalette::ButtonText, color("foreground"));
    palette.setColor(QPalette::Highlight, color("primary"));
    palette.setColor(QPalette::HighlightedText, color("primaryText"));
    palette.setColor(QPalette::Disabled, QPalette::Text, color("disabledText"));
    palette.setColor(QPalette::Disabled, QPalette::ButtonText, color("disabledText"));
    palette.setColor(QPalette::Disabled, QPalette::WindowText, color("disabledText"));
    application.setPalette(palette);
    const auto nativeQss = QString::fromUtf8(
        token_resources::read(":/native/" + profile + "/native-components.qss"));
    if (nativeQss.trimmed().isEmpty())
        throw std::runtime_error("Missing generated native component QSS");
    application.setStyleSheet(application.styleSheet() + nativeQss);
    const auto focus = selected["focusIndicator"].toObject();
    commonWidget::FocusIndicator::instance().configure(focus["mode"].toString(),
                                                       focus["opacity"].toDouble(-1));
    commonWidget::button_detail::installButtonRuntime(application, runtime);
    installedProfile = profile;
}
} // namespace component_theme
