#include "ComponentTheme.h"
#include "ReferencePage.h"
#include <QApplication>
#include <QCommandLineParser>
#include <QDebug>

int main(int argc, char **argv) {
    QApplication application(argc, argv);
    QCommandLineParser parser;
    parser.addHelpOption();
    parser.addOption({"profile", "Token profile", "name", "light-comfortable"});
    parser.process(application);
    try {
        const auto profile = parser.value("profile");
        component_theme::install(application, profile);
        ReferencePage page(profile);
        // 既有映射示例的数据在应用入口注入；页面接口可替换为真实业务提供的数据。
        page.setChambers({{"etch-01", QStringLiteral("刻蚀腔体 ETCH-01"), {}, false},
                          {"etch-02", QStringLiteral("刻蚀腔体 ETCH-02"), {}, false},
                          {"etch-03", QStringLiteral("维护中 ETCH-03"), {}, true}});
        auto fields = page.fields();
        fields.chamberIds = QStringList{"etch-01"};
        page.setFields(fields);
        page.show();
        return application.exec();
    } catch (const std::exception &error) {
        qCritical("%s", error.what());
        return 1;
    }
}
