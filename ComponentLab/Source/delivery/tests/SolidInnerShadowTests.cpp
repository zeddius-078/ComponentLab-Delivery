#include "SolidInnerShadow.h"
#include "ButtonAppearanceAdapter.h"
#include "DemoRuntime.h"
#include <QApplication>
#include <QDir>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QLabel>
#include <QPainter>
#include <QSignalSpy>
#include <QStyleFactory>
#include <QtTest>
#include <cmath>

using namespace surface::solid_inner_shadow;
using namespace commonWidget;
namespace {
std::shared_ptr<const surface::Catalog> catalog;
QJsonObject sample(const QString& state = "default") {
    return catalog->button("solid-inner-shadow", "DEFAULT", false, false, state).solidInnerShadow;
}
void settle() {
    QTest::qWait(25);
    QApplication::processEvents();
}
void exportReferenceCases(const QString& folder) {
    if (!QDir().mkpath(folder))
        throw std::runtime_error("Cannot create comparison folder");
    QJsonArray cases;
    for (QString profile :
         {"light-comfortable", "light-compact", "dark-comfortable", "dark-compact"}) {
        surface::Catalog source(profile);
        const QList<QSize> sizes{{120, 44}, {267, 48}, {360, 56}, {267, 48},
                                 {267, 48}, {120, 44}, {5, 4},    {120, 44}};
        for (int index = 0; index < sizes.size(); ++index)
            for (QString state : {"default", "hover", "pressed", "disabled"})
                for (qreal dpr : {1., 1.25, 1.5}) {
                    const auto data =
                        source.button("solid-inner-shadow", "DEFAULT", false, false, state)
                            .solidInnerShadow;
                    auto g = data["geometry"].toObject();
                    auto v = data["visual"].toObject();
                    // Reference geometry uses its original three-stop background interface.
                    v.remove("backgroundStops");
                    if (index == 3) {
                        g["width"] = 10.;
                        g["cornerRadius"] = 10.;
                    }
                    if (index == 4) {
                        g["matchButtonRadiusToInnerShadow"] = false;
                        g["buttonRadius"] = 14.;
                    }
                    if (index == 5) {
                        g["width"] = 0.;
                        g["cornerRadius"] = 0.;
                    }
                    if (index == 7) {
                        g["fadeStartRatio"] = 2.;
                        g["borderWidth"] = 20.;
                    }
                    const auto size = sizes[index];
                    const qreal width = size.width(), height = size.height();
                    const auto t = qBound(0., g["width"].toDouble(), qMin(width, height) / 2);
                    const auto r = qBound(0., g["cornerRadius"].toDouble(), t);
                    const auto f = qBound(0., g["fadeStartRatio"].toDouble(), .95);
                    QJsonArray points;
                    for (const auto& p : boundaryPoints(width, height, t, r, f))
                        points.append(QJsonArray{p.x(), p.y()});
                    const auto name =
                        QString("%1-%2-%3-%4.png").arg(profile).arg(index).arg(state).arg(dpr);
                    if (!renderBackground(size, dpr, g, v).save(folder + '/' + name))
                        throw std::runtime_error("Cannot save reference raster");
                    cases.append(QJsonObject{{"profile", profile},
                                             {"case", index},
                                             {"state", state},
                                             {"size", QJsonArray{size.width(), size.height()}},
                                             {"dpr", dpr},
                                             {"geometry", g},
                                             {"visual", v},
                                             {"boundary", points},
                                             {"image", name}});
                }
    }
    QFile file(folder + "/cases.json");
    if (!file.open(QIODevice::WriteOnly) || file.write(QJsonDocument(cases).toJson()) < 0)
        throw std::runtime_error("Cannot write reference cases");
}
} // namespace
class SolidInnerShadowTests : public QObject {
    Q_OBJECT
  private slots:
    void fiveStopSurfaceMatchesGradientMaterial() {
        for (QString type : {"PRIMARY", "DEFAULT", "DASHED"})
            for (bool danger : {false, true})
                for (QString state : {"default", "hover", "pressed", "disabled"}) {
                    const auto data =
                        catalog->button("solid-inner-shadow", type, danger, false, state)
                            .solidInnerShadow;
                    const auto visual = data["visual"].toObject();
                    const auto stops = visual["backgroundStops"].toArray();
                    QCOMPARE(stops.size(), 5);
                    const auto gradient =
                        catalog->button("gradient-inset", type, danger, false, state);
                    QLinearGradient expectedGradient(0, 0, 0, 40);
                    for (const auto& value : stops) {
                        const auto stop = value.toObject();
                        const auto at = stop["at"].toDouble();
                        const auto color = stop["color"].toString();
                        QVERIFY2(gradient.qss.contains(QString("stop:%1 %2").arg(at).arg(color)),
                                 qPrintable(type + '/' + state + '/' + color));
                        expectedGradient.setColorAt(at, surface::color(color));
                    }
                    QImage expected(160, 40, QImage::Format_ARGB32_Premultiplied);
                    QPainter painter(&expected);
                    painter.fillRect(expected.rect(), expectedGradient);
                    painter.end();
                    const auto actual =
                        renderBackground({160, 40}, 1, data["geometry"].toObject(), visual)
                            .toImage();
                    for (int y : {4, 9, 14, 20, 28, 34})
                        QCOMPARE(actual.pixelColor(80, y), expected.pixelColor(80, y));
                }
    }
    void shadowHasRestrainedTokenDimensions() {
        QCOMPARE(sample()["geometry"].toObject()["width"].toDouble(), 2.);
        for (QString type : {"PRIMARY", "DEFAULT"}) {
            const auto visual = catalog->button("solid-inner-shadow", type, false, false, "default")
                                    .solidInnerShadow["visual"]
                                    .toObject();
            QVERIFY(visual["blackMix"].toDouble() <= .1);
        }
    }
    void softShadowUsesButtonTokenDimensions() {
        for (QString preset : {"inset", "gradient-inset"})
            for (QString type : {"PRIMARY", "DEFAULT", "DASHED"})
                for (bool danger : {false, true})
                    for (QString state : {"default", "hover", "pressed", "disabled"}) {
                        const auto value = catalog->button(preset, type, danger, false, state);
                        if (state == "disabled") {
                            QVERIFY(value.shadows.isEmpty());
                            continue;
                        }
                        QCOMPARE(value.shadows.size(), 1);
                        const auto shadow = value.shadows[0].toObject();
                        QCOMPARE(shadow["alpha"].toDouble(), .18);
                        QCOMPARE(shadow["y"].toDouble(), 1.);
                        QCOMPARE(shadow["blur"].toDouble(), 3.);
                    }
    }
    void roleColorsAndDangerHaveGeometricShadow() {
        const auto primary =
            catalog->button("solid-inner-shadow", "PRIMARY", false, false, "default")
                .solidInnerShadow;
        const auto secondary = sample();
        QVERIFY(primary["visual"].toObject()["bgMid"] != secondary["visual"].toObject()["bgMid"]);
        QVERIFY(!catalog->button("solid-inner-shadow", "PRIMARY", true, false, "default")
                     .solidInnerShadow.isEmpty());
    }
    void referenceScopeAndDisabled() {
        QCOMPARE(catalog->presetIds().size(), 7);
        QCOMPARE(catalog->buttonPresetIds().size(), 8);
        QVERIFY(!catalog->presetIds().contains("solid-inner-shadow"));
        QVERIFY(!sample("disabled").isEmpty());
        for (QString type : {"PRIMARY", "DEFAULT", "DASHED", "TEXT", "LINK"})
            for (bool danger : {false, true})
                for (QString state : {"default", "hover", "pressed", "disabled"}) {
                    const auto visual =
                        catalog->button("solid-inner-shadow", type, danger, false, state)
                            .solidInnerShadow["visual"]
                            .toObject();
                    QVERIFY(!visual.isEmpty());
                    const double alpha = danger || type == "PRIMARY" || type == "LINK" ? .08 : .1;
                    QCOMPARE(visual["blackMix"].toDouble(), alpha);
                    const auto base = surface::color(visual["shadowBaseColor"].toString());
                    const auto shadow = surface::color(visual["shadowColor"].toString());
                    QCOMPARE(shadow, QColor(qRound(base.red() * (1 - alpha)),
                                            qRound(base.green() * (1 - alpha)),
                                            qRound(base.blue() * (1 - alpha))));
                }
        QVERIFY(!catalog->buttonUnsupportedReason("solid-inner-shadow", "PRIMARY", true).isEmpty());
        QCOMPARE(sample()["geometry"].toObject()["width"].toDouble(), 2.);
    }
    void mirroredBoundaryAndFallback() {
        for (const auto& size : QList<QSize>{{120, 44}, {267, 48}, {360, 56}}) {
            const auto left = sidePath(size.width(), size.height(), 4, 4, 1. / 3, true);
            const auto right = sidePath(size.width(), size.height(), 4, 4, 1. / 3, false);
            QCOMPARE(left.elementCount(), right.elementCount());
            for (int i = 0; i < left.elementCount(); ++i) {
                QVERIFY(qAbs(left.elementAt(i).x + right.elementAt(i).x - size.width()) < 1e-10);
                QCOMPARE(left.elementAt(i).y, right.elementAt(i).y);
            }
            QVERIFY(arcProfile(size.width(), size.height(), 4, 4, 1. / 3).useArc);
        }
        QVERIFY(!arcProfile(120, 44, 4, 0, 1. / 3).useArc);
        QVERIFY(sidePath(5, 4, 2, 2, .95, true).isEmpty());
    }
    void dashedBorderPreservesTheMaterial() {
        const auto normal = sample();
        const auto dashed = catalog->button("solid-inner-shadow", "DASHED", false, false, "default")
                                .solidInnerShadow;
        QCOMPARE(dashed["geometry"], normal["geometry"]);
        QCOMPARE(dashed["visual"].toObject()["shadowColor"],
                 normal["visual"].toObject()["shadowColor"]);
        QCOMPARE(dashed["visual"].toObject()["borderStyle"].toString(), QString("dashed"));
        const auto solidImage = renderBackground({267, 48}, 1, normal["geometry"].toObject(),
                                                 normal["visual"].toObject())
                                    .toImage();
        const auto dashedImage = renderBackground({267, 48}, 1, dashed["geometry"].toObject(),
                                                  dashed["visual"].toObject())
                                     .toImage();
        QVERIFY(solidImage != dashedImage);
        QCOMPARE(solidImage.pixelColor(100, 46), dashedImage.pixelColor(100, 46));
    }
    void continuousBottomAndDpr() {
        auto g = sample()["geometry"].toObject();
        g["borderWidth"] = 0;
        for (const auto& size : QList<QSize>{{120, 44}, {267, 48}, {360, 56}})
            for (QString state : {"default", "hover", "pressed", "disabled"})
                for (qreal dpr : {1., 1.25, 1.5}) {
                    const auto v = sample(state)["visual"].toObject();
                    const auto pixmap = renderBackground(size, dpr, g, v);
                    QCOMPARE(pixmap.devicePixelRatioF(), dpr);
                    QCOMPARE(pixmap.width(), int(std::nearbyint(size.width() * dpr)));
                    const auto image = pixmap.toImage();
                    const auto color = surface::color(v["shadowColor"].toString());
                    const int y = int(std::ceil((size.height() - g["width"].toDouble()) * dpr));
                    for (int x = int(6 * dpr); x < int((size.width() - 6) * dpr); ++x)
                        QCOMPARE(image.pixelColor(x, y), color);
                }
    }
    void cacheInvalidationAndProfile() {
        Layer layer;
        auto g = sample()["geometry"].toObject();
        for (QString state : {"default", "hover", "pressed", "disabled"}) {
            layer.setTokens(g, sample(state)["visual"].toObject(), state);
            layer.backgroundPixmap({267, 48}, 1);
        }
        QCOMPARE(layer.cacheEntries(), 4);
        auto v = sample()["visual"].toObject();
        layer.setTokens(g, v, "default");
        layer.backgroundPixmap({267, 48}, 1);
        QCOMPARE(layer.renderCount(), 4);
        layer.backgroundPixmap({267, 48}, 1.25);
        QCOMPARE(layer.renderCount(), 5);
        QCOMPARE(layer.cacheEntries(), 4);
        v["shadowColor"] = "#ff0000";
        layer.setTokens(g, v, "default");
        layer.backgroundPixmap({267, 48}, 1.25);
        QCOMPARE(layer.renderCount(), 6);
        g["width"] = 10.;
        layer.setTokens(g, v, "default");
        QCOMPARE(layer.cacheEntries(), 0);
        layer.backgroundPixmap({267, 48}, 1.25);
        layer.clearCache();
        QCOMPARE(layer.cacheEntries(), 0);
        const auto other = surface::Catalog("dark-comfortable")
                               .button("solid-inner-shadow", "DEFAULT", false, false, "default")
                               .solidInnerShadow;
        layer.setTokens(other["geometry"].toObject(), other["visual"].toObject(), "default");
        QCOMPARE(
            layer.backgroundPixmap({267, 48}, 1).toImage(),
            renderBackground({267, 48}, 1, other["geometry"].toObject(), other["visual"].toObject())
                .toImage());
    }
    void buttonInteractionAndContent() {
        Z_PushButton button;
        button.setText("Alpha");
        button.setLeadingIconSource(":/icons/arrow-left.svg");
        button.setTrailingIconSource(":/icons/arrow-right.svg");
        ButtonAppearanceAdapter adapter(&button, catalog);
        adapter.setAppearance("solid-inner-shadow");
        button.setFixedSize(267, 48);
        button.show();
        settle();
        button.clearFocus();
        auto* layer = button.findChild<Layer*>();
        QVERIFY(layer && !layer->isHidden());
        for (const auto& state : QStringList{"default", "hover", "pressed", "disabled"}) {
            button.setEnabled(true);
            button.setAttribute(Qt::WA_UnderMouse, state == "hover" || state == "pressed");
            button.setDown(state == "pressed");
            if (state == "disabled")
                button.setEnabled(false);
            adapter.refresh();
            settle();
            QCOMPARE(layer->state(), state);
        }
        button.setEnabled(true);
        button.setDown(false);
        button.setAttribute(Qt::WA_UnderMouse, false);
        adapter.refresh();
        settle();
        const int renders = layer->renderCount();
        button.setText("Bravo");
        adapter.refresh();
        settle();
        QCOMPARE(layer->renderCount(), renders);
        QCOMPARE(button.leadingIconSource(), QString(":/icons/arrow-left.svg"));
        QCOMPARE(button.trailingIconSource(), QString(":/icons/arrow-right.svg"));
        button.setAccessibleName("Back");
        button.setIconOnly(true);
        settle();
        const auto size = button.size();
        button.setLoading(true);
        settle();
        adapter.refresh();
        QCOMPARE(layer->state(), QString("default"));
        QCOMPARE(button.size(), size);
        QSignalSpy clicks(&button, &QPushButton::clicked);
        button.click();
        QCOMPARE(clicks.count(), 0);
        button.setLoading(false);
        button.setIconOnly(false);
        settle();
        QVERIFY(!button.findChild<QWidget*>("LEADING")->isHidden());
        QVERIFY(!button.findChild<QWidget*>("TRAILING")->isHidden());
        adapter.setAppearance("gradient-inset");
        settle();
        QVERIFY(layer->isHidden());
        QVERIFY(button.styleSheet().contains("qlineargradient"));
    }
};
int main(int argc, char** argv) {
    QCoreApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
    QCoreApplication::setAttribute(Qt::AA_UseHighDpiPixmaps);
    QApplication app(argc, argv);
    app.setStyle(QStyleFactory::create("Fusion"));
    const auto profile = qEnvironmentVariable("DEMO_PROFILE", "light-comfortable");
    button_demo::install(app, profile);
    catalog = std::make_shared<surface::Catalog>(profile);
    if (app.arguments().size() == 3 && app.arguments()[1] == "--export") {
        exportReferenceCases(app.arguments()[2]);
        return 0;
    }
    SolidInnerShadowTests tests;
    return QTest::qExec(&tests, argc, argv);
}
#include "SolidInnerShadowTests.moc"
