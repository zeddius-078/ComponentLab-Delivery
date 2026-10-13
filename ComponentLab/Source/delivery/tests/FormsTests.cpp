#include "DemoRuntime.h"
#include "Z_InputField.h"
#include "Z_Select.h"
#include "Z_OtpInput.h"
#include <QClipboard>
#include <QApplication>
#include <QDir>
#include <QInputMethodEvent>
#include <QLabel>
#include <QLineEdit>
#include <QListView>
#include <QPlainTextEdit>
#include <QPointer>
#include <QScreen>
#include <QScrollBar>
#include <QSignalSpy>
#include <QStyleFactory>
#include <QTest>
#include <QAbstractButton>
#include <QVBoxLayout>
#include <QStackedWidget>
#include <QSvgRenderer>
#include <QTextBlock>
#include <QFontInfo>
#include <QFontDatabase>
using commonWidget::Z_InputField;
using commonWidget::Z_Select;

class FormsTests : public QObject {
    Q_OBJECT
    std::shared_ptr<const field::Tokens> tokens;
    QString evidence;
    QVector<Z_Select::Option> options() {
        return {{"a", "Alpha", "Group A", false},
                {"b", "Beta", "Group A", true},
                {"c", "Gamma", "Group B", false},
                {"d", "Delta", "Group B", false}};
    }
    void show(QWidget& w, int width = 420) {
        w.resize(width, w.sizeHint().height());
        w.show();
        w.activateWindow();
        QTest::qWait(40);
    }
  private slots:
    void initTestCase() {
        const auto profile = qEnvironmentVariable("DEMO_PROFILE", "light-comfortable");
        tokens = std::make_shared<field::Tokens>(profile);
        evidence = qEnvironmentVariable("DEMO_EVIDENCE");
        if (!evidence.isEmpty())
            QVERIFY(QDir().mkpath(evidence));
    }
    void inputGeometryAndTree() {
        Z_InputField w(tokens);
        w.setLabel("Recipe");
        w.setHint("Batch A");
        w.setAllowClear(true);
        w.setValue("Sample");
        show(w);
        for (auto size : {"sm", "md", "lg"}) {
            w.setControlSize(size);
            QTest::qWait(10);
            const auto m = tokens->size(size);
            QVERIFY(w.control()->height() >= m["height"].toInt());
            QVERIFY(w.control()->height() >= 32);
            QCOMPARE(w.lineEdit()->font().pixelSize(), m["fontSize"].toInt());
            QVERIFY(w.control()->rect().contains(w.lineEdit()->geometry()));
        }
        for (auto name : {"LABEL", "CONTROL", "PREFIX", "SUFFIX", "EDITOR", "TEXTAREA", "CLEAR",
                          "PASSWORD_TOGGLE", "SEARCH_ACTION", "FOOTER", "HINT", "COUNT"})
            QVERIFY2(w.findChild<QWidget*>(name), name);
    }
    void capturedInputPreservesEditorsAndNativeContracts() {
        Z_InputField w(tokens);w.setAppearance("figma-light");w.setSnapshotState("Default");
        auto* line=w.lineEdit();auto* area=w.textArea();
        w.setPlaceholder("Input text");show(w,320);
        for(const auto& size:QStringList{"sm","md","lg"}) {
            w.setControlSize(size);QCoreApplication::processEvents();
            QCOMPARE(w.control()->height(),size=="lg"?40:32);
            QCOMPARE(line->height(),20);QCOMPARE(line->font().pixelSize(),14);
            QCOMPARE(line->x(),size=="sm"?9:size=="md"?13:17);
        }
        w.setValue("ABCDE");line->setSelection(1,3);w.setSnapshotState("Hover");
        QCOMPARE(w.lineEdit(),line);QCOMPARE(w.textArea(),area);QCOMPARE(line->selectedText(),QString("BCD"));
        w.setKind(Z_InputField::Kind::Password);w.setPasswordVisible(true);
        QCOMPARE(line->echoMode(),QLineEdit::Normal);QCOMPARE(w.value(),QString("ABCDE"));
        auto* action=w.findChild<field::Action*>("PASSWORD_TOGGLE");
        w.setSnapshotState("Focus");QCOMPARE(action->property("figmaInputGlyph").toString(),QString("EyeOffAccent"));
        QCOMPARE(action->size(),QSize(32,32));QCOMPARE(action->property("figmaAssetBounds").toRect(),QRect(9,9,16,16));
        action->click();QCOMPARE(line->echoMode(),QLineEdit::Password);
        w.setSnapshotState("Error");QCOMPARE(action->property("figmaInputGlyph").toString(),QString("EyeDanger"));
        w.setKind(Z_InputField::Kind::Search);w.setAllowClear(true);
        QSignalSpy searches(&w,&Z_InputField::searchRequested);
        w.setLoading(true);w.findChild<field::Action*>("SEARCH_ACTION")->click();QCOMPARE(searches.count(),0);
        QCOMPARE(w.findChild<field::Action*>("SEARCH_ACTION")->property("figmaInputGlyph").toString(),QString("LoadingPrimary"));
        w.setLoading(false);w.findChild<field::Action*>("SEARCH_ACTION")->click();QCOMPARE(searches.count(),1);
        w.setControlled(true);QSignalSpy requests(&w,&Z_InputField::valueChangeRequested);
        w.findChild<field::Action*>("CLEAR")->click();QCOMPARE(requests.count(),1);QCOMPARE(w.value(),QString("ABCDE"));
        w.setControlled(false);w.findChild<field::Action*>("CLEAR")->click();QVERIFY(w.value().isEmpty());
        w.setKind(Z_InputField::Kind::TextArea);w.setAutoSize(false,2,5);
        QCOMPARE(w.control()->height(),54);w.setControlSize("sm");QCOMPARE(w.control()->height(),46);
        w.setSnapshotState("Default");w.setPlaceholder("Describe the context...");
        QCoreApplication::processEvents();
        QVERIFY(w.value().isEmpty());QVERIFY(area->document()->isEmpty());
        QCOMPARE(area->placeholderText(),QString("Describe the context..."));
        if(!evidence.isEmpty())QVERIFY(w.grab().save(evidence+"/input-textarea-captured.png"));
        w.setVariant("underlined");QVERIFY(!w.control()->isFigmaInputAppearance());
        QVERIFY(w.control()->property("figmaInputCompatibilityFallback").toBool());
        w.setVariant("outlined");QVERIFY(w.control()->isFigmaInputAppearance());
        w.setAppearance("solid");QVERIFY(!w.control()->isFigmaInputAppearance());
        w.setAppearance("figma-light");w.setKind(Z_InputField::Kind::Text);w.setSnapshotState("");
        line->setFocus();QInputMethodEvent commit;commit.setCommitString(QString::fromUtf8("中文"));
        QApplication::sendEvent(line,&commit);QCOMPARE(w.value(),QString::fromUtf8("中文"));
        QVERIFY_EXCEPTION_THROWN(w.setSnapshotState("Pressed"),std::invalid_argument);
    }
    void capturedInputInitialTextRendering() {
        Z_InputField w(tokens);w.setKind(Z_InputField::Kind::Password);
        w.setAppearance("figma-light");w.setValue("secret42");w.setSnapshotState("Default");show(w,320);
        QCOMPARE(w.lineEdit()->displayText(),QString(8,QChar(0x2022)));
        QCOMPARE(QFontInfo(w.lineEdit()->font()).family(),QString("Inter"));
        QCOMPARE(w.lineEdit()->font().weight(),int(QFont::Medium));
        w.setAppearance("solid");QVERIFY(w.lineEdit()->displayText()!=QString(8,QChar(0x2022)));
    }
    void capturedInputInitialPlaceholderRendering() {
        Z_InputField w(tokens);w.setKind(Z_InputField::Kind::TextArea);w.setAppearance("figma-light");
        w.setPlaceholder("Describe the context...");w.setSnapshotState("Default");w.setReadOnly(true);show(w,320);
        QSignalSpy nativeChanges(w.textArea(),&QPlainTextEdit::textChanged);
        QSignalSpy valueChanges(&w,&Z_InputField::valueChanged);
        const auto image=w.grab().toImage().convertToFormat(QImage::Format_ARGB32);
        int textPixels=0;
        for(int y=4;y<23;++y)for(int x=18;x<220;++x)
            if(image.pixelColor(x,y).alpha()>0 && image.pixelColor(x,y).value()<210)++textPixels;
        QVERIFY2(textPixels>20,qPrintable(QString("placeholder pixels=%1").arg(textPixels)));
        QCOMPARE(nativeChanges.count(),0);QCOMPARE(valueChanges.count(),0);
    }
    void capturedTextAreaLineHeightKeepsNativeEditing() {
        Z_InputField w(tokens);w.setKind(Z_InputField::Kind::TextArea);w.setAppearance("figma-light");
        w.setValue("First line\nSecond line");show(w,320);
        auto* area=w.textArea();auto* doc=area->document();
        QCOMPARE(QFontInfo(area->font()).family(),QString("Inter"));
        QCOMPARE(area->font().weight(),int(QFont::Medium));
        QCOMPARE(doc->documentLayout()->blockBoundingRect(doc->begin()).height(),14.0);
        auto cursor=area->textCursor();cursor.setPosition(0);area->setTextCursor(cursor);
        const int firstY=area->cursorRect().y();
        QTest::keyClick(area,Qt::Key_Down);QCOMPARE(area->cursorRect().y()-firstY,14);
        area->moveCursor(QTextCursor::End);QTest::keyClicks(area,"!");
        QCOMPARE(w.value(),QString("First line\nSecond line!"));
        area->undo();QCOMPARE(w.value(),QString("First line\nSecond line"));
        w.setAppearance("solid");QVERIFY(doc->documentLayout()->blockBoundingRect(doc->begin()).height()>14);
    }
    void editingGraphemesAndIme() {
        Z_InputField w(tokens);
        w.setAllowClear(true);
        w.setMaximumLength(3);
        show(w);
        w.lineEdit()->setFocus();
        QTest::keyClicks(w.lineEdit(), "abcd");
        QCOMPARE(w.value(), QString("abc"));
        QTest::mouseClick(w.findChild<QAbstractButton*>("CLEAR"), Qt::LeftButton);
        QCOMPARE(w.value(), QString());
        QVERIFY(w.lineEdit()->hasFocus());
        const auto graphemes = QString::fromUtf8("中á😀");
        QCOMPARE(Z_InputField::characterCount(graphemes), 3);
        w.setValue(graphemes + "z");
        QCOMPARE(w.value(), graphemes);
        w.setMaximumLength(-1);
        w.setValue({});
        QInputMethodEvent preedit(QString::fromUtf8("zhong"), {});
        QApplication::sendEvent(w.lineEdit(), &preedit);
        QCOMPARE(w.value(), QString());
        QInputMethodEvent commit;
        commit.setCommitString(QString::fromUtf8("中文"));
        QApplication::sendEvent(w.lineEdit(), &commit);
        QCOMPARE(w.value(), QString::fromUtf8("中文"));
    }
    void controlledAndReadonly() {
        Z_InputField w(tokens);
        w.setValue("A");
        w.setControlled(true);
        show(w);
        w.lineEdit()->setFocus();
        w.lineEdit()->end(false);
        QSignalSpy requests(&w, &Z_InputField::valueChangeRequested),
            changes(&w, &Z_InputField::valueChanged);
        QTest::keyClicks(w.lineEdit(), "B");
        QCOMPARE(w.value(), QString("A"));
        QCOMPARE(requests.count(), 1);
        QCOMPARE(changes.count(), 0);
        connect(&w, &Z_InputField::valueChangeRequested, &w, &Z_InputField::setValue);
        QTest::keyClicks(w.lineEdit(), "C");
        QCOMPARE(w.value(), QString("AC"));
        QCOMPARE(changes.count(), 1);
        w.setReadOnly(true);
        QTest::keyClicks(w.lineEdit(), "D");
        QCOMPARE(w.value(), QString("AC"));
        w.setReadOnly(false);
        w.setEnabled(false);
        QTest::keyClicks(w.lineEdit(), "E");
        QCOMPARE(w.value(), QString("AC"));
    }
    void decorationKeepsEditorAndSelection() {
        Z_InputField w(tokens);
        w.setValue("ABCDEF");
        show(w);
        auto* editor = w.lineEdit();
        editor->setFocus();
        editor->setSelection(1, 3);
        for (int i = 0; i < 8; ++i) {
            w.setPrefix(i % 2 ? "Prefix" : "");
            w.setSuffix(i % 2 ? "suffix" : "");
            w.setShowCount(i % 2);
            w.setAppearance(i % 2 ? "gradient-inset" : "solid");
        }
        QCOMPARE(w.lineEdit(), editor);
        QCOMPARE(editor->selectedText(), QString("BCD"));
        QVERIFY(editor->hasFocus());
        w.setPrefixIconSource(":/icons/arrow-left.svg");
        QCOMPARE(w.lineEdit(), editor);
        QVERIFY_EXCEPTION_THROWN(w.setSuffixIconSource("https://example.com/icon.svg"),
                                 std::invalid_argument);
    }
    void passwordSearchAndLoading() {
        Z_InputField w(tokens);
        w.setKind(Z_InputField::Kind::Password);
        w.setValue("secret");
        show(w);
        QCOMPARE(w.lineEdit()->echoMode(), QLineEdit::Password);
        QTest::mouseClick(w.findChild<QAbstractButton*>("PASSWORD_TOGGLE"), Qt::LeftButton);
        QCOMPARE(w.lineEdit()->echoMode(), QLineEdit::Normal);
        w.setKind(Z_InputField::Kind::Search);
        QSignalSpy search(&w, &Z_InputField::searchRequested);
        QTest::keyClick(w.lineEdit(), Qt::Key_Return);
        QCOMPARE(search.count(), 1);
        w.setLoading(true);
        const auto rect = w.control()->geometry();
        QTest::keyClick(w.lineEdit(), Qt::Key_Return);
        QCOMPARE(search.count(), 1);
        QTest::qWait(70);
        QCOMPARE(w.control()->geometry(), rect);
        w.setLoading(false);
        QTest::mouseClick(w.findChild<QAbstractButton*>("SEARCH_ACTION"), Qt::LeftButton);
        QCOMPARE(search.count(), 2);
    }
    void textAreaAutosizeAndCount() {
        // The gallery registers this CJK fallback before a user opens legacy Input examples.
        QVERIFY(QFontDatabase::addApplicationFont(":/figma/fonts/NotoSansSC-Regular.otf") >= 0);
        Z_InputField w(tokens);
        w.setKind(Z_InputField::Kind::TextArea);
        w.setAutoSize(true, 2, 4);
        w.setShowCount(true);
        w.setCountLimit(5);
        show(w);
        const int initialHeight = w.control()->height();
        w.setValue(QString::fromUtf8("首片检测完成。\n等待工程师确认腔体状态。"));
        QTest::qWait(20);
        QCOMPARE(w.textArea()->verticalScrollBar()->maximum(), 0);
        w.setValue("one\ntwo\nthree\nfour\nfive");
        QTest::qWait(20);
        QVERIFY(w.control()->height() > initialHeight);
        QVERIFY(w.textArea()->verticalScrollBar()->maximum() > 0);
        QCOMPARE(w.value(), QString("one\ntwo\nthree\nfour\nfive"));
        QCOMPARE(w.findChild<QLabel*>("COUNT")->text(), QString("23 / 5"));
        w.setValue(QString(180, 'x'));
        QTest::qWait(20);
        QVERIFY2(w.control()->height() > initialHeight, "Wrapped lines must contribute to autoSize");
    }
    void selectSingleKeyboardAndPopup() {
        Z_Select w(tokens);
        w.setOptions(options());
        show(w);
        w.setOpen(true);
        QTest::qWait(20);
        QVERIFY(w.isOpen());
        QCOMPARE(w.optionView()->currentIndex().data().toString(), QString("Alpha"));
        QTest::keyClick(w.searchEdit(), Qt::Key_Down);
        QCOMPARE(w.optionView()->currentIndex().data().toString(), QString("Gamma"));
        QTest::keyClick(w.searchEdit(), Qt::Key_Return);
        QCOMPARE(w.values(), QStringList{"c"});
        QVERIFY(!w.isOpen());
        w.setOpen(true);
        w.setQuery("Delta");
        QCOMPARE(w.optionView()->model()->rowCount(), 2);
        QTest::keyClick(w.searchEdit(), Qt::Key_Return);
        QCOMPARE(w.values(), QStringList{"d"});
        w.setReadOnly(true);
        w.setOpen(true);
        QVERIFY(!w.isOpen());
        w.setReadOnly(false);
        w.setOpen(true);
        QVERIFY(QGuiApplication::primaryScreen()->availableGeometry().contains(
            w.popupWidget()->geometry()));
        QTest::keyClick(w.searchEdit(), Qt::Key_Escape);
        QVERIFY(!w.isOpen());
    }
    void otpCompositionAndPaste() {
        commonWidget::Z_OtpInput w(tokens);
        show(w);
        QSignalSpy complete(&w, &commonWidget::Z_OtpInput::completed);
        auto editors = w.editors();
        editors.first()->setFocus();
        QApplication::clipboard()->setText("123 456");
        QTest::keyClick(editors.first(), Qt::Key_V, Qt::ControlModifier);
        QCOMPARE(w.value(), QString("123456"));
        QCOMPARE(complete.count(), 1);
        w.setValue("12");
        QTest::keyClick(editors[2], Qt::Key_Backspace);
        QCOMPARE(w.value(), QString("1"));
        w.setReadOnly(true);
        QTest::keyClicks(editors[0], "9");
        QCOMPARE(w.value(), QString("1"));
        w.setMasked(true);
        QCOMPARE(editors[0]->echoMode(), QLineEdit::Password);
        QVERIFY_EXCEPTION_THROWN(w.setValue("ABC"), std::invalid_argument);
        for (auto* frame : w.findChildren<field::Chrome*>())
            QVERIFY(frame->height() >= 32);
    }
    void selectMultipleTagsControlledAndLimits() {
        Z_Select w(tokens);
        w.setMode(Z_Select::Mode::Multiple);
        w.setOptions(options());
        w.setValues({"a"});
        w.setControlled(true);
        show(w);
        w.setOpen(true);
        QSignalSpy changes(&w, &Z_Select::valuesChanged),
            requests(&w, &Z_Select::valuesChangeRequested);
        w.setQuery("Gamma");
        QTest::keyClick(w.searchEdit(), Qt::Key_Return);
        QCOMPARE(w.values(), QStringList{"a"});
        QCOMPARE(changes.count(), 0);
        QCOMPARE(requests.count(), 1);
        connect(&w, &Z_Select::valuesChangeRequested, &w, &Z_Select::setValues);
        w.setMaxCount(1);
        QSignalSpy limits(&w, &Z_Select::limitReached);
        w.setQuery("Gamma");
        QTest::keyClick(w.searchEdit(), Qt::Key_Return);
        QCOMPARE(limits.count(), 1);
        w.setMaxCount(3);
        w.setQuery("Gamma");
        QTest::keyClick(w.searchEdit(), Qt::Key_Return);
        QCOMPARE(w.values(), (QStringList{"a", "c"}));
        w.setOpen(false);
        w.setMode(Z_Select::Mode::Tags);
        w.setOpen(true);
        w.setQuery("Custom");
        QTest::keyClick(w.searchEdit(), Qt::Key_Return);
        QVERIFY(w.values().contains("Custom"));
        w.setMaxCount(-1);
        w.setQuery("East,West;");
        QVERIFY(w.values().contains("East"));
        QVERIFY(w.values().contains("West"));
        w.setOpen(false);
    }
    void selectTagsFirstLayoutAndStableFocus() {
        Z_Select w(tokens);
        w.setMode(Z_Select::Mode::Multiple);
        w.setOptions(options());
        w.setValues({"a", "c", "d"});
        show(w, 640);
        QTest::qWait(30);
        auto labels = w.findChildren<QLabel*>("TAG_LABEL");
        QVERIFY(!labels.isEmpty());
        QVERIFY2(labels.first()->text().contains("Alpha"),
                 qPrintable("First tag was prematurely elided: " + labels.first()->text()));
        auto* remove = w.findChild<QAbstractButton*>("TAG_REMOVE");
        QVERIFY(remove);
        QPointer<QAbstractButton> identity(remove);
        remove->setFocus();
        QTest::qWait(10);
        QVERIFY(identity);
        QCOMPARE(identity.data(), w.findChild<QAbstractButton*>("TAG_REMOVE"));
        QTest::mouseClick(remove, Qt::LeftButton);
        QTest::qWait(20);
        QVERIFY(!w.values().contains("a"));
    }
    void selectAsyncAndLargeModel() {
        Z_Select w(tokens);
        w.setOptions(options());
        w.setValues({"a"});
        show(w);
        auto first = w.beginOptionsRequest(), second = w.beginOptionsRequest();
        QVERIFY(!w.completeOptionsRequest(first, {}));
        QVERIFY(w.completeOptionsRequest(second, {{"x", "New label", {}, false}}));
        QCOMPARE(w.labels(), QStringList{"Alpha"});
        QVector<Z_Select::Option> many;
        many.reserve(10000);
        for (int i = 0; i < 10000; ++i)
            many.append({QString::number(i), QString("Item %1").arg(i), {}, i % 10 == 0});
        QElapsedTimer timer;
        timer.start();
        w.setOptions(many);
        qInfo("Model replace 10000 rows: %lld ms", timer.elapsed());
        QCOMPARE(w.optionView()->model()->rowCount(), 10000);
        QVERIFY(w.findChildren<QWidget*>().size() < 100);
        w.setQuery("Item 9999");
        QCOMPARE(w.optionView()->model()->rowCount(), 1);
        QVERIFY_EXCEPTION_THROWN(w.setOptions({{"x", "A", {}, false}, {"x", "B", {}, false}}),
                                 std::invalid_argument);
    }
    void selectInsideInitiallyHiddenPage() {
        QStackedWidget host;
        host.addWidget(new QWidget);
        auto* page = new QWidget;
        auto* layout = new QVBoxLayout(page);
        auto* w = new Z_Select(tokens);
        w->setMode(Z_Select::Mode::Multiple);
        w->setOptions(options());
        w->setValues({"a", "c", "d"});
        layout->addWidget(w);
        layout->addStretch();
        host.addWidget(page);
        host.resize(700, 300);
        host.show();
        QTest::qWait(20);
        host.setCurrentWidget(page);
        QTest::qWait(50);
        auto labels = w->findChildren<QLabel*>("TAG_LABEL");
        QVERIFY(!labels.isEmpty());
        QVERIFY2(labels.first()->text().contains("Alpha"),
                 qPrintable("Hidden page tag: " + labels.first()->text()));
    }
    void materialsAndRenderEvidence() {
        Z_InputField input(tokens);
        Z_Select select(tokens);
        input.setLabel("Recipe");
        input.setValue("Batch-91");
        select.setLabel("Chamber");
        select.setOptions(options());
        select.setValues({"a"});
        show(input);
        show(select);
        const auto height = input.control()->height();
        for (auto variant : {"outlined", "filled", "borderless", "underlined"}) {
            input.setVariant(variant);
            select.setVariant(variant);
            for (auto preset : {"solid", "gradient", "inset", "gradient-inset", "outline",
                                "elevated", "gradient-elevated"}) {
                input.setAppearance(preset);
                select.setAppearance(preset);
                QApplication::processEvents();
                QCOMPARE(input.control()->height(), height);
                QVERIFY(!input.grab().isNull());
                QVERIFY(!select.grab().isNull());
            }
        }
        if (!evidence.isEmpty()) {
            input.setVariant("outlined");
            input.setAppearance("gradient-inset");
            QVERIFY(input.grab().save(evidence + "/input.png"));
            select.setVariant("outlined");
            select.setAppearance("gradient-elevated");
            QVERIFY(select.grab().save(evidence + "/select.png"));
            select.setOpen(true);
            QTest::qWait(40);
            QVERIFY(select.popupWidget()->grab().save(evidence + "/select-popup.png"));
            select.setOpen(false);
        }
    }
};
int main(int argc, char** argv) {
    QCoreApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
    QCoreApplication::setAttribute(Qt::AA_UseHighDpiPixmaps);
    QApplication app(argc, argv);
    app.setStyle(QStyleFactory::create("Fusion"));
    button_demo::install(app, qEnvironmentVariable("DEMO_PROFILE", "light-comfortable"));
    FormsTests tests;
    return QTest::qExec(&tests, argc, argv);
}
#include "FormsTests.moc"
