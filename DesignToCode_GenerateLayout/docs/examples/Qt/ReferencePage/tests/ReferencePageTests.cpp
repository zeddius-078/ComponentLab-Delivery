#include "ComponentTheme.h"
#include "ReferencePage.h"
#include "Z_PushButton.h"
#include <QApplication>
#include <QDir>
#include <QLabel>
#include <QLayout>
#include <QLineEdit>
#include <QListView>
#include <QSignalSpy>
#include <QtTest>
#include <stdexcept>

using namespace commonWidget;

class ReferencePageTests : public QObject {
    Q_OBJECT
private:
    QString profile = qEnvironmentVariable("REFERENCE_PROFILE", "standard-light");

    void populate(ReferencePage &page) {
        page.setChambers({{"etch-01", "刻蚀腔体 ETCH-01", {}, false},
                          {"etch-02", "刻蚀腔体 ETCH-02", {}, false},
                          {"etch-03", "维护中 ETCH-03", {}, true}});
        auto fields = page.fields();
        fields.chamberIds = QStringList{"etch-01"};
        page.setFields(fields);
    }

private slots:
    void initTestCase() { component_theme::install(*qApp, profile); }

    void realWidgetsAndStaticProperties() {
        ReferencePage page(profile);
        const auto *input = page.findChild<Z_InputField *>("lotInput");
        const auto *select = page.findChild<Z_Select *>("chamberSelect");
        const auto *check = page.findChild<Z_CheckBox *>("qualityCheck");
        const auto *submit = page.findChild<Z_PushButton *>("submitButton");
        const auto *search = page.findChild<Z_PushButton *>("searchButton");
        QVERIFY(input && select && check && submit && search);
        QCOMPARE(input->value(), QString("LOT-2026-0917"));
        QCOMPARE(input->lineEdit()->placeholderText(), QString("输入批次编号"));
        QCOMPARE(check->text(), QString("启用质量检查"));
        QVERIFY(check->isChecked());
        QCOMPARE(submit->text(), QString("确认操作"));
        QVERIFY(submit->isEnabled());
        QVERIFY(search->isIconOnly());
        QCOMPARE(search->accessibleName(), QString("查询批次"));
        QCOMPARE(submit->trailingIconSource(), QString(":/icons/arrow-right.svg"));
        QCOMPARE(page.findChild<QLabel *>("lotLabel")->buddy(), input);
        QCOMPARE(page.findChild<QLabel *>("chamberLabel")->buddy(), select);
        QCOMPARE(input->focusProxy(), input->lineEdit());
        QVERIFY(qobject_cast<QLineEdit *>(select->focusProxy()));
    }

    void themeContextIsRequired() {
        QWidget unthemed;
        QVERIFY_EXCEPTION_THROWN(designer_widgets::InputField{&unthemed}, std::logic_error);
        QVERIFY_EXCEPTION_THROWN(designer_widgets::CheckBox{&unthemed}, std::logic_error);
        QVERIFY_EXCEPTION_THROWN(designer_widgets::Select{&unthemed}, std::logic_error);
        QVERIFY_EXCEPTION_THROWN(ReferencePage("unknown-profile"), std::logic_error);
    }

    void fieldsAreTypedAndAtomic() {
        ReferencePage page(profile);
        populate(page);
        QSignalSpy spy(&page, &ReferencePage::fieldsChanged);
        page.setFields({"LOT-42", QStringList{"etch-02"}, false});
        QCOMPARE(spy.count(), 1);
        const auto fields = page.fields();
        QCOMPARE(fields.lotNumber, QString("LOT-42"));
        QCOMPARE(fields.chamberIds, QStringList{"etch-02"});
        QVERIFY(!fields.qualityCheck);
    }

    void actionsCarryFields() {
        ReferencePage page(profile);
        populate(page);
        page.show();
        QSignalSpy submit(&page, &ReferencePage::submitRequested);
        QSignalSpy search(&page, &ReferencePage::searchRequested);
        QTest::mouseClick(page.findChild<Z_PushButton *>("submitButton"), Qt::LeftButton);
        QTest::mouseClick(page.findChild<Z_PushButton *>("searchButton"), Qt::LeftButton);
        QCOMPARE(submit.count(), 1);
        QCOMPARE(search.count(), 1);
        QCOMPARE(submit.first()[0].toString(), page.fields().lotNumber);
        QCOMPARE(submit.first()[1].toStringList(), page.fields().chamberIds);
        QCOMPARE(submit.first()[2].toBool(), page.fields().qualityCheck);
        QCOMPARE(search.first()[0].toString(), page.fields().lotNumber);
    }

    void busySuppressesActionsAndRestores() {
        ReferencePage page(profile);
        populate(page);
        auto *submit = page.findChild<Z_PushButton *>("submitButton");
        auto *search = page.findChild<Z_PushButton *>("searchButton");
        QSignalSpy spy(&page, &ReferencePage::submitRequested);
        page.setBusy(true);
        QVERIFY(submit->property("loading").toBool());
        QVERIFY(!search->isEnabled());
        QVERIFY(page.findChild<Z_InputField *>("lotInput")->lineEdit()->isReadOnly());
        QVERIFY(!page.findChild<Z_CheckBox *>("qualityCheck")->isEnabled());
        submit->click();
        QCOMPARE(spy.count(), 0);
        page.setBusy(false);
        QVERIFY(!submit->property("loading").toBool());
        QVERIFY(search->isEnabled());
        submit->click();
        QCOMPARE(spy.count(), 1);
    }

    void errorUsesComponentState() {
        ReferencePage page(profile);
        auto *input = page.findChild<Z_InputField *>("lotInput");
        page.setLotError("批次编号无效");
        QCOMPARE(input->control()->visualState(), QString("error"));
        QCOMPARE(input->lineEdit()->accessibleDescription(), QString("批次编号无效"));
        page.setLotError({});
        QVERIFY(input->control()->visualState() != "error");
        QVERIFY(page.styleSheet().isEmpty());
    }

    void disabledOptionsRemainDisabled() {
        ReferencePage page(profile);
        populate(page);
        auto *select = page.findChild<Z_Select *>("chamberSelect");
        const auto *model = select->optionView()->model();
        QCOMPARE(model->rowCount(), 3);
        QVERIFY(model->flags(model->index(0, 0)) & Qt::ItemIsEnabled);
        QVERIFY(!(model->flags(model->index(2, 0)) & Qt::ItemIsEnabled));
    }

    void keyboardUsesOriginalComponentBehavior() {
        ReferencePage page(profile);
        populate(page);
        page.show();
        auto *select = page.findChild<Z_Select *>("chamberSelect");
        auto *check = page.findChild<Z_CheckBox *>("qualityCheck");
        QTest::keyClick(select->focusProxy(), Qt::Key_Down);
        QVERIFY(select->isOpen());
        QTest::keyClick(select->searchEdit(), Qt::Key_Escape);
        QVERIFY(!select->isOpen());
        QTest::keyClick(check, Qt::Key_Space);
        QVERIFY(!page.fields().qualityCheck);
        // 忙碌期间字段锁定，键盘路径与鼠标路径使用同一套组件状态。
        page.setBusy(true);
        QTest::keyClick(select->focusProxy(), Qt::Key_Down);
        QVERIFY(!select->isOpen());
    }

    void layoutsResizeAndCapture() {
        ReferencePage page(profile);
        populate(page);
        const auto destination = qEnvironmentVariable("REFERENCE_EVIDENCE");
        QVERIFY(!destination.isEmpty());
        QVERIFY(QDir().mkpath(destination));
        page.show();
        // 小窗口和宽窗口分别检查页面层的几何包含关系；内部图标画布继续采用组件尺寸。
        for (const QSize size : {QSize(420, 360), QSize(960, 540)}) {
            page.resize(size);
            QCoreApplication::processEvents();
            // 控件必须完整位于页面内，并满足原组件声明的最小尺寸。
            for (const auto *name :
                 {"lotInput", "chamberSelect", "qualityCheck", "submitButton", "searchButton"}) {
                const auto *widget = page.findChild<QWidget *>(name);
                QVERIFY(widget);
                const QRect geometry(widget->mapTo(&page, QPoint()), widget->size());
                QVERIFY2(page.rect().contains(geometry), name);
                QVERIFY(widget->width() >= widget->minimumSizeHint().width());
                QVERIFY(widget->height() >= widget->minimumSizeHint().height());
            }
            QVERIFY(page.grab().save(destination + "/" + QString::number(size.width()) + ".png"));
        }
        auto fields = page.fields();
        fields.lotNumber = QString(160, 'X');
        page.setFields(fields);
        page.setLotError("批次编号过长，请核对生产记录中的编号后重新输入。");
        page.resize(420, 440);
        QCoreApplication::processEvents();
        QCOMPARE(page.fields().lotNumber.size(), 160);
        QVERIFY(page.grab().save(destination + "/long-text-error.png"));
    }
};

QTEST_MAIN(ReferencePageTests)
#include "ReferencePageTests.moc"
