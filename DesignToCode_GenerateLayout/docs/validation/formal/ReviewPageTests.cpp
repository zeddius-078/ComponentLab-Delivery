// 独立业务调用方测试：只通过生成页面的公开对象接入行为和局部定制。
#include "ReviewPage.h"
#include "ReviewResultPage.h"
#include "HorizontalReviewPage.h"
#include <ComponentTheme.h>
#include <QApplication>
#include <QFile>
#include <QFontDatabase>
#include <QLineEdit>
#include <QSignalSpy>
#include <QTest>
#include <QDir>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <stdexcept>

class CustomizedReview : public ReviewPage {
public:
    CustomizedReview() {
        getConfirmReviewButton()->setToolTip(QStringLiteral("Business-owned action"));
    }
};

class ReviewPageTests : public QObject {
    Q_OBJECT
private slots:
    void initTestCase() {
        component_theme::install(*qApp, "wide-light");
        const auto font = qEnvironmentVariable("FORMAL_TEST_FONT");
        if (!font.isEmpty()) QVERIFY(QFontDatabase::addApplicationFont(font) >= 0);
    }
    void publicContractAndGeometry() {
        CustomizedReview page;
        QFile style(QStringLiteral(FORMAL_QSS));
        QVERIFY(style.open(QIODevice::ReadOnly));
        page.setStyleSheet(QString::fromUtf8(style.readAll()));
        page.show();
        QTest::qWait(100);
        QCOMPARE(page.getReviewPage(), static_cast<QWidget*>(&page));
        QCOMPARE(page.getReviewPageLayout()->alignment(), Qt::Alignment(Qt::AlignBottom));
        QCOMPARE(page.getReviewPageLayout()->spacing(), 20);
        QCOMPARE(page.getReviewPageLayout()->count(), 5);
        QVERIFY(page.getSafetyConfirmation()->isChecked());
        QVERIFY(!page.getWithdrawButton()->isEnabled());
        QCOMPARE(page.getConfirmReviewButton()->toolTip(), QStringLiteral("Business-owned action"));
        for (int i = 0; i < page.getReviewPageLayout()->count(); ++i) {
            auto* item = page.getReviewPageLayout()->itemAt(i);
            QVERIFY(!item->spacerItem());
            QVERIFY(page.rect().contains(item->widget()->geometry()));
            QVERIFY(!item->widget()->property("designNodeId").isValid());
        }
        const auto height = page.getWithdrawButton()->height();
        QCOMPARE(page.getWithdrawButton()->geometry().bottom(), page.height() - 32 - 1);
        const auto initialReport = qEnvironmentVariable("FORMAL_TEST_REPORT");
        if (!initialReport.isEmpty()) {
            QVERIFY(QDir().mkpath(initialReport));
            QVERIFY(page.grab().save(initialReport + "/reviewPage.png"));
        }
        page.resize(760, 680);
        QTest::qWait(50);
        QCOMPARE(page.getWithdrawButton()->geometry().bottom(), page.height() - 32 - 1);
        QCOMPARE(page.getWithdrawButton()->height(), height);
        const auto directory = qEnvironmentVariable("FORMAL_TEST_REPORT");
        if (!directory.isEmpty()) {
            QVERIFY(QDir().mkpath(directory));
            QVERIFY(page.grab().save(directory + "/review-compact.png"));
        }
    }
    void businessSignalsAndDisabledState() {
        ReviewPage page;
        page.show();
        QTest::qWait(50);
        auto* input = page.getWorkOrderInput();
        QSignalSpy changes(input, &commonWidget::Z_InputField::valueChanged);
        QTest::keyClicks(input->lineEdit(), "WO-2026");
        QCOMPARE(input->value(), QStringLiteral("WO-2026"));
        QVERIFY(changes.count() > 0);
        QString submitted;
        connect(page.getConfirmReviewButton(), &QPushButton::clicked, &page,
                [&] { submitted = input->value(); });
        QTest::mouseClick(page.getConfirmReviewButton(), Qt::LeftButton);
        QCOMPARE(submitted, QStringLiteral("WO-2026"));
        QSignalSpy disabled(page.getWithdrawButton(), &QPushButton::clicked);
        QTest::mouseClick(page.getWithdrawButton(), Qt::LeftButton);
        QCOMPARE(disabled.count(), 0);
    }
    void businessLocalStyleOverride() {
        CustomizedReview page;
        QFile style(QStringLiteral(FORMAL_QSS));
        QVERIFY(style.open(QIODevice::ReadOnly));
        page.setStyleSheet(QString::fromUtf8(style.readAll()));
        page.show(); QTest::qWait(50);
        auto* heading = page.getReviewHeading();
        const auto original = heading->palette().color(QPalette::WindowText);
        const auto geometry = heading->geometry();
        // 业务副本可通过公开对象局部覆盖；清除覆盖后恢复页面 QSS，不修改生成文件。
        heading->setStyleSheet(QStringLiteral("color: #123456;"));
        QTest::qWait(20);
        QCOMPARE(heading->palette().color(QPalette::WindowText), QColor("#123456"));
        QCOMPARE(heading->geometry(), geometry);
        heading->setStyleSheet(QString{});
        QTest::qWait(20);
        QCOMPARE(heading->palette().color(QPalette::WindowText), original);
        QCOMPARE(heading->geometry(), geometry);
    }
    void nestedLayoutsAndPublicOptions() {
        ReviewResultPage result;
        HorizontalReviewPage horizontal;
        const QList<QPair<QWidget*, QString>> pages{{&result, QStringLiteral(FORMAL_RESULT_QSS)},
                                                   {&horizontal, QStringLiteral(FORMAL_HORIZONTAL_QSS)}};
        for (const auto& pair : pages) {
            auto* page = pair.first;
            QFile style(pair.second);
            QVERIFY(style.open(QIODevice::ReadOnly));
            page->setStyleSheet(QString::fromUtf8(style.readAll()));
            page->show();
            QTest::qWait(100);
            for (auto* layout : page->findChildren<QBoxLayout*>()) {
                if (!layout->objectName().endsWith("Layout")) continue;
                for (int i = 0; i < layout->count(); ++i) {
                    auto* item = layout->itemAt(i);
                    QVERIFY(!item->spacerItem());
                    if (auto* widget = item->widget()) {
                        const QRect rect(widget->mapTo(page, QPoint()), widget->size());
                        QVERIFY2(page->rect().contains(rect), qPrintable(widget->objectName()));
                    }
                }
            }
            const auto directory = qEnvironmentVariable("FORMAL_TEST_REPORT");
            if (!directory.isEmpty()) QVERIFY(page->grab().save(directory + "/" + page->objectName() + ".png"));
        }
        QCOMPARE(result.getResultActionsLayout()->direction(), QBoxLayout::LeftToRight);
        QCOMPARE(horizontal.getHorizontalReviewPageLayout()->direction(), QBoxLayout::LeftToRight);
        QCOMPARE(horizontal.getHorizontalWorkOrderFormLayout()->direction(), QBoxLayout::TopToBottom);
        QVERIFY(!result.getSubmitDisabledButton()->isEnabled());
        QCOMPARE(result.getChamberOptionOne()->controlSize(), QStringLiteral("md"));
        QVERIFY(!result.getChamberOptionOne()->text().isEmpty());
        result.getChamberOptionOne()->setText(QStringLiteral("Business chamber"));
        QCOMPARE(result.getChamberOptionOne()->text(), QStringLiteral("Business chamber"));
        QSignalSpy back(result.getReturnButton(), &QPushButton::clicked);
        QTest::mouseClick(result.getReturnButton(), Qt::LeftButton);
        QCOMPARE(back.count(), 1);
        QSignalSpy horizontalBack(horizontal.getHorizontalReturnButton(), &QPushButton::clicked);
        QTest::mouseClick(horizontal.getHorizontalReturnButton(), Qt::LeftButton);
        QCOMPARE(horizontalBack.count(), 1);
    }
    void compareIndependentDesignGeometry() {
        const auto path = qEnvironmentVariable("FORMAL_GEOMETRY_REFERENCE");
        if (path.isEmpty()) QSKIP("Independent Pixso geometry was not supplied");
        QFile source(path);
        QVERIFY(source.open(QIODevice::ReadOnly));
        const auto reference = QJsonDocument::fromJson(source.readAll()).object();
        QCOMPARE(reference["fileKey"].toString(), QStringLiteral("4S0Ktm81WnvzuJmZHvRuOg"));
        ReviewPage review;
        ReviewResultPage result;
        HorizontalReviewPage horizontal;
        const QMap<QString, QPair<QWidget*, QString>> pages{
            {"19:45", {&review, QStringLiteral(FORMAL_QSS)}},
            {"21:436", {&result, QStringLiteral(FORMAL_RESULT_QSS)}},
            {"27:450", {&horizontal, QStringLiteral(FORMAL_HORIZONTAL_QSS)}}};
        QJsonArray checks;
        int mismatches = 0;
        for (const auto& pageValue : reference["pages"].toArray()) {
            const auto ref = pageValue.toObject();
            QVERIFY(pages.contains(ref["id"].toString()));
            const auto pair = pages[ref["id"].toString()];
            auto* page = pair.first;
            QFile style(pair.second); QVERIFY(style.open(QIODevice::ReadOnly));
            page->setStyleSheet(QString::fromUtf8(style.readAll()));
            const auto size = ref["size"].toArray();
            page->resize(size[0].toInt(), size[1].toInt());
            page->show(); QTest::qWait(100);
            for (const auto& nodeValue : ref["nodes"].toArray()) {
                const auto node = nodeValue.toObject();
                const auto name = node["engineeringName"].toString();
                QRect actual;
                if (name == page->objectName()) actual = page->rect();
                else if (auto* widget = page->findChild<QWidget*>(name))
                    actual = QRect(widget->mapTo(page, QPoint()), widget->size());
                else if (auto* layout = page->findChild<QLayout*>(name + "Layout"))
                    actual = layout->geometry().translated(layout->parentWidget()->mapTo(page, QPoint()));
                else QFAIL(qPrintable("Missing public object: " + name));
                const auto r = node["rect"].toArray();
                const QRect expected(qRound(r[0].toDouble()), qRound(r[1].toDouble()),
                                     qRound(r[2].toDouble()), qRound(r[3].toDouble()));
                const bool equal = actual == expected;
                if (!equal) ++mismatches;
                checks.append(QJsonObject{{"name", name}, {"equal", equal},
                    {"expected", r}, {"actual", QJsonArray{actual.x(), actual.y(), actual.width(), actual.height()}}});
            }
        }
        const auto report = qEnvironmentVariable("FORMAL_TEST_REPORT");
        if (!report.isEmpty()) {
            QFile output(report + "/design-geometry.json");
            QVERIFY(output.open(QIODevice::WriteOnly));
            output.write(QJsonDocument(QJsonObject{{"mismatches", mismatches}, {"checks", checks}}).toJson());
        }
        QCOMPARE(checks.size(), 34);
        QCOMPARE(mismatches, 0);
    }
#ifdef FORMAL_RESULT_PAGE_ASSET
    void pageResourceLoadedFromCompiledQrc() {
        ReviewResultPage page;
        const QString source = QStringLiteral(":/formal-fixture/result/page-marker.svg");
        QCOMPARE(page.getReturnButton()->leadingIconSource(), source);
        QFile file(source);
        QVERIFY(file.open(QIODevice::ReadOnly));
        QVERIFY(file.readAll().contains("2454C6"));
        const QPixmap image(source);
        QVERIFY(!image.isNull());
        QCOMPARE(image.size(), QSize(16, 16));
        QCOMPARE(image.toImage().pixelColor(8, 8), QColor("#2454C6"));
        // 业务局部替换使用公开接口，不改平台资源或重新写生成器。
        page.getReturnButton()->setLeadingIconSource(":/icons/arrow-right.svg");
        QCOMPARE(page.getReturnButton()->leadingIconSource(), QStringLiteral(":/icons/arrow-right.svg"));
    }
#endif
};
QTEST_MAIN(ReviewPageTests)
#include "ReviewPageTests.moc"
