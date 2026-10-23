#include "BaselineGallery.h"

#include <MyDesignSystem/MyDesignSystem.h>

#include <QApplication>
#include <QImage>
#include <QSet>
#include <QStackedWidget>
#include <QStyle>
#include <QStyleOptionButton>
#include <QTabWidget>
#include <QTreeWidget>
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

    void galleryDisplaysOnePublishedComponentPerPage() {
        commonWidget::initializeNativeComponentResources();
        qApp->setStyleSheet(commonWidget::nativeComponentStyleSheet() +
                            baselineGalleryCanvasStyleSheet());
        BaselineWindow window;
        QCOMPARE(window.componentSectionCount(), 9);
        QCOMPARE(window.componentPageCount(), 59);
        QVERIFY(window.hasObjectInspector());
        QVERIFY(!baselineGalleryCanvasStyleSheet().isEmpty());
        QVERIFY(window.findChild<QTabWidget*>(QStringLiteral("baselineSdkTabs")));
        QVERIFY(window.findChild<QTabWidget*>(QStringLiteral("baselineGalleryTabs")));
        auto* navigation = window.findChild<QTreeWidget*>(QStringLiteral("baselineNavigation"));
        QVERIFY(navigation);
        QCOMPARE(navigation->topLevelItemCount(), 1);
        auto* published = navigation->topLevelItem(0);
        QCOMPARE(published->childCount(), 9);
        auto* pages = window.findChild<QStackedWidget*>(QStringLiteral("baselineGalleryPages"));
        QVERIFY(pages);
        QCOMPARE(pages->count(), 59);
        int leafCount = 0;
        QSet<int> pageIndices;
        QTreeWidgetItem* checkboxLeaf = nullptr;
        for (int categoryIndex = 0; categoryIndex < published->childCount(); ++categoryIndex) {
            auto* category = published->child(categoryIndex);
            for (int componentIndex = 0; componentIndex < category->childCount(); ++componentIndex) {
                auto* leaf = category->child(componentIndex);
                QVERIFY(leaf->text(0).startsWith(QStringLiteral("Z_")));
                const int pageIndex = leaf->data(0, Qt::UserRole).toInt();
                QVERIFY(pageIndex >= 0 && pageIndex < pages->count());
                QVERIFY(!pageIndices.contains(pageIndex));
                pageIndices.insert(pageIndex);
                const QString qtClass = leaf->data(0, Qt::UserRole + 1).toString();
                if (qtClass == QStringLiteral("QCheckBox"))
                    checkboxLeaf = leaf;
                const auto roots = pages->widget(pageIndex)->findChildren<QWidget*>();
                int matching = 0;
                for (auto* root : roots) {
                    if (!root->objectName().startsWith(QStringLiteral("componentRoot_")))
                        continue;
                    QCOMPARE(root->objectName(), QStringLiteral("componentRoot_%1").arg(qtClass));
                    ++matching;
                }
                if (qtClass != QStringLiteral("QOpenGLWidget"))
                    QVERIFY2(matching > 0, "Page must contain its own SDK component instance");
                ++leafCount;
            }
        }
        QCOMPARE(leafCount, 59);
        QVERIFY(checkboxLeaf);
        navigation->setCurrentItem(checkboxLeaf);
        QCOMPARE(pages->currentIndex(), checkboxLeaf->data(0, Qt::UserRole).toInt());
        auto* selectedTabs = pages->currentWidget()->findChild<QTabWidget*>(
            QStringLiteral("baselineComponentTabs"));
        QVERIFY(selectedTabs);
        QCOMPARE(selectedTabs->count(), 2);
        selectedTabs->setCurrentIndex(1);
        QVERIFY(pages->currentWidget()->findChild<commonWidget::Z_CheckBox*>(
            QStringLiteral("baselineIndeterminateCheckbox")));
        auto* inspector = window.findChild<QTreeWidget*>(QStringLiteral("baselineObjectInspector"));
        QVERIFY(inspector);
        QCOMPARE(inspector->columnCount(), 2);
        QCOMPARE(inspector->topLevelItem(0)->text(1),
                 QStringLiteral("commonWidget::Z_CheckBox"));
        QVERIFY(window.findChild<commonWidget::Z_PushButton*>(
            QStringLiteral("componentRoot_QPushButton")));
        QVERIFY(window.findChild<commonWidget::Z_CheckBox*>(
            QStringLiteral("baselineIndeterminateCheckbox")));
    }
};

int main(int argc, char** argv) {
    QApplication app(argc, argv);
    BaselineConsumerTests tests;
    return QTest::qExec(&tests, argc, argv);
}

#include "BaselineConsumerTests.moc"
