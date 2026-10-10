#include "ui_SelectOptionFixture.h"
#include "ComponentTheme.h"
#include "DesignerWidgets.h"
#include <QListView>
#include <QTest>

class SelectOptionTests : public QObject {
    Q_OBJECT
private slots:
    void compactUicAndDisabledItem() {
        component_theme::install(*qApp, "light-compact");
        designer_widgets::ThemeHost page("light-compact");
        Ui::SelectOptionFixture ui;
        ui.setupUi(&page);
        QCOMPARE(ui.optionItem->text(), QString("ETCH-01"));
        QCOMPARE(ui.optionItem->controlSize(), QString("md"));
        const field::Tokens expected("light-compact");
        QCOMPARE(ui.optionItem->sizeHint().height(), expected.metric("optionHeight"));
        page.show();
        QTest::qWait(50);
        // 平台测试核对内部模型禁用状态；业务交付只依赖上面的公开属性。
        auto *view = ui.optionItem->findChild<QListView*>(QString(), Qt::FindDirectChildrenOnly);
        QVERIFY(view && view->model());
        QCOMPARE(view->model()->rowCount(), 1);
        auto index = view->model()->index(0, 0);
        QVERIFY(!(view->model()->flags(index) & Qt::ItemIsEnabled));
        QTest::mouseClick(view->viewport(), Qt::LeftButton, Qt::NoModifier,
                          view->visualRect(index).center());
        QVERIFY(view->selectionModel()->selectedIndexes().isEmpty());
        ui.optionItem->setText("ETCH-02");
        QCOMPARE(view->model()->index(0, 0).data().toString(), QString("ETCH-02"));
    }

    void missingThemeIsRejected() {
        QWidget plainParent;
        QVERIFY_EXCEPTION_THROWN(designer_widgets::SelectOption item(&plainParent), std::logic_error);
    }
};
QTEST_MAIN(SelectOptionTests)
#include "SelectOptionTests.moc"
