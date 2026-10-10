#include "Z_CheckBox.h"
#include "ButtonAppearanceAdapter.h"
#include "CheckBoxGroupController.h"
#include "DemoRuntime.h"
#include <QAccessible>
#include <QApplication>
#include <QDir>
#include <QFrame>
#include <QLabel>
#include <QPushButton>
#include <QSignalSpy>
#include <QStyleFactory>
#include <QtTest>

using commonWidget::Z_CheckBox;
using commonWidget::Z_PushButton;
namespace {
std::shared_ptr<const surface::Catalog> catalog;
void events() {
    QApplication::processEvents();
    QTest::qWait(15);
    QApplication::processEvents();
}
QImage faceImage(const QString& preset) {
    QWidget host;
    host.resize(240, 140);
    host.setStyleSheet("QWidget#pixelHost { background:#ffffff; }");
    host.setObjectName("pixelHost");
    Z_CheckBox c(catalog, "", &host);
    c.setAppearance(preset);
    c.setChecked(true);
    c.move(70, 50);
    c.resize(c.sizeHint());
    host.show();
    events();
    c.clearFocus();
    auto* glyph = c.findChild<QLabel*>("GLYPH");
    glyph->hide();
    events();
    return host.grab(c.indicatorRect().translated(c.pos())).toImage();
}
int distance(const QColor& a, const QColor& b) {
    return qAbs(a.red() - b.red()) + qAbs(a.green() - b.green()) + qAbs(a.blue() - b.blue());
}
QColor at(const QImage& image, double x, double y) {
    return image.pixelColor(qBound(0, int(image.width() * x), image.width() - 1),
                            qBound(0, int(image.height() * y), image.height() - 1));
}
} // namespace
class ContractTests : public QObject {
    Q_OBJECT
  private slots:
    void binaryAndIndependentMixed() {
        Z_CheckBox c(catalog, "Quality");
        QSignalSpy toggle(&c, &QAbstractButton::toggled),
            request(&c, &Z_CheckBox::checkedChangeRequested);
        c.setIndeterminate(true);
        QVERIFY(!c.isChecked());
        QCOMPARE(c.checkState(), Qt::Unchecked);
        c.click();
        QVERIFY(c.isChecked());
        QVERIFY(c.isIndeterminate());
        QCOMPARE(c.checkState(), Qt::Checked);
        c.click();
        QVERIFY(!c.isChecked());
        QCOMPARE(toggle.count(), 2);
        QCOMPARE(request.count(), 2);
    }
    void controlledRequestAndCommit() {
        Z_CheckBox c(catalog, "Alarm");
        c.setControlled(true);
        QSignalSpy request(&c, &Z_CheckBox::checkedChangeRequested),
            toggle(&c, &QAbstractButton::toggled);
        c.click();
        QVERIFY(!c.isChecked());
        QCOMPARE(request.count(), 1);
        QVERIFY(request[0][0].toBool());
        QCOMPARE(toggle.count(), 0);
        c.setChecked(true);
        QCOMPARE(toggle.count(), 1);
        c.click();
        QVERIFY(c.isChecked());
        QVERIFY(!request[1][0].toBool());
    }
    void disabledAndParentDisabled() {
        QWidget parent;
        Z_CheckBox c(catalog, "Quality", &parent);
        QSignalSpy request(&c, &Z_CheckBox::checkedChangeRequested);
        c.setEnabled(false);
        c.click();
        QCOMPARE(request.count(), 0);
        QCOMPARE(c.interactionState(), QString("disabled"));
        c.setEnabled(true);
        parent.setEnabled(false);
        c.click();
        QCOMPARE(request.count(), 0);
        QCOMPARE(c.interactionState(), QString("disabled"));
        parent.setEnabled(true);
        c.click();
        QVERIFY(c.isChecked());
    }
    void keyboardAndWholeLabelClick() {
        Z_CheckBox c(catalog, "Quality inspection");
        c.resize(c.sizeHint());
        c.show();
        c.setFocus();
        events();
        QTest::keyClick(&c, Qt::Key_Space);
        QVERIFY(c.isChecked());
        QTest::mouseClick(&c, Qt::LeftButton, Qt::NoModifier,
                          QPoint(c.width() - 3, c.height() / 2));
        QVERIFY(!c.isChecked());
        QWidget parent;
        Z_CheckBox a(catalog, "A", &parent), b(catalog, "B", &parent);
        a.move(0, 0);
        b.move(0, 40);
        parent.resize(200, 100);
        parent.show();
        a.setFocus();
        events();
        QTest::keyClick(&a, Qt::Key_Tab);
        QVERIFY(b.hasFocus());
    }
    void accessibilityMixedAndAction() {
        Z_CheckBox c(catalog, "Accessible check");
        c.setIndeterminate(true);
        auto* a = QAccessible::queryAccessibleInterface(&c);
        QVERIFY(a);
        QCOMPARE(a->role(), QAccessible::CheckBox);
        QVERIFY(a->state().checkable);
        QVERIFY(a->state().checkStateMixed);
        QVERIFY(!a->state().checked);
        QCOMPARE(a->childCount(), 0);
        QCOMPARE(a->text(QAccessible::Name), QString("Accessible check"));
        a->actionInterface()->doAction(QAccessibleActionInterface::toggleAction());
        QVERIFY(c.isChecked());
        c.setEnabled(false);
        a->actionInterface()->doAction(QAccessibleActionInterface::toggleAction());
        QVERIFY(c.isChecked());
    }
    void groupSummaryAndDisabledPreservation() {
        Z_CheckBox all(catalog, "All"), a(catalog, "A"), b(catalog, "B"), locked(catalog, "Locked");
        a.setObjectName("A");
        b.setObjectName("B");
        locked.setObjectName("Locked");
        locked.setChecked(true);
        locked.setEnabled(false);
        CheckBoxGroupController group(&all, {&a, &b, &locked});
        QVERIFY(!all.isChecked());
        QVERIFY(!all.isIndeterminate());
        a.click();
        QVERIFY(all.isIndeterminate());
        all.click();
        QVERIFY(a.isChecked() && b.isChecked() && locked.isChecked());
        QVERIFY(all.isChecked());
        all.click();
        QVERIFY(!a.isChecked() && !b.isChecked() && locked.isChecked());
        QCOMPARE(group.values(), QStringList{"Locked"});
        a.setEnabled(false);
        b.setEnabled(false);
        events();
        QVERIFY(!all.isEnabled());
        QVERIFY(!all.isIndeterminate());
    }
    void invalidPresetAndSizeAreAtomic() {
        Z_CheckBox c(catalog, "Value");
        QVERIFY_EXCEPTION_THROWN(c.setAppearance("missing"), std::invalid_argument);
        QCOMPARE(c.appearance(), QString("solid"));
        QVERIFY_EXCEPTION_THROWN(c.setControlSize("xxl"), std::invalid_argument);
        QCOMPARE(c.controlSize(), QString("sm"));
    }
    void geometryAndTypeface() {
        Z_CheckBox c(catalog, "Quality check");
        c.show();
        for (QString size : {"sm", "lg"}) {
            c.setControlSize(size);
            c.resize(c.sizeHint());
            events();
            auto geometry = c.sizeHint();
            auto* label = c.findChild<QLabel*>("LABEL");
            QVERIFY(label->font().pixelSize() >= 14);
            QVERIFY(geometry.height() >= 32);
            for (auto preset : catalog->presetIds()) {
                c.setAppearance(preset);
                events();
                QCOMPARE(c.sizeHint(), geometry);
                QCOMPARE(c.indicatorRect().width(),
                         catalog->checkboxMetrics(size)["indicator"].toInt());
            }
        }
        c.setLayoutDirection(Qt::RightToLeft);
        events();
        QVERIFY(c.indicatorRect().left() > c.width() / 2);
    }
    void allCheckboxStatesRender() {
        Z_CheckBox c(catalog, "Quality check");
        c.resize(c.sizeHint());
        c.show();
        int cases = 0;
        for (auto preset : catalog->presetIds())
            for (bool selected : {false, true})
                for (bool mixed : {false, true})
                    for (bool disabled : {false, true}) {
                        c.setAppearance(preset);
                        c.setChecked(selected);
                        c.setIndeterminate(mixed);
                        c.setEnabled(!disabled);
                        events();
                        auto image = c.grab().toImage();
                        QVERIFY(!image.isNull());
                        QVERIFY(image.width() >= 32);
                        ++cases;
                    }
        QCOMPARE(cases, catalog->presetIds().size() * 8);
    }
    void fillAndShadowPixelProof() {
        auto solid = faceImage("solid"), gradient = faceImage("gradient"),
             inset = faceImage("inset"), outline = faceImage("outline");
        QVERIFY2(distance(at(solid, .5, .25), at(solid, .5, .75)) <= 3,
                 "Solid interior should be uniform");
        QVERIFY2(distance(at(gradient, .5, .25), at(gradient, .5, .75)) > 12,
                 "Gradient must contain different interior colors");
        QVERIFY2(distance(at(solid, .5, .18), at(inset, .5, .18)) > 5,
                 "Inset must change pixels inside the face");
        QVERIFY2(distance(at(outline, .5, .5), QColor(Qt::white)) <= 3,
                 "Outline interior must expose host background");
        QString path = qEnvironmentVariable("DEMO_EVIDENCE");
        if (!path.isEmpty()) {
            QDir().mkpath(path);
            QVERIFY(solid.save(path + "/solid.png"));
            QVERIFY(gradient.save(path + "/gradient.png"));
            QVERIFY(inset.save(path + "/inset.png"));
            QVERIFY(outline.save(path + "/outline.png"));
        }
    }
    void gradientStatesAreDistinct() {
        auto normal = catalog->checkbox("gradient", true, "default"),
             hover = catalog->checkbox("gradient", true, "hover"),
             pressed = catalog->checkbox("gradient", true, "pressed");
        QVERIFY(normal.qss != hover.qss);
        QVERIFY(normal.qss != pressed.qss);
        QVERIFY(hover.qss != pressed.qss);
        auto disabled = catalog->checkbox("gradient-elevated", true, "disabled");
        QVERIFY(disabled.shadows.isEmpty());
        QVERIFY(!disabled.qss.contains("qlineargradient"));
    }
    void shadowMaskAndDpr() {
        auto r = catalog->checkbox("gradient-elevated", true, "default");
        for (double dpr : {1., 1.5, 2.})
            for (auto v : r.shadows) {
                auto s = v.toObject();
                bool inset = s["kind"] == "inset";
                int pad = 0;
                auto image = surface::shadowImage({40, 32}, dpr, 4, 1, s, inset, &pad);
                QVERIFY(!image.isNull());
                QCOMPARE(image.devicePixelRatio(), dpr);
                int inside = 0, outside = 0;
                for (int y = 0; y < image.height(); ++y)
                    for (int x = 0; x < image.width(); ++x)
                        if (image.pixelColor(x, y).alpha() > 2) {
                            double lx = x / dpr - pad, ly = y / dpr - pad;
                            if (lx > 2 && lx < 38 && ly > 2 && ly < 30)
                                ++inside;
                            else if (lx < 0 || lx >= 40 || ly < 0 || ly >= 32)
                                ++outside;
                        }
                if (inset) {
                    QVERIFY(inside > 0);
                    QCOMPARE(outside, 0);
                } else {
                    QCOMPARE(inside, 0);
                    QVERIFY(outside > 0);
                }
                QCOMPARE(surface::shadowImage({40, 32}, dpr, 4, 1, s, inset), image);
            }
    }
    void buttonAppearanceAndIconLoading() {
        Z_PushButton b;
        b.setType(commonWidget::ButtonType::Primary);
        b.setText("Confirm");
        b.setAccessibleName("Back");
        b.setLeadingIconSource(":/icons/arrow-left.svg");
        ButtonAppearanceAdapter adapter(&b, catalog);
        b.show();
        events();
        b.resize(b.sizeHint());
        for (auto preset : catalog->presetIds())
            for (bool iconOnly : {false, true}) {
                b.setIconOnly(iconOnly);
                b.adjustSize();
                events();
                auto size = b.size();
                adapter.setAppearance(preset);
                events();
                QCOMPARE(b.size(), size);
                if (iconOnly)
                    QCOMPARE(b.width(), b.height());
                QSignalSpy clicked(&b, &QAbstractButton::clicked);
                b.setLoading(true);
                events();
                QCOMPARE(b.size(), size);
                b.click();
                QCOMPARE(clicked.count(), 0);
                QVERIFY(b.findChild<QWidget*>("LoadingIndicator")->isVisible());
                auto first = b.grab().toImage();
                QTest::qWait(70);
                auto second = b.grab().toImage();
                QVERIFY(first != second);
                b.setLoading(false);
                events();
                b.click();
                QCOMPARE(clicked.count(), 1);
            }
    }
    void buttonTypesRetainSemantics() {
        Z_PushButton b;
        b.setText("Confirm");
        ButtonAppearanceAdapter adapter(&b, catalog);
        b.show();
        for (auto type : {commonWidget::ButtonType::Primary, commonWidget::ButtonType::Default,
                          commonWidget::ButtonType::Dashed, commonWidget::ButtonType::Text,
                          commonWidget::ButtonType::Link})
            for (bool danger : {false, true})
                for (bool ghost : {false, true}) {
                    b.setType(type);
                    b.setDanger(danger);
                    b.setGhost(ghost);
                    adapter.setAppearance("gradient-elevated");
                    events();
                    auto r =
                        catalog->button("gradient-elevated", b.property("buttonType").toString(),
                                        danger, ghost, "default");
                    if (ghost || type == commonWidget::ButtonType::Text ||
                        type == commonWidget::ButtonType::Link)
                        QVERIFY(r.shadows.isEmpty());
                    b.setEnabled(false);
                    events();
                    QCOMPARE(b.property("visualState").toString(), QString("DISABLED"));
                    QVERIFY(!b.graphicsEffect()->isEnabled());
                    b.setEnabled(true);
                }
    }
    void buttonFilledGlyphRetainsContrastRole() {
        Z_PushButton b;
        b.setType(commonWidget::ButtonType::Default);
        b.setText("Confirm");
        b.setLeadingIconSource(":/icons/arrow-left.svg");
        b.setLeadingIconStyle(commonWidget::IconContainerStyle::Filled);
        ButtonAppearanceAdapter adapter(&b, catalog);
        b.show();
        events();
        for (auto preset : catalog->presetIds()) {
            adapter.setAppearance(preset);
            events();
            auto* glyph = b.findChild<QWidget*>("LEADING")->findChild<QLabel*>("GLYPH");
            auto r = catalog->button(preset, "DEFAULT", false, false, "default");
            QCOMPARE(glyph->palette().color(QPalette::WindowText), r.iconFilledForeground);
            QVERIFY(r.iconFilledForeground != r.foreground);
        }
    }
};
int main(int argc, char** argv) {
    QCoreApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
    QCoreApplication::setAttribute(Qt::AA_UseHighDpiPixmaps);
    QApplication app(argc, argv);
    app.setStyle(QStyleFactory::create("Fusion"));
    auto profile = qEnvironmentVariable("DEMO_PROFILE", "light-comfortable");
    button_demo::install(app, profile);
    catalog = std::make_shared<surface::Catalog>(profile);
    ContractTests tests;
    return QTest::qExec(&tests, argc, argv);
}
#include "ContractTests.moc"
