#include <MyDesignSystem/MyDesignSystem.h>

#include <QApplication>
#include <QImage>
#include <QStyle>
#include <QStyleOptionButton>
#include <QtTest>

class BaselineConsumerTests final : public QObject {
    Q_OBJECT

  private slots:
    void allPublishedWrappersUseThePublicSdkContract() {
        QWidget owner;
#define VERIFY_NATIVE_WRAPPER(QT_CLASS, SDK_CLASS) \
    { \
        commonWidget::SDK_CLASS component(&owner); \
        QCOMPARE(component.property("nativeQtClass").toString(), QStringLiteral(#QT_CLASS)); \
        QCOMPARE(component.property("designSystemClass").toString(), \
                 QStringLiteral("commonWidget::" #SDK_CLASS)); \
    }
        Z_NATIVE_COMPONENT_TYPES(VERIFY_NATIVE_WRAPPER)
#undef VERIFY_NATIVE_WRAPPER
    }

    void qssAndIndeterminateStateAreOwnedByTheSdk() {
        commonWidget::initializeNativeComponentResources();
        const QString qss = commonWidget::nativeComponentStyleSheet();
        QVERIFY(qss.contains(QStringLiteral("HarmonyOS Sans")));
        QVERIFY(qss.contains(QStringLiteral("indicator:indeterminate")));
        qApp->setStyleSheet(qss);
        commonWidget::Z_CheckBox checkbox;
        checkbox.setTristate(true);
        checkbox.setCheckState(Qt::PartiallyChecked);
        QCOMPARE(checkbox.checkState(), Qt::PartiallyChecked);
        QVERIFY(!qApp->styleSheet().isEmpty());
        checkbox.resize(160, 32);
        checkbox.show();
        QTest::qWait(20);
        QStyleOptionButton option;
        option.initFrom(&checkbox);
        option.state |= QStyle::State_NoChange;
        const QRect indicator = checkbox.style()->subElementRect(
            QStyle::SE_CheckBoxIndicator, &option, &checkbox);
        const QImage rendered = checkbox.grab().toImage();
        const QColor center = rendered.pixelColor(indicator.center());
        const QColor fill = rendered.pixelColor(indicator.left() + 2, indicator.center().y());
        // The middle of the indeterminate indicator is the SDK-owned white
        // horizontal dash, not an invisible white square or the purple fill.
        QVERIFY2(center.red() > 220 && center.green() > 220 && center.blue() > 220,
                 "Indeterminate checkbox must render its white horizontal dash");
        QVERIFY2(fill.red() > 70 && fill.red() < 130 && fill.green() < 90 && fill.blue() > 220,
                 "Indeterminate checkbox must use the SDK purple fill");
    }

    void inheritedQtConstructorsReceiveIdentityBeforePainting() {
        commonWidget::initializeNativeComponentResources();
        qApp->setStyleSheet(commonWidget::nativeComponentStyleSheet());
        commonWidget::Z_CheckBox checkbox(QStringLiteral("Inherited Qt constructor"));
        // The identity is established by the forwarding constructor itself,
        // not deferred to show(), polish(), or an event-loop side effect.
        QCOMPARE(checkbox.property("designSystemClass").toString(),
                 QStringLiteral("commonWidget::Z_CheckBox"));
        checkbox.setTristate(true);
        checkbox.setCheckState(Qt::PartiallyChecked);
        checkbox.resize(220, 32);
        checkbox.show();
        QTest::qWait(20);
        QStyleOptionButton option;
        option.initFrom(&checkbox);
        option.state |= QStyle::State_NoChange;
        const QRect indicator = checkbox.style()->subElementRect(
            QStyle::SE_CheckBoxIndicator, &option, &checkbox);
        const QColor center = checkbox.grab().toImage().pixelColor(indicator.center());
        const QColor fill = checkbox.grab().toImage().pixelColor(
            indicator.left() + 2, indicator.center().y());
        QVERIFY2(center.red() > 220 && center.green() > 220 && center.blue() > 220,
                 "Inherited constructor must receive the QSS indeterminate dash");
        QVERIFY2(fill.red() > 70 && fill.red() < 130 && fill.green() < 90 && fill.blue() > 220,
                 "Inherited constructor must receive the SDK purple fill");
    }
};

int main(int argc, char** argv) {
    QApplication app(argc, argv);
    BaselineConsumerTests tests;
    return QTest::qExec(&tests, argc, argv);
}

#include "BaselineConsumerTests.moc"
