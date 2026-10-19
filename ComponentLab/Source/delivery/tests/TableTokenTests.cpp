#include "DemoRuntime.h"
#include "DataViewsGallery.h"
#include "TokenResources.h"
#include "native/Z_TableView.h"
#include <QApplication>
#include <QDir>
#include <QGraphicsProxyWidget>
#include <QGraphicsScene>
#include <QGraphicsView>
#include <QHeaderView>
#include <QImage>
#include <QJsonDocument>
#include <QStandardItemModel>
#include <QStyleFactory>
#include <cstdio>
#include <stdexcept>

namespace {
void require(bool condition, const char* message) {
    if (!condition)
        throw std::runtime_error(message);
}

QJsonValue token(const QJsonObject& profile, const QString& path) {
    for (const auto& category : {"color", "layout", "number", "font", "boolean", "string", "asset"}) {
        const auto values = profile[QString::fromLatin1(category)].toObject();
        if (values.contains(path))
            return values[path];
    }
    throw std::runtime_error(("Missing test token: " + path).toStdString());
}
}

int main(int argc, char** argv) {
    QApplication app(argc, argv);
    app.setStyle(QStyleFactory::create(QStringLiteral("Fusion")));
    const QString profileName = qEnvironmentVariable("DEMO_PROFILE", "standard-light");
    try {
        button_demo::install(app, profileName);
        const auto document = QJsonDocument::fromJson(
            token_resources::read(QStringLiteral(":/tokens/tokens_qt.json")));
        const auto profile = document.object()[QStringLiteral("profiles")]
                                 .toObject()[profileName].toObject();
        require(!profile.isEmpty(), "Table test profile is unavailable");

        commonWidget::Z_TableView table;
        QStandardItemModel model(3, 3, &table);
        table.setModel(&model);
        require(table.density() == QStringLiteral("compact"), "Wrong default Table density");
        require(table.striping() == QStringLiteral("none"), "Wrong default Table striping");

        for (const auto& density : {QStringLiteral("compact"), QStringLiteral("small"),
                                    QStringLiteral("medium"), QStringLiteral("large")}) {
            table.setDensity(density);
            const int expected = qRound(token(
                profile, QStringLiteral("component.table.base.density.%1.slot.row.min-height")
                             .arg(density)).toDouble());
            require(table.verticalHeader()->defaultSectionSize() == expected,
                    "Table row height does not consume the active density Token");
            require(table.property("tableDensity").toString() == density,
                    "Table density property is stale");
        }

        table.setStriping(QStringLiteral("zebra"));
        require(table.alternatingRowColors(), "Table zebra striping is not active");
        require(table.property("tableStriping").toString() == QStringLiteral("zebra"),
                "Table striping property is stale");
        table.setStriping(QStringLiteral("none"));
        require(!table.alternatingRowColors(), "Table none striping still alternates rows");

        bool rejectedDensity = false;
        try { table.setDensity(QStringLiteral("legacy")); }
        catch (const std::invalid_argument&) { rejectedDensity = true; }
        require(rejectedDensity, "Table accepted an undeclared density");

        bool rejectedStriping = false;
        try { table.setStriping(QStringLiteral("bands")); }
        catch (const std::invalid_argument&) { rejectedStriping = true; }
        require(rejectedStriping, "Table accepted an undeclared striping mode");

        const QString qss = app.styleSheet();
        const qsizetype tableStart = qss.indexOf(QStringLiteral("/* Z_TableView"));
        const qsizetype nextComponent = qss.indexOf(QStringLiteral("/* Z_ColumnView"), tableStart);
        const QString tableQss = tableStart >= 0
            ? qss.mid(tableStart, nextComponent > tableStart ? nextComponent - tableStart : -1)
            : QString();
        require(tableQss.contains(QStringLiteral("commonWidget::Z_TableView")),
                "Generated Table QSS was not installed");
        require(tableQss.contains(token(profile,
            QStringLiteral("component.table.overlay.cell-selection.selected.slot.cell.background-color"))
                                 .toString(), Qt::CaseInsensitive),
                "Generated Table QSS does not contain the selected Token value");
        require(!tableQss.contains(QStringLiteral("#F3EFFF"), Qt::CaseInsensitive),
                "Legacy Table selection color leaked into generated QSS");

        std::unique_ptr<QWidget> gallery(
            native_gallery::createDataViewsGallery(QStringLiteral("717:2"), nullptr));
        auto* view = qobject_cast<QGraphicsView*>(gallery.get());
        require(view && view->scene(), "Table visual overview was not created");
        gallery->resize(1200, 800);
        gallery->show();
        QApplication::processEvents();
        QWidget* canvas = nullptr;
        for (QGraphicsItem* item : view->scene()->items()) {
            if (auto* proxy = qgraphicsitem_cast<QGraphicsProxyWidget*>(item)) {
                canvas = proxy->widget();
                break;
            }
        }
        require(canvas, "Table visual overview canvas is missing");
        const QImage overview = canvas->grab().toImage().convertToFormat(QImage::Format_ARGB32);
        const QRgb selectedRgb = QColor(token(profile,
            QStringLiteral("component.table.overlay.cell-selection.selected.slot.cell.background-color"))
                                              .toString()).rgba();
        const QRgb headerRgb = QColor(token(profile,
            QStringLiteral("component.table.base.shared.slot.header.background-color"))
                                            .toString()).rgba();
        qsizetype selectedPixels = 0;
        qsizetype headerPixels = 0;
        for (int y = 0; y < overview.height(); ++y) {
            const auto* line = reinterpret_cast<const QRgb*>(overview.constScanLine(y));
            for (int x = 0; x < overview.width(); ++x) {
                selectedPixels += line[x] == selectedRgb;
                headerPixels += line[x] == headerRgb;
            }
        }
        require(selectedPixels > 1000,
                "Table visual overview does not paint the selected Token color");
        require(headerPixels > 1000,
                "Table visual overview does not paint the header Token color");
        const QString evidence = qEnvironmentVariable("DEMO_EVIDENCE");
        if (!evidence.isEmpty()) {
            require(QDir().mkpath(evidence), "Cannot create Table evidence directory");
            require(overview.save(evidence + QStringLiteral("/table-overview.png")),
                    "Cannot save Table visual overview evidence");
        }
        return 0;
    } catch (const std::exception& error) {
        std::fprintf(stderr, "Table Token test: %s\n", error.what());
        return 1;
    }
}
