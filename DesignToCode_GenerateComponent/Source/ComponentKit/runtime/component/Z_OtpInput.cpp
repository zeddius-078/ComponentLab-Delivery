#include "Z_OtpInput.h"
#include <QHBoxLayout>
#include <QKeyEvent>
#include <QLineEdit>
#include <QSignalBlocker>
#include <stdexcept>

namespace commonWidget {
Z_OtpInput::Z_OtpInput(std::shared_ptr<const field::Tokens> input, int length, QWidget* parent)
    : QWidget(parent), tokens(std::move(input)) {
    if (length < 1 || length > 12)
        throw std::invalid_argument("OTP length must be 1..12");
    setObjectName("OTP_ROOT");
    auto* row = new QHBoxLayout(this);
    row->setContentsMargins(0, 0, 0, 0);
    row->setSpacing(tokens->metric("gap"));
    for (int i = 0; i < length; ++i) {
        auto* frame = new field::Chrome(tokens, this);
        frame->setObjectName(QString("CELL_%1").arg(i));
        auto* box = new QHBoxLayout(frame);
        int border = tokens->metric("borderWidth");
        box->setContentsMargins(border, 0, border, 0);
        box->setSpacing(0);
        auto* editor = new QLineEdit(frame);
        editor->setObjectName("EDITOR");
        editor->setProperty("fieldPart", "EDITOR");
        editor->setAlignment(Qt::AlignCenter);
        editor->setMinimumWidth(0);
        editor->setInputMethodHints(Qt::ImhDigitsOnly);
        editor->setAccessibleName(tr("Digit %1 of %2").arg(i + 1).arg(length));
        editor->installEventFilter(this);
        frame->setFocusProxy(editor);
        frame->setFocusPolicy(Qt::ClickFocus);
        box->addWidget(editor);
        row->addWidget(frame);
        frames.append(frame);
        cells.append(editor);
        values.append(QString());
        connect(editor, &QLineEdit::textEdited, this,
                [this, i](const QString& text) { edit(i, text); });
    }
    row->addStretch();
    setFocusProxy(cells.first());
    setControlSize("md");
}
Z_OtpInput::~Z_OtpInput() {
    for (auto* cell : cells) {
        cell->removeEventFilter(this);
        disconnect(cell, nullptr, this, nullptr);
    }
}
QString Z_OtpInput::value() const {
    return values.join(QString());
}
QList<QLineEdit*> Z_OtpInput::editors() const {
    return cells;
}
void Z_OtpInput::commit(const QStringList& next) {
    bool changed = next != values;
    values = next;
    for (int i = 0; i < cells.size(); ++i)
        if (cells[i]->text() != values[i]) {
            QSignalBlocker b(cells[i]);
            cells[i]->setText(values[i]);
        }
    if (changed) {
        emit valueChanged(value());
        if (value().size() == cells.size())
            emit completed(value());
    }
}
void Z_OtpInput::setValue(const QString& text) {
    if (text.size() > cells.size())
        throw std::invalid_argument("OTP value exceeds length");
    for (auto c : text)
        if (c < '0' || c > '9')
            throw std::invalid_argument("OTP accepts ASCII digits");
    QStringList next;
    for (int i = 0; i < cells.size(); ++i)
        next.append(i < text.size() ? QString(text[i]) : QString());
    commit(next);
}
void Z_OtpInput::edit(int index, const QString& text) {
    if (!isEnabled() || cells[index]->isReadOnly()) {
        commit(values);
        return;
    }
    QString digits;
    for (auto c : text)
        if (c >= '0' && c <= '9')
            digits += c;
    auto next = values;
    if (text.isEmpty())
        next[index].clear();
    else if (!digits.isEmpty())
        for (int j = 0; j < digits.size() && index + j < cells.size(); ++j)
            next[index + j] = QString(digits[j]);
    commit(next);
    if (!digits.isEmpty()) {
        auto* cell = cells[qMin(cells.size() - 1, index + digits.size())];
        cell->setFocus();
        cell->selectAll();
    }
}
bool Z_OtpInput::eventFilter(QObject* target, QEvent* event) {
    int i = cells.indexOf(qobject_cast<QLineEdit*>(target));
    if (i >= 0 && event->type() == QEvent::KeyPress) {
        auto key = static_cast<QKeyEvent*>(event)->key();
        int next = i;
        if (key == Qt::Key_Left)
            next = qMax(0, i - 1);
        else if (key == Qt::Key_Right)
            next = qMin(cells.size() - 1, i + 1);
        else if (key == Qt::Key_Backspace && cells[i]->text().isEmpty() && i > 0 &&
                 !cells[i]->isReadOnly()) {
            edit(i - 1, {});
            next = i - 1;
        }
        if (next != i) {
            cells[next]->setFocus();
            cells[next]->selectAll();
            return true;
        }
    }
    return QWidget::eventFilter(target, event);
}
void Z_OtpInput::setMasked(bool on) {
    for (auto* c : cells)
        c->setEchoMode(on ? QLineEdit::Password : QLineEdit::Normal);
}
void Z_OtpInput::setReadOnly(bool on) {
    for (auto* c : cells)
        c->setReadOnly(on);
}
void Z_OtpInput::setControlSize(const QString& size) {
    for (auto* f : frames) {
        f->setControlSize(size);
        f->setFixedWidth(f->controlHeight());
    }
    updateGeometry();
}
void Z_OtpInput::setStatus(const QString& v) {
    for (auto* f : frames)
        f->setStatus(v);
}
void Z_OtpInput::setVariant(const QString& v) {
    for (auto* f : frames)
        f->setVariant(v);
}
void Z_OtpInput::setAppearance(const QString& v) {
    for (auto* f : frames)
        f->setAppearance(v);
}
} // namespace commonWidget
