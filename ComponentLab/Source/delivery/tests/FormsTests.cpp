#include "DemoRuntime.h"
#include "FocusIndicator.h"
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
#include <cstdio>
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
        const auto profile = qEnvironmentVariable("DEMO_PROFILE", "standard-light");
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
                          "PASSWORD_TOGGLE", "SEARCH_ACTION", "LOADING_ACTION", "FOOTER", "HINT", "COUNT"})
            QVERIFY2(w.findChild<QWidget*>(name), name);
    }
    void v2InputPreservesEditorsAndLocksLoading() {
        Z_InputField w(tokens);w.setAppearance("outlined");
        auto* line=w.lineEdit();auto* area=w.textArea();
        w.setValue("ABCDE");show(w,320);line->setSelection(1,3);
        w.setSnapshotState("Hover");
        QCOMPARE(w.lineEdit(),line);QCOMPARE(w.textArea(),area);
        QCOMPARE(line->selectedText(),QString("BCD"));
        QCOMPARE(w.control()->property("fieldAppearance").toString(),QString("outlined"));
        QVERIFY(!tokens->sourceHash().isEmpty());
        w.setKind(Z_InputField::Kind::Search);
        w.setLoading(true);
        QVERIFY(line->isReadOnly());
        QVERIFY(w.findChild<field::Action*>("LOADING_ACTION")->isVisible());
        QVERIFY(!w.findChild<field::Action*>("SEARCH_ACTION")->isVisible());
        w.setLoading(false);QVERIFY(!line->isReadOnly());
        QVERIFY_EXCEPTION_THROWN(w.setAppearance("figma-light"),std::invalid_argument);
        QVERIFY_EXCEPTION_THROWN(w.setSnapshotState("Pressed"),std::invalid_argument);
    }
    void inputHoverAndActiveFocusUseDistinctStates() {
        const auto hoverRecipe = tokens->recipe(field::Family::Input, "outlined", "hover");
        const auto focusedRecipe = tokens->recipe(field::Family::Input, "outlined", "focused");
        QCOMPARE(hoverRecipe.border, tokens->color("hoverBorder"));
        QCOMPARE(focusedRecipe.border, tokens->color("focusBorder"));
        QCOMPARE(hoverRecipe.border, QColor("#191919"));
        QCOMPARE(focusedRecipe.border, QColor("#6235F7"));
        QVERIFY(hoverRecipe.border != focusedRecipe.border);

        auto& focusIndicator = commonWidget::FocusIndicator::instance();
        const auto previousMode = focusIndicator.mode();
        focusIndicator.setMode(commonWidget::FocusIndicator::Mode::Keyboard);

        QWidget host;
        QVBoxLayout layout(&host);
        QLineEdit focusSink;
        Z_InputField input(tokens);
        layout.addWidget(&focusSink);
        layout.addWidget(&input);
        show(host);

        focusSink.setFocus();
        QTest::mouseMove(input.control(), input.control()->rect().center());
        QTRY_COMPARE(input.control()->property("fieldState").toString(), QString("hover"));

        QTest::mouseClick(input.lineEdit(), Qt::LeftButton, {}, input.lineEdit()->rect().center());
        QTRY_VERIFY(input.lineEdit()->hasFocus());
        QTRY_COMPARE(input.control()->property("fieldState").toString(), QString("focused"));
        focusIndicator.setMode(previousMode);
    }
    void v2TextAreaLineHeightKeepsNativeEditing() {
        Z_InputField w(tokens);w.setKind(Z_InputField::Kind::TextArea);w.setAppearance("outlined");
        w.setValue("First line\nSecond line");show(w,320);
        auto* area=w.textArea();auto* doc=area->document();
        QCOMPARE(area->font().pixelSize(),tokens->size("md")["fontSize"].toInt());
        auto cursor=area->textCursor();cursor.setPosition(0);area->setTextCursor(cursor);
        area->moveCursor(QTextCursor::End);QTest::keyClicks(area,"!");
        QCOMPARE(w.value(),QString("First line\nSecond line!"));
        area->undo();QCOMPARE(w.value(),QString("First line\nSecond line"));
        QVERIFY(doc->documentLayout()->blockBoundingRect(doc->begin()).height()>0);
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
            w.setSnapshotState(i % 2 ? "Hover" : "Default");
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
    void inputEmbeddedActionStatesAndApi() {
        Z_InputField w(tokens);
        w.setAllowClear(true);
        w.setValue("secret");
        show(w);
        auto* clear = w.clearAction();
        QCOMPARE(clear, w.findChild<field::Action*>("CLEAR"));
        QCOMPARE(clear->role(), field::Action::Role::Clear);
        QTest::mouseMove(w.lineEdit(), QPoint(2, 2));
        QTRY_COMPARE(clear->interactionState(), field::Action::InteractionState::Default);
        const auto defaultStyle = clear->styleSheet();
        QSignalSpy clearStates(clear, &field::Action::interactionStateChanged);
        QTest::mouseMove(clear, clear->rect().center());
        QTRY_COMPARE(clear->interactionState(), field::Action::InteractionState::Hover);
        QTRY_VERIFY(defaultStyle != clear->styleSheet());
        const auto hoverStyle = clear->styleSheet();
        QTest::mousePress(clear, Qt::LeftButton, {}, clear->rect().center());
        QCOMPARE(clear->interactionState(), field::Action::InteractionState::Pressed);
        QTRY_VERIFY(hoverStyle != clear->styleSheet());
        QTest::mouseRelease(clear, Qt::LeftButton, {}, clear->rect().center());
        QVERIFY(clearStates.count() >= 2);

        w.setValue("clear-by-api");
        QSignalSpy cleared(&w, &Z_InputField::cleared);
        w.clear();
        QCOMPARE(w.value(), QString());
        QCOMPARE(cleared.count(), 1);

        w.setValue("secret");
        w.setReadOnly(true);
        QVERIFY(clear->isVisible());
        QCOMPARE(clear->interactionState(), field::Action::InteractionState::Disabled);
        QVERIFY(clear->styleSheet() != defaultStyle);
        const auto before = w.value();
        w.clear();
        QCOMPARE(w.value(), before);
        w.setReadOnly(false);

        w.setKind(Z_InputField::Kind::Password);
        auto* password = w.passwordToggleAction();
        QCOMPARE(password, w.findChild<field::Action*>("PASSWORD_TOGGLE"));
        QCOMPARE(password->role(), field::Action::Role::PasswordToggle);
        QSignalSpy visibility(&w, &Z_InputField::passwordVisibilityChanged);
        w.togglePasswordVisibility();
        QVERIFY(w.isPasswordVisible());
        QCOMPARE(w.lineEdit()->echoMode(), QLineEdit::Normal);
        QCOMPARE(visibility.count(), 1);
        w.togglePasswordVisibility();
        QVERIFY(!w.isPasswordVisible());
        QCOMPARE(w.lineEdit()->echoMode(), QLineEdit::Password);
        QCOMPARE(visibility.count(), 2);
        QTest::mouseMove(w.lineEdit(), QPoint(2, 2));
        QTRY_COMPARE(password->interactionState(), field::Action::InteractionState::Default);
        QTest::mouseMove(password, password->rect().center());
        QTRY_COMPARE(password->interactionState(), field::Action::InteractionState::Hover);
        QTest::mousePress(password, Qt::LeftButton, {}, password->rect().center());
        QCOMPARE(password->interactionState(), field::Action::InteractionState::Pressed);
        QTest::mouseRelease(password, Qt::LeftButton, {}, password->rect().center());
        w.setReadOnly(true);
        QCOMPARE(password->interactionState(), field::Action::InteractionState::Disabled);
    }
    void textAreaAutosizeAndCount() {
        // The gallery registers this CJK fallback before a user opens legacy Input examples.
        QVERIFY(QFontDatabase::addApplicationFont(":/fonts/HarmonyOS_Sans_SC_Regular.ttf") >= 0);
        Z_InputField w(tokens);
        w.setKind(Z_InputField::Kind::TextArea);
        w.setAutoSize(true, 2, 4);
        w.setShowCount(true);
        w.setCountLimit(5);
        show(w);
        const int initialHeight = w.control()->height();
        w.setValue(QString::fromUtf8("首片检测完成。\n等待工程师确认腔体状态。"));
        QTest::qWait(20);
        w.setValue("one\ntwo\nthree\nfour\nfive");
        QTest::qWait(20);
        QVERIFY(w.control()->height() > initialHeight);
        const int cappedHeight = w.control()->height();
        w.setValue("one\ntwo\nthree\nfour\nfive\nsix\nseven\neight\nnine");
        QTest::qWait(20);
        QCOMPARE(w.control()->height(), cappedHeight);
        w.setValue("one\ntwo\nthree\nfour\nfive");
        QTest::qWait(20);
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
    void v2StatesAndRenderEvidence() {
        Z_InputField input(tokens);
        Z_Select select(tokens);
        input.setLabel("Recipe");
        input.setValue("Batch-91");
        select.setLabel("Chamber");
        select.setOptions(options());
        select.setValues({"a"});
        show(input);
        show(select);
        for (const auto& size : QStringList{"sm", "md", "lg"}) {
            input.setControlSize(size);
            select.setControlSize(size);
            for (const auto& state : QStringList{"Default", "Hover", "Focused", "Error", "Disabled"}) {
                input.setSnapshotState(state);
                select.control()->setSnapshotState(state);
                QApplication::processEvents();
                QVERIFY(!input.grab().isNull());
                QVERIFY(!select.grab().isNull());
            }
        }
        if (!evidence.isEmpty()) {
            input.setAppearance("outlined");
            input.setSnapshotState("Focused");
            QVERIFY(input.grab().save(evidence + "/input.png"));
            select.setAppearance("outlined");
            select.control()->setSnapshotState("Focused");
            QVERIFY(select.grab().save(evidence + "/select.png"));
            select.setOpen(true);
            QTest::qWait(40);
            QVERIFY(select.popupWidget()->grab().save(evidence + "/select-popup.png"));
            select.setOpen(false);
        }
    }
};
int main(int argc, char** argv) {
    try {
        QCoreApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
        QCoreApplication::setAttribute(Qt::AA_UseHighDpiPixmaps);
        QApplication app(argc, argv);
        app.setStyle(QStyleFactory::create("Fusion"));
        button_demo::install(app, qEnvironmentVariable("DEMO_PROFILE", "standard-light"));
        FormsTests tests;
        return QTest::qExec(&tests, argc, argv);
    } catch (const std::exception& error) {
        std::fprintf(stderr, "FormsTests uncaught exception: %s\n", error.what());
        return 2;
    }
}
#include "FormsTests.moc"
