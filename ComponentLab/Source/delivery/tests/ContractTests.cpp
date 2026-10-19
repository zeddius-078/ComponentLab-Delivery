#include "Z_CheckBox.h"
#include "LabelAppearance.h"
#include "ProgressAppearance.h"
#include "RadioAppearance.h"
#include "native/Z_RadioButton.h"
#include "CheckBoxGroupController.h"
#include "DemoRuntime.h"
#include <QAccessible>
#include <QApplication>
#include <QDir>
#include <QFile>
#include <QFrame>
#include <QLabel>
#include <QFontInfo>
#include <QPushButton>
#include <QProgressBar>
#include <QRadioButton>
#include <QRegularExpression>
#include <QButtonGroup>
#include <QSvgRenderer>
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
    void figmaAppearancePreservesNativeContract() {
        QWidget host;
        Z_CheckBox c(catalog,"Checkbox",&host);
        c.setAppearance("figma-light");c.setControlSize("lg");
        c.setFixedSize(108,32);c.move(10,10);host.resize(140,60);host.show();events();
        QCOMPARE(c.designContentRect(),QRect(4,4,100,24));
        QCOMPARE(c.indicatorRect(),QRect(6,6,20,20));
        auto* label=c.findChild<QLabel*>("LABEL");QVERIFY(label);
        QCOMPARE(label->geometry().left(),36);
        QCOMPARE(QFontInfo(label->font()).family(),QString("HarmonyOS Sans"));
        QCOMPARE(label->font().pixelSize(),14);
        QSignalSpy requested(&c,&Z_CheckBox::checkedChangeRequested);
        QTest::mouseClick(&c,Qt::LeftButton,Qt::NoModifier,QPoint(100,16));
        QVERIFY(c.isChecked());QCOMPARE(requested.count(),1);
        c.setChecked(false);c.setIndeterminate(true);
        QVERIFY(!c.isChecked());QVERIFY(c.isIndeterminate());QVERIFY(!c.isTristate());
        c.setProperty("figmaPreviewState","Hover");
        QCOMPARE(c.interactionState(),QString("hover"));
        QCOMPARE(c.checkState(),Qt::Unchecked);
        c.setEnabled(false);QCOMPARE(c.interactionState(),QString("disabled"));
        c.click();QCOMPARE(requested.count(),1);
        c.setEnabled(true);c.setControlSize("sm");
        QCOMPARE(c.indicatorRect().size(),QSize(18,18));
        QCOMPARE(c.designContentRect().height(),20);
        c.setAppearance("solid");
        QCOMPARE(c.appearance(),QString("solid"));
    }
    void v2ButtonKeepsNativeInteractionAndState() {
        QWidget host;Z_PushButton b(&host);
        b.setText("Button");b.setType(commonWidget::ButtonType::Primary);b.setSize(commonWidget::ButtonSize::S);
        b.resize(b.sizeHint());host.resize(160,80);host.show();events();
        QVERIFY(b.height() > 0);QCOMPARE(b.height() % 2, 0);
        QSignalSpy clicked(&b,&QPushButton::clicked);
        QTest::mouseClick(&b,Qt::LeftButton);QCOMPARE(clicked.count(),1);
        b.setProperty("figmaPreviewState","hover");events();QVERIFY(!b.isDown());
        QCOMPARE(b.property("buttonIntent").toString(),QString("primary"));
        QCOMPARE(b.property("buttonAppearance").toString(),QString("filled"));
        b.setProperty("figmaPreviewState","active");events();QVERIFY(!b.isDown());
        b.setEnabled(false);events();b.click();QCOMPARE(clicked.count(),1);
        QCOMPARE(b.property("visualState").toString(),QString("disabled"));
        b.setEnabled(true);b.setProperty("figmaPreviewState",QString());
        const auto normal=b.size();b.setLoading(true);events();
        QCOMPARE(b.size(),normal);QVERIFY(!b.isEnabled());b.click();QCOMPARE(clicked.count(),1);
        const auto first=b.grab().toImage();QTest::qWait(120);const auto second=b.grab().toImage();
        QVERIFY(first!=second);
        b.setLoading(false);events();QVERIFY(b.isEnabled());QCOMPARE(b.size(),normal);
        QVERIFY(b.findChild<QLabel*>("LABEL")->isVisible());
    }
    void capturedRadioKeepsNativeExclusiveKeyboardAndAssets() {
        QWidget host;host.setStyleSheet("background:white;");host.resize(400,100);
        commonWidget::Z_RadioButton first(&host),second(&host);
        first.setText("&First");second.setText("Second");first.resize(180,32);second.setGeometry(190,0,180,32);
        QButtonGroup group;group.addButton(&first,1);group.addButton(&second,2);
        commonWidget::applyFigmaRadioAppearance(&first,"Default");
        commonWidget::applyFigmaRadioAppearance(&second);
        first.setChecked(true);host.show();events();
        QCOMPARE(QFontInfo(first.font()).family(),QString("HarmonyOS Sans"));
        QSignalSpy clicks(&second,&QAbstractButton::clicked),toggles(&first,&QAbstractButton::toggled);
        QTest::mouseClick(&second,Qt::LeftButton,Qt::NoModifier,QPoint(8,16));
        QCOMPARE(group.checkedId(),2);QCOMPARE(clicks.count(),1);QCOMPARE(toggles.count(),1);
        first.setFocus(Qt::TabFocusReason);QTest::keyClick(&first,Qt::Key_Space);
        QCOMPARE(group.checkedId(),1);QVERIFY(!second.isChecked());
        commonWidget::applyFigmaRadioAppearance(&first,"Pressed",false);
        events();
        const auto pressed=first.grab().toImage();
        QCOMPARE(pressed.pixelColor(1,16).alpha(),0);
        QCOMPARE(pressed.pixelColor(11,5).alpha(),0);
        QVERIFY(!first.isDown());QCOMPARE(first.text(),QString("&First"));
        QCOMPARE(group.checkedId(),1);QVERIFY(first.autoExclusive());
        auto* accessible=QAccessible::queryAccessibleInterface(&first);QVERIFY(accessible);
        QVERIFY(accessible->state().checked);QVERIFY(!accessible->text(QAccessible::Name).isEmpty());
        commonWidget::applyFigmaRadioAppearance(&first,"Default");events();
        const auto selectedImage = first.grab().toImage();
        bool hasSelectedInk = false;
        for (int y = 0; y < selectedImage.height() && !hasSelectedInk; ++y)
            for (int x = 0; x < qMin(48, selectedImage.width()); ++x)
                if (distance(selectedImage.pixelColor(x, y), QColor("#6235f7")) < 20) {
                    hasSelectedInk = true;
                    break;
                }
        QVERIFY(hasSelectedInk);
        first.setEnabled(false);events();
        const auto disabledImage = first.grab().toImage();
        bool hasDisabledInk = false;
        for (int y = 0; y < disabledImage.height() && !hasDisabledInk; ++y)
            for (int x = 0; x < qMin(48, disabledImage.width()); ++x)
                if (distance(disabledImage.pixelColor(x, y), QColor("#8a8a8a")) < 20) {
                    hasDisabledInk = true;
                    break;
                }
        QVERIFY(hasDisabledInk);
        first.click();QCOMPARE(group.checkedId(),1);
        first.setEnabled(true);first.setLayoutDirection(Qt::RightToLeft);events();
        const auto rtlImage = first.grab().toImage();
        bool hasRtlInk = false;
        for (int y = 0; y < rtlImage.height() && !hasRtlInk; ++y)
            for (int x = rtlImage.width() / 2; x < rtlImage.width(); ++x)
                if (distance(rtlImage.pixelColor(x, y), QColor("#6235f7")) < 20) {
                    hasRtlInk = true;
                    break;
                }
        QVERIFY(hasRtlInk);
        QCOMPARE(first.findChildren<QObject*>("figmaRadioRenderer").size(),1);
        for(const auto& value:QStringList{"unselected","selected"})
            for(const auto& state:QStringList{"default","hover","pressed","focus-visible","disabled"}) {
                const QString path=QString(":/figma/radio/%1-%2.svg").arg(value,state);
                QSvgRenderer asset(path);QVERIFY2(asset.isValid(),qPrintable(path));
                const int extent=state=="focus-visible"?22:18;
                QCOMPARE(asset.defaultSize(),QSize(extent,extent));
                if (value == QStringLiteral("selected")) {
                    QFile source(path);QVERIFY(source.open(QIODevice::ReadOnly));
                    const QString xml = QString::fromUtf8(source.readAll());
                    const QRegularExpression ring(
                        QStringLiteral("id=\"Ring\" cx=\"([0-9]+)\" cy=\"([0-9]+)\""));
                    const QRegularExpression dot(
                        QStringLiteral("id=\"Dot\" cx=\"([0-9]+)\" cy=\"([0-9]+)\""));
                    const auto ringMatch = ring.match(xml), dotMatch = dot.match(xml);
                    QVERIFY(ringMatch.hasMatch());QVERIFY(dotMatch.hasMatch());
                    QCOMPARE(dotMatch.captured(1), ringMatch.captured(1));
                    QCOMPARE(dotMatch.captured(2), ringMatch.captured(2));
                }
            }
        QVERIFY_EXCEPTION_THROWN(commonWidget::applyFigmaRadioAppearance(&first,"Active"),std::invalid_argument);
    }
    void capturedLabelKeepsLogicalAlignmentAndWrapping() {
        QWidget host;host.setStyleSheet("QWidget {background:white;}");host.resize(460,100);
        QLabel label("Alignment",&host);label.resize(440,32);
        commonWidget::applyFigmaLabelAppearance(&label,"Start",false);
        QCOMPARE(label.alignment(),Qt::Alignment(Qt::AlignLeft|Qt::AlignVCenter|Qt::AlignAbsolute));
        host.show();events();
        QCOMPARE(QFontInfo(label.font()).family(),QString("HarmonyOS Sans"));
        const auto inkStart=[](const QImage& image) {
            for(int x=0;x<image.width();++x)for(int y=0;y<image.height();++y) {
                const auto color=image.pixelColor(x,y);
                if(color.alpha()>128 && color.red()<70 && color.green()<70 && color.blue()<70)return x;
            }
            return -1;
        };
        QVERIFY(inkStart(label.grab().toImage())<20);
        label.setLayoutDirection(Qt::RightToLeft);events();
        QVERIFY(inkStart(label.grab().toImage())>300);
        commonWidget::applyFigmaLabelAppearance(&label,"End",true);events();
        QVERIFY(inkStart(label.grab().toImage())<20);QVERIFY(label.wordWrap());
        QVERIFY(label.alignment()&Qt::AlignAbsolute);
        QCOMPARE(label.property("figmaLogicalAlignment").toString(),QString("End"));
        QCOMPARE(label.findChildren<QObject*>("figmaLabelDirectionBinding").size(),1);
        label.setLayoutDirection(Qt::LeftToRight);events();
        QVERIFY(inkStart(label.grab().toImage())>300);
        QCOMPARE(label.text(),QString("Alignment"));
        QVERIFY_EXCEPTION_THROWN(commonWidget::applyFigmaLabelAppearance(&label,"Left",false),std::invalid_argument);
        commonWidget::configureFigmaLabel(
            &label, {{"Alignment","Center"},{"Wrap","False"},{"State","Disabled"}});
        QVERIFY(!label.isEnabled());QVERIFY(!label.wordWrap());
        QCOMPARE(label.focusPolicy(),Qt::NoFocus);
        QCOMPARE(label.property("figmaLabelState").toString(),QString("Disabled"));
        QVERIFY_EXCEPTION_THROWN(
            commonWidget::configureFigmaLabel(
                &label, {{"Alignment","Center"},{"Wrap","Sometimes"},{"State","Default"}}),
            std::invalid_argument);
    }
    void capturedProgressKeepsNativeRangeValueAndBusyContract() {
        QWidget host;host.setStyleSheet("QWidget {background:white;}");host.resize(300,240);
        QProgressBar bar(&host);bar.resize(270,24);bar.setRange(0,100);bar.setValue(68);
        bar.setFormat("%p%");
        commonWidget::applyFigmaProgressAppearance(&bar,"Determinate",true);
        host.show();events();
        QCOMPARE(QFontInfo(bar.font()).family(),QString("HarmonyOS Sans"));
        const QColor purple("#6235f7"),rail("#f3f3f3");
        QCOMPARE(bar.grab().toImage().pixelColor(50,12),purple);
        QCOMPARE(bar.grab().toImage().pixelColor(140,12),rail);
        QSignalSpy values(&bar,&QProgressBar::valueChanged);
        bar.setValue(20);events();QCOMPARE(values.count(),1);
        QCOMPARE(bar.grab().toImage().pixelColor(50,12),rail);
        bar.setValue(68);bar.setInvertedAppearance(true);events();
        QCOMPARE(bar.grab().toImage().pixelColor(30,12),rail);
        QCOMPARE(bar.grab().toImage().pixelColor(130,12),purple);
        commonWidget::applyFigmaProgressAppearance(&bar,"Completed",true);events();
        QCOMPARE(bar.value(),68);QCOMPARE(bar.text(),QString("68%"));
        QCOMPARE(bar.grab().toImage().pixelColor(130,12),QColor("#067a3a"));
        bar.setOrientation(Qt::Vertical);bar.resize(32,200);events();
        QCOMPARE(bar.grab().toImage().pixelColor(16,30),purple);
        QCOMPARE(bar.grab().toImage().pixelColor(16,180),QColor(Qt::white));
        bar.setInvertedAppearance(false);events();
        QCOMPARE(bar.grab().toImage().pixelColor(16,180),purple);
        bar.setOrientation(Qt::Horizontal);bar.resize(270,24);bar.setRange(0,0);events();
        QCOMPARE(bar.text(),QString());
        const auto still=bar.grab().toImage();QTest::qWait(100);QCOMPARE(bar.grab().toImage(),still);
        commonWidget::applyFigmaProgressAppearance(&bar,"Indeterminate",false);events();
        const auto moving=bar.grab().toImage();QTest::qWait(150);QVERIFY(bar.grab().toImage()!=moving);
        bar.setProperty("reducedMotion",true);events();
        const auto reduced=bar.grab().toImage();QTest::qWait(100);QCOMPARE(bar.grab().toImage(),reduced);
        bar.setRange(-50,50);bar.setValue(0);bar.setFormat("%v / %m");events();
        QCOMPARE(bar.text(),QString("0 / 100"));
        bar.reset();events();QCOMPARE(bar.text(),QString());
        QCOMPARE(bar.findChildren<QObject*>("figmaProgressRenderer").size(),1);
        QVERIFY_EXCEPTION_THROWN(commonWidget::applyFigmaProgressAppearance(&bar,"Unknown"),std::invalid_argument);
        bar.setRange(-50,50);bar.setValue(0);
        commonWidget::configureFigmaProgressBar(
            &bar, {{"Orientation","Horizontal"},{"State","Indeterminate"},
                   {"Inverted","False"},{"Show label","True"}}, true);
        QCOMPARE(bar.minimum(),0);QCOMPARE(bar.maximum(),0);QVERIFY(!bar.isTextVisible());
        commonWidget::configureFigmaProgressBar(
            &bar, {{"Orientation","Horizontal"},{"State","Paused"},
                   {"Inverted","True"},{"Show label","False"}}, true);
        QCOMPARE(bar.minimum(),-50);QCOMPARE(bar.maximum(),50);QCOMPARE(bar.value(),0);
        QCOMPARE(bar.format(),QString("Paused"));QVERIFY(bar.isTextVisible());
        QCOMPARE(bar.focusPolicy(),Qt::NoFocus);
        QVERIFY_EXCEPTION_THROWN(
            commonWidget::configureFigmaProgressBar(
                &bar, {{"Orientation","Vertical"},{"State","Completed"},
                       {"Inverted","False"},{"Show label","True"}}),
            std::invalid_argument);
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
    void figmaIndeterminateUsesHorizontalDash() {
        Z_CheckBox c(catalog, "");
        c.setAppearance("figma-light");
        c.setChecked(false);
        c.setIndeterminate(true);
        c.resize(c.sizeHint());
        c.show();
        events();
        const auto indicator = c.grab(c.indicatorRect()).toImage();
        QVERIFY(!indicator.isNull());
        const int centerY = indicator.height() / 2;
        int centerLightPixels = 0;
        int upperLightPixels = 0;
        for (int x = 0; x < indicator.width(); ++x) {
            const auto isLight = [](const QColor& color) {
                return color.red() > 220 && color.green() > 220 && color.blue() > 220;
            };
            centerLightPixels += isLight(indicator.pixelColor(x, centerY));
            upperLightPixels += isLight(indicator.pixelColor(x, qMax(0, centerY - 3)));
        }
        QVERIFY2(centerLightPixels >= 8, "Indeterminate mark must contain a visible horizontal dash");
        QVERIFY2(upperLightPixels < centerLightPixels / 2,
                 "Indeterminate mark must be a dash rather than a filled square");
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
        b.show();
        events();
        b.resize(b.sizeHint());
        for (auto appearance : {commonWidget::ButtonAppearance::Filled,
                                commonWidget::ButtonAppearance::Outlined,
                                commonWidget::ButtonAppearance::Dashed,
                                commonWidget::ButtonAppearance::Text,
                                commonWidget::ButtonAppearance::Link})
            for (bool iconOnly : {false, true}) {
                b.setIconOnly(iconOnly);
                b.adjustSize();
                events();
                auto size = b.size();
                b.setAppearance(appearance);
                events();
                QCOMPARE(b.size(), size);
                if (iconOnly)
                    QCOMPARE(b.sizeHint().width(), b.sizeHint().height());
                QSignalSpy clicked(&b, &QAbstractButton::clicked);
                b.setLoading(true);
                events();
                QCOMPARE(b.size(), size);
                b.click();
                QCOMPARE(clicked.count(), 0);
                auto* indicator = b.findChild<QWidget*>("LoadingIndicator");
                QVERIFY(indicator);
                QVERIFY(indicator->isVisible());
                const int firstAngle = indicator->property("spinnerAngle").toInt();
                QTRY_VERIFY_WITH_TIMEOUT(indicator->property("spinnerAngle").toInt() != firstAngle,
                                         200);
                b.setLoading(false);
                events();
                b.click();
                QCOMPARE(clicked.count(), 1);
            }
    }
    void buttonV2AxesRetainSemantics() {
        Z_PushButton b;
        b.setText("Confirm");
        b.show();
        const QMap<commonWidget::ButtonIntent, QString> intents{
            {commonWidget::ButtonIntent::Primary, "primary"},
            {commonWidget::ButtonIntent::Secondary, "secondary"},
            {commonWidget::ButtonIntent::Operation, "operation"},
            {commonWidget::ButtonIntent::Danger, "danger"}};
        const QMap<commonWidget::ButtonAppearance, QString> appearances{
            {commonWidget::ButtonAppearance::Filled, "filled"},
            {commonWidget::ButtonAppearance::Outlined, "outlined"},
            {commonWidget::ButtonAppearance::Dashed, "dashed"},
            {commonWidget::ButtonAppearance::Text, "text"},
            {commonWidget::ButtonAppearance::Link, "link"}};
        for (auto intent = intents.begin(); intent != intents.end(); ++intent)
            for (auto appearance = appearances.begin(); appearance != appearances.end();
                 ++appearance) {
                    b.setIntent(intent.key());
                    b.setAppearance(appearance.key());
                    events();
                    QCOMPARE(b.property("buttonIntent").toString(), intent.value());
                    QCOMPARE(b.property("buttonAppearance").toString(), appearance.value());
                    b.setEnabled(false);
                    events();
                    QCOMPARE(b.property("visualState").toString(), QString("disabled"));
                    b.setEnabled(true);
                }
    }
    void buttonFilledGlyphRetainsContrastRole() {
        Z_PushButton b;
        b.setType(commonWidget::ButtonType::Default);
        b.setText("Confirm");
        b.setLeadingIconSource(":/icons/arrow-left.svg");
        b.setLeadingIconStyle(commonWidget::IconContainerStyle::Filled);
        b.show();
        events();
        for (auto appearance : {commonWidget::ButtonAppearance::Filled,
                                commonWidget::ButtonAppearance::Outlined,
                                commonWidget::ButtonAppearance::Dashed,
                                commonWidget::ButtonAppearance::Text,
                                commonWidget::ButtonAppearance::Link}) {
            b.setAppearance(appearance);
            events();
            auto* glyph = b.findChild<QWidget*>("LEADING")->findChild<QLabel*>("GLYPH");
            QVERIFY(glyph->palette().color(QPalette::WindowText).isValid());
        }
    }
};
int main(int argc, char** argv) {
    QCoreApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
    QCoreApplication::setAttribute(Qt::AA_UseHighDpiPixmaps);
    QApplication app(argc, argv);
    app.setStyle(QStyleFactory::create("Fusion"));
    auto profile = qEnvironmentVariable("DEMO_PROFILE", "standard-light");
    button_demo::install(app, profile);
    catalog = std::make_shared<surface::Catalog>(profile);
    ContractTests tests;
    return QTest::qExec(&tests, argc, argv);
}
#include "ContractTests.moc"
