#include "Z_InputField.h"
#include "IconSource.h"
#include <QApplication>
#include <QEvent>
#include <QFontDatabase>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPlainTextEdit>
#include <QSignalBlocker>
#include <QStyle>
#include <QTextBoundaryFinder>
#include <QTextDocument>
#include <QTextBlock>
#include <QTextLayout>
#include <QAbstractTextDocumentLayout>
#include <QTimer>
#include <QVBoxLayout>
#include <limits>
#include <cmath>
#include <stdexcept>

namespace commonWidget {
namespace {
// QPlainTextDocumentLayout ignores QTextBlockFormat line-height in Qt 5.15.2.
// Adapt only layout metrics; the QTextDocument, cursor, IME, undo and native editor remain intact.
class InputDocumentLayout final : public QPlainTextDocumentLayout {
public:
    explicit InputDocumentLayout(QTextDocument* document) : QPlainTextDocumentLayout(document) {}

    void setCapturedLineHeight(int value) {
        if (lineHeight == value)
            return;
        lineHeight = value;
        document()->markContentsDirty(0, document()->characterCount());
    }

    QRectF blockBoundingRect(const QTextBlock& block) const override {
        auto bounds = QPlainTextDocumentLayout::blockBoundingRect(block);
        if (lineHeight <= 0 || !block.isVisible())
            return bounds;
        auto* layout = block.layout();
        for (int i = 0; i < layout->lineCount(); ++i) {
            auto line = layout->lineAt(i);
            line.setPosition(
                QPointF(line.position().x(), i * lineHeight + (lineHeight - line.height()) / 2));
        }
        bounds.setHeight(qMax(1, layout->lineCount()) * lineHeight);
        return bounds;
    }

private:
    int lineHeight = 0;
};

QString truncate(const QString& text, int maximum) {
    if (maximum < 0)
        return text;
    QTextBoundaryFinder finder(QTextBoundaryFinder::Grapheme, text);
    int end = 0;
    for (int i = 0; i < maximum; ++i) {
        int p = finder.toNextBoundary();
        if (p < 0)
            return text;
        end = p;
    }
    return text.left(end);
}

QLabel* label(const char* name, QWidget* parent) {
    auto* w = new QLabel(parent);
    w->setObjectName(name);
    w->setProperty("fieldPart", name);
    w->setTextFormat(Qt::PlainText);
    return w;
}
} // namespace

struct Z_InputField::Impl {
    std::shared_ptr<const field::Tokens> tokens;
    field::Chrome* chrome = nullptr;
    QLineEdit* line = nullptr;
    QPlainTextEdit* area = nullptr;
    QLabel *label = nullptr, *hint = nullptr, *prefix = nullptr, *suffix = nullptr,
           *count = nullptr;
    field::Action *clear = nullptr, *password = nullptr, *search = nullptr;
    QHBoxLayout* row = nullptr;
    QVBoxLayout* outer = nullptr;
    QWidget* footer = nullptr;
    QString value, prefixText, suffixText, appearance = "solid";
    QIcon prefixIcon, suffixIcon;
    Kind kind = Kind::Text;
    bool controlled = false, clearAllowed = false, countVisible = false, readOnly = false;
    bool loading = false, autoSize = true, syncing = false, resizePending = false;
    int maximum = -1, countLimit = -1, minRows = 2, maxRows = 5;
};

Z_InputField::Z_InputField(std::shared_ptr<const field::Tokens> tokens, QWidget* parent)
    : QWidget(parent), d(new Impl) {
    d->tokens = std::move(tokens);
    setObjectName("INPUT_ROOT");
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Maximum);
    d->outer = new QVBoxLayout(this);
    d->outer->setContentsMargins(0, 0, 0, 0);
    d->outer->setSpacing(d->tokens->metric("labelGap"));
    d->label = label("LABEL", this);
    d->label->setWordWrap(true);
    d->chrome = new field::Chrome(d->tokens, this);
    d->row = new QHBoxLayout(d->chrome);
    d->row->setSpacing(d->tokens->metric("gap"));
    d->prefix = label("PREFIX", d->chrome);
    d->prefix->setProperty("fieldPart", "PREFIX");
    d->suffix = label("SUFFIX", d->chrome);
    d->suffix->setProperty("fieldPart", "SUFFIX");
    d->line = new QLineEdit(d->chrome);
    d->line->setObjectName("EDITOR");
    d->line->setProperty("fieldPart", "EDITOR");
    d->line->setMaxLength(std::numeric_limits<int>::max());
    d->line->setMinimumWidth(0);
    d->area = new QPlainTextEdit(d->chrome);
    d->area->document()->setDocumentLayout(new InputDocumentLayout(d->area->document()));
    d->area->setObjectName("TEXTAREA");
    d->area->setProperty("fieldPart", "EDITOR");
    d->area->setFrameShape(QFrame::NoFrame);
    d->area->document()->setDocumentMargin(0);
    d->area->setTabChangesFocus(true);
    d->area->setMinimumSize(0, 0);
    d->area->viewport()->installEventFilter(this);
    d->clear = new field::Action(d->tokens, d->chrome);
    d->clear->setObjectName("CLEAR");
    d->clear->setIconSource(d->tokens->iconSource("clear"));
    d->clear->setToolTip(tr("Clear"));
    d->clear->setAccessibleName(tr("Clear input"));
    d->password = new field::Action(d->tokens, d->chrome);
    d->password->setObjectName("PASSWORD_TOGGLE");
    d->password->setIconSource(d->tokens->iconSource("showPassword"));
    d->password->setCheckable(true);
    d->password->setToolTip(tr("Show password"));
    d->password->setAccessibleName(tr("Show password"));
    d->search = new field::Action(d->tokens, d->chrome);
    d->search->setObjectName("SEARCH_ACTION");
    d->search->setIconSource(d->tokens->iconSource("search"));
    d->search->setToolTip(tr("Search"));
    d->search->setAccessibleName(tr("Search"));
    d->row->addWidget(d->prefix);
    d->row->addWidget(d->line, 1);
    d->row->addWidget(d->area, 1);
    d->row->addWidget(d->suffix);
    d->row->addWidget(d->clear);
    d->row->addWidget(d->password);
    d->row->addWidget(d->search);
    d->footer = new QWidget(this);
    d->footer->setObjectName("FOOTER");
    auto* foot = new QHBoxLayout(d->footer);
    foot->setContentsMargins(0, 0, 0, 0);
    foot->setSpacing(d->tokens->metric("hintGap"));
    d->hint = label("HINT", d->footer);
    d->hint->setWordWrap(true);
    d->count = label("COUNT", d->footer);
    d->count->setAlignment(Qt::AlignRight | Qt::AlignTop);
    foot->addWidget(d->hint, 1);
    foot->addWidget(d->count);
    d->outer->addWidget(d->label);
    d->outer->addWidget(d->chrome);
    d->outer->addWidget(d->footer);
    d->label->setBuddy(d->line);
    setFocusProxy(d->line);
    d->minRows = d->tokens->metric("textAreaMinRows");
    d->maxRows = d->tokens->metric("textAreaMaxRows");
    connect(d->line, &QLineEdit::textEdited, this, &Z_InputField::propose);
    connect(d->area, &QPlainTextEdit::textChanged, this, [this] {
        if (!d->syncing)
            propose(d->area->toPlainText());
    });
    connect(d->line, &QLineEdit::returnPressed, this, [this] {
        if (d->readOnly || d->loading)
            return;
        emit submitted(d->value);
        if (d->kind == Kind::Search)
            emit searchRequested(d->value, "input");
    });
    connect(d->clear, &QAbstractButton::clicked, this, [this] {
        propose({});
        (d->kind == Kind::TextArea ? static_cast<QWidget*>(d->area) : d->line)->setFocus();
        emit cleared();
        if (d->kind == Kind::Search)
            emit searchRequested(d->value, "clear");
    });
    connect(d->password, &QAbstractButton::toggled, this, [this](bool on) {
        d->line->setEchoMode(d->kind != Kind::Password || on ? QLineEdit::Normal
                                                             : QLineEdit::Password);
        d->password->setToolTip(on ? tr("Hide password") : tr("Show password"));
        d->password->setAccessibleName(d->password->toolTip());
        d->password->setIconSource(d->tokens->iconSource(on ? "hidePassword" : "showPassword"));
        sync();
    });
    connect(d->password, &QAbstractButton::clicked, d->line, [this] {
        d->line->setFocus();
    });
    connect(d->search, &QAbstractButton::clicked, this, [this] {
        if (!d->loading)
            emit searchRequested(d->value, "button");
    });
    connect(d->chrome, &field::Chrome::visualChanged, this, [this] {
        if (!d->syncing && d->line)
            sync();
    });
    sync();
}

Z_InputField::~Z_InputField() {
    for (auto* child : findChildren<QObject*>())
        disconnect(child, nullptr, this, nullptr);
}

QString Z_InputField::value() const {
    return d->value;
}

bool Z_InputField::isControlled() const {
    return d->controlled;
}

QString Z_InputField::appearance() const {
    return d->appearance;
}

bool Z_InputField::isPasswordVisible() const {
    return d->password->isChecked();
}

void Z_InputField::setPasswordVisible(bool value) {
    d->password->setChecked(value);
}

void Z_InputField::setSnapshotState(const QString& value) {
    d->chrome->setSnapshotState(value);
    setProperty("figmaInputSnapshotState", value);
}

QLineEdit* Z_InputField::lineEdit() const {
    return d->line;
}

QPlainTextEdit* Z_InputField::textArea() const {
    return d->area;
}

field::Chrome* Z_InputField::control() const {
    return d->chrome;
}

QSize Z_InputField::sizeHint() const {
    return QSize(d->tokens->metric("fieldPreferredWidth"), d->outer->sizeHint().height());
}

int Z_InputField::characterCount(const QString& text) {
    QTextBoundaryFinder f(QTextBoundaryFinder::Grapheme, text);
    int n = 0;
    while (f.toNextBoundary() >= 0)
        ++n;
    return n;
}

void Z_InputField::setValue(const QString& value) {
    auto next = truncate(value, d->maximum);
    bool changed = d->value != next;
    d->value = next;
    // Stable editors preserve cursor, selection, undo and IME when decoration changes.
    if (d->line->text() != next) {
        QSignalBlocker b(d->line);
        d->line->setText(next);
    }
    if (d->area->toPlainText() != next) {
        QSignalBlocker b(d->area);
        d->area->setPlainText(next);
    }
    sync();
    if (changed)
        emit valueChanged(next);
}

void Z_InputField::propose(const QString& value) {
    if (d->syncing)
        return;
    if (!isEnabled() || d->readOnly) {
        setValue(d->value);
        return;
    }
    const auto next = truncate(value, d->maximum);
    const int cursor = d->line->cursorPosition();
    if (!d->controlled)
        setValue(next);
    emit valueChangeRequested(next);
    if (d->controlled) {
        setValue(d->value);
        d->line->setCursorPosition(qMin(cursor, d->value.size()));
    }
}

void Z_InputField::setKind(Kind value) {
    d->kind = value;
    d->line->setEchoMode(value == Kind::Password && !d->password->isChecked() ? QLineEdit::Password
                                                                              : QLineEdit::Normal);
    setFocusProxy(value == Kind::TextArea ? static_cast<QWidget*>(d->area) : d->line);
    d->label->setBuddy(focusProxy());
    sync();
}

void Z_InputField::setControlSize(const QString& value) {
    d->chrome->setControlSize(value);
    sync();
}

void Z_InputField::setVariant(const QString& value) {
    d->chrome->setVariant(value);
}

void Z_InputField::setAppearance(const QString& value) {
    if (value == "figma-light") {
        static const bool fontsLoaded = [] {
            QFontDatabase::addApplicationFont(":/figma/fonts/Inter-Regular.otf");
            QFontDatabase::addApplicationFont(":/figma/fonts/Inter-Medium.otf");
            QFontDatabase::addApplicationFont(":/figma/fonts/NotoSansSC-Regular.otf");
            return true;
        }();
        Q_UNUSED(fontsLoaded);
        d->chrome->setAppearance("solid");
        d->appearance = value;
        d->chrome->setFigmaInputAppearance(true);
    } else {
        // Validate first so an invalid choice cannot partially reset the active appearance.
        d->tokens->recipe(value, "outlined", "default");
        d->appearance = value;
        d->chrome->setFigmaInputAppearance(false);
        d->chrome->setAppearance(value);
    }
}

void Z_InputField::setStatus(const QString& value) {
    d->chrome->setStatus(value);
}

void Z_InputField::setLabel(const QString& value) {
    d->label->setText(value);
    d->line->setAccessibleName(value);
    d->area->setAccessibleName(value);
    sync();
}

void Z_InputField::setHint(const QString& value) {
    d->hint->setText(value);
    d->line->setAccessibleDescription(value);
    d->area->setAccessibleDescription(value);
    sync();
}

void Z_InputField::setPlaceholder(const QString& value) {
    d->line->setPlaceholderText(value);
    d->area->setPlaceholderText(value);
#if QT_VERSION == QT_VERSION_CHECK(5, 15, 2)
    // Qt 5.15.2 setPlaceholderText updates the viewport but not placeholderVisible.
    // Refresh that cache only, without setPlainText(), undo loss, fake textChanged or IME edits.
    // Version-pinned private slot: revalidate this compatibility shim on a Qt upgrade.
    if (!QMetaObject::invokeMethod(d->area, "_q_textChanged", Qt::DirectConnection))
        throw std::runtime_error("Qt 5.15.2 placeholder refresh slot unavailable");
#endif
}

void Z_InputField::setPrefix(const QString& value) {
    d->prefixText = value;
    d->prefixIcon = {};
    sync();
}

void Z_InputField::setSuffix(const QString& value) {
    d->suffixText = value;
    d->suffixIcon = {};
    sync();
}

void Z_InputField::setPrefixIconSource(const QString& value) {
    d->prefixIcon = IconSource::load(value);
    d->prefixText.clear();
    sync();
}

void Z_InputField::setSuffixIconSource(const QString& value) {
    d->suffixIcon = IconSource::load(value);
    d->suffixText.clear();
    sync();
}

void Z_InputField::setAllowClear(bool value) {
    d->clearAllowed = value;
    sync();
}

void Z_InputField::setShowCount(bool value) {
    d->countVisible = value;
    sync();
}

void Z_InputField::setMaximumLength(int value) {
    if (value < -1)
        throw std::invalid_argument("Maximum length must be -1 or >=0");
    d->maximum = value;
    setValue(d->value);
}

void Z_InputField::setCountLimit(int value) {
    if (value < -1)
        throw std::invalid_argument("Count limit must be -1 or >=0");
    d->countLimit = value;
    sync();
}

void Z_InputField::setReadOnly(bool value) {
    d->readOnly = value;
    d->line->setReadOnly(value);
    d->area->setReadOnly(value);
    sync();
}

void Z_InputField::setControlled(bool value) {
    d->controlled = value;
}

void Z_InputField::setLoading(bool value) {
    d->loading = value;
    d->search->setBusy(value);
    sync();
}

void Z_InputField::setAutoSize(bool enabled, int min, int max) {
    if (!min)
        min = d->tokens->metric("textAreaMinRows");
    if (!max)
        max = d->tokens->metric("textAreaMaxRows");
    if (min < 1 || max < min)
        throw std::invalid_argument("Invalid textarea row bounds");
    d->autoSize = enabled;
    d->minRows = min;
    d->maxRows = max;
    sync();
}

void Z_InputField::changeEvent(QEvent* e) {
    QWidget::changeEvent(e);
    if (d && d->line && e->type() == QEvent::EnabledChange)
        sync();
}

bool Z_InputField::eventFilter(QObject* target, QEvent* e) {
    if (target == d->area->viewport() && e->type() == QEvent::Resize && !d->resizePending) {
        d->resizePending = true;
        QTimer::singleShot(0, this, [this] {
            d->resizePending = false;
            sync();
        });
    }
    return QWidget::eventFilter(target, e);
}

void Z_InputField::sync() {
    if (d->syncing)
        return;
    d->syncing = true;
    const auto& t = *d->tokens;
    auto m = t.size(d->chrome->controlSize());
    const bool captured = d->chrome->isFigmaInputAppearance();
    const auto metrics = captured ? field::inputVisuals()["metrics"].toObject() : QJsonObject{};
    const auto colors = captured ? field::inputVisuals()["colors"].toObject() : QJsonObject{};
    const auto capturedSize = metrics["sizes"].toObject()[d->chrome->controlSize()].toObject();
    const bool disabled = captured ? d->chrome->visualState() == "disabled" : !isEnabled();
    const auto foreground = captured ? QColor(colors[disabled ? "disabledText" : "text"].toString())
                                     : t.color(isEnabled() ? "text" : "disabledText");
    QString css;
    if (captured) {
        m["paddingInline"] = capturedSize["paddingInline"];
        m["paddingBlock"] = metrics["textAreaBlockPadding"];
        m["lineHeight"] = metrics["textFrameHeight"];
        css =
            QString("QLineEdit[fieldPart=\"EDITOR\"], QPlainTextEdit[fieldPart=\"EDITOR\"] {"
                    "background:transparent;border:0;padding:0;min-height:0;color:%1;"
                    "font-family:'Inter';font-size:14px;selection-background-color:#6235f7;"
                    "selection-color:white;}"
                    "QLabel "
                    "{background:transparent;border:0;font-family:'Inter';font-size:14px;color:%1;}"
                    "QLabel[fieldPart=\"HINT\"],QLabel[fieldPart=\"COUNT\"] {color:%2;}"
                    "QLabel[fieldPart=\"COUNT\"][countExceeded=\"true\"] {color:#e02128;}")
                .arg(foreground.name())
                .arg(colors[disabled ? "disabledText" : "placeholder"].toString());
    } else
        css = t.qss(t.recipe(d->chrome->property("surfacePreset").toString(),
                             d->chrome->property("fieldVariant").toString(),
                             d->chrome->visualState()),
                    d->chrome->controlSize());
    if (styleSheet() != css)
        setStyleSheet(css);
    const QString editorCss =
        captured ? QString("background:transparent;border:0;padding:0;min-height:0;color:%1;"
                           "font-family:'Inter','Noto Sans SC';font-size:14px;"
                           "lineedit-password-character:8226;lineedit-password-mask-delay:0;"
                           "selection-background-color:#6235f7;selection-color:white;")
                       .arg(foreground.name())
                 : QString();
    QFont editorFont(captured ? "Inter" : t.family());
    if (captured)
        editorFont.setFamilies({"Inter", "Noto Sans SC"});
    editorFont.setPixelSize(captured ? 14 : m["fontSize"].toInt());
    editorFont.setWeight(captured && !d->value.isEmpty() ? QFont::Medium : QFont::Normal);
    // Chrome has its own QSS scope. Set editor typography locally, not via an ancestor selector.
    for (auto* editor : QList<QWidget*>{d->line, d->area}) {
        if (editor->styleSheet() != editorCss)
            editor->setStyleSheet(editorCss);
        if (captured && editor->font() != editorFont)
            editor->setFont(editorFont);
        else if (!captured && d->line->property("figmaInputAppearance").toBool())
            editor->setFont(QFont()); // restore inherited legacy styling, not a new font override
    }
    if (d->line->property("figmaInputAppearance").toBool() != captured) {
        d->line->setProperty("figmaInputAppearance", captured);
        QEvent styleChange(QEvent::StyleChange);
        QApplication::sendEvent(d->line, &styleChange);
    }
    const int pad = m["paddingInline"].toInt() + (captured ? 1 : t.metric("borderWidth"));
    const bool area = d->kind == Kind::TextArea;
    d->row->setSpacing(captured ? metrics["gap"].toInt() : t.metric("gap"));
    d->outer->setSpacing(captured ? metrics["footerGap"].toInt() : t.metric("labelGap"));
    d->row->setContentsMargins(pad, area ? m["paddingBlock"].toInt() + t.metric("borderWidth") : 0,
                               pad, area ? m["paddingBlock"].toInt() + t.metric("borderWidth") : 0);
    d->line->setVisible(!area);
    d->area->setVisible(area);
    d->line->setFixedHeight(m["lineHeight"].toInt());
    // Consumers may replace the exposed native QTextDocument; never cast their layout blindly.
    if (auto* layout = dynamic_cast<InputDocumentLayout*>(d->area->document()->documentLayout()))
        layout->setCapturedLineHeight(
            captured
                ? metrics[d->value.isEmpty() ? "placeholderLineHeight" : "valueLineHeight"].toInt()
                : 0);
    d->label->setVisible(!d->label->text().isEmpty());
    d->hint->setVisible(!d->hint->text().isEmpty());
    d->count->setVisible(d->countVisible);
    d->footer->setVisible(!d->hint->text().isEmpty() || d->countVisible);
    QPalette p = d->line->palette();
    p.setColor(QPalette::Text, foreground);
    p.setColor(QPalette::Disabled, QPalette::Text, foreground);
    p.setColor(QPalette::PlaceholderText,
               captured ? QColor(colors[disabled ? "disabledText" : "placeholder"].toString())
                        : t.color(isEnabled() ? "placeholder" : "disabledText"));
    p.setColor(QPalette::Disabled, QPalette::PlaceholderText, p.color(QPalette::PlaceholderText));
    d->line->setPalette(p);
    d->area->setPalette(p);
    for (auto* w : {d->label, d->hint, d->count}) {
        QFont f(captured ? "Inter" : t.family());
        f.setPixelSize(captured ? 14 : t.metric(w == d->label ? "labelFontSize" : "hintFontSize"));
        w->setFont(f);
        auto pal = w->palette();
        pal.setColor(QPalette::WindowText, w == d->label ? foreground : t.color("muted"));
        w->setPalette(pal);
    }
    for (bool prefix : {true, false}) {
        auto* w = prefix ? d->prefix : d->suffix;
        const auto& text = prefix ? d->prefixText : d->suffixText;
        const auto& icon = prefix ? d->prefixIcon : d->suffixIcon;
        w->setVisible(!text.isEmpty() || !icon.isNull());
        if (!icon.isNull())
            w->setPixmap(
                field::iconPixmap(icon, t.metric("iconSize"), devicePixelRatioF(), foreground));
        else
            w->setText(text);
    }
    d->clear->setVisible(d->clearAllowed && !d->value.isEmpty() && !d->readOnly && isEnabled());
    d->password->setVisible(d->kind == Kind::Password);
    d->search->setVisible(d->kind == Kind::Search);
    d->search->setEnabled(!d->readOnly && !d->loading);
    d->password->setEnabled(isEnabled());
    const QString tone = disabled                              ? "Disabled"
                         : d->chrome->visualState() == "focus" ? "Accent"
                         : d->chrome->visualState() == "error" ? "Danger"
                                                               : "Primary";
    d->password->setFigmaGlyph(captured ? (d->password->isChecked() ? "EyeOff" : "Eye") + tone
                                        : "");
    d->clear->setFigmaGlyph(captured ? "ClosePrimary" : "");
    d->search->setFigmaGlyph(captured ? (d->loading ? "LoadingPrimary"
                                         : disabled ? "SearchDisabled"
                                                    : "SearchPrimary")
                                      : "");
    const int count = characterCount(d->value),
              limit = d->countLimit >= 0 ? d->countLimit : d->maximum;
    d->count->setText(limit >= 0 ? QString("%1 / %2").arg(count).arg(limit)
                                 : QString::number(count));
    const bool exceeded = limit >= 0 && count > limit;
    if (d->count->property("countExceeded").toBool() != exceeded) {
        d->count->setProperty("countExceeded", exceeded);
        d->count->style()->unpolish(d->count);
        d->count->style()->polish(d->count);
    }
    int visualLines = 0;
    qreal measuredTextHeight = 0;
    if (area) {
        auto* document = d->area->document();
        for (auto block = document->begin(); block.isValid() && visualLines < d->maxRows;
             block = block.next()) {
            document->documentLayout()->blockBoundingRect(block);
            // Fallback CJK fonts can be taller than the Latin Token line-height.
            // Native autoSize must fit real line boxes without changing the document.
            const int limit = d->autoSize ? d->maxRows : d->minRows;
            qreal visibleBlockHeight = 0;
            for (int i = 0; i < block.layout()->lineCount() && visualLines + i < limit; ++i) {
                const auto line = block.layout()->lineAt(i);
                // Negative leading affects the next line's origin, not the last line's extent.
                visibleBlockHeight = qMax(visibleBlockHeight, line.y() + line.height());
            }
            measuredTextHeight += visibleBlockHeight;
            visualLines += qMax(1, block.layout()->lineCount());
        }
    }
    int rows = d->autoSize ? qBound(d->minRows, visualLines, d->maxRows) : d->minRows;
    // QFrame's styled border insets are separate from the child layout's Token padding.
    const auto frameInsets = d->chrome->contentsMargins();
    d->chrome->setControlHeight(
        captured && area
            ? capturedSize["textAreaHeight"].toInt() + qMax(0, rows - 2) * 22
            :
            // Qt 5.15.2 _q_adjustScrollbars reserves one viewport pixel below the last line.
            area ? qMax(rows * m["lineHeight"].toInt(), int(std::ceil(measuredTextHeight)) + 1) +
                       2 * (m["paddingBlock"].toInt() + t.metric("borderWidth")) +
                       frameInsets.top() + frameInsets.bottom()
                 : d->chrome->controlHeight());
    setMinimumWidth(captured ? 0 : t.metric("fieldMinimumWidth"));
    d->syncing = false;
    updateGeometry();
}
} // namespace commonWidget
