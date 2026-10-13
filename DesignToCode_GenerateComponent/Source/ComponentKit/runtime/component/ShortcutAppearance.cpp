#include "ShortcutAppearance.h"
#include "../surface/Surface.h"
#include <QAbstractButton>
#include <QEvent>
#include <QFontDatabase>
#include <QKeyEvent>
#include <QKeySequenceEdit>
#include <QLayout>
#include <QLineEdit>
#include <QPainter>
#include <QProxyStyle>
#include <QStyleFactory>
#include <QStyleOption>
#include <QSvgRenderer>
#include <QTimer>
#include <QVariant>
#include <stdexcept>

namespace commonWidget {
namespace {
QString visualState(const QKeySequenceEdit* edit) {
    if (!edit->isEnabled())
        return "Disabled";
    const auto fixed = edit->property("figmaShortcutSnapshotState").toString();
    if (!fixed.isEmpty())
        return fixed;
    if (edit->property("figmaShortcutRecording").toBool())
        return "Recording";
    if (!edit->property("figmaShortcutConflict").toString().isEmpty())
        return "Conflict";
    if (edit->hasFocus())
        return "Focus";
    return edit->keySequence().isEmpty() ? "Empty" : "Default";
}

QString hintText(const QKeySequenceEdit* edit) {
    const auto state = visualState(edit);
    if (state == "Disabled")
        return "Shortcut unavailable";
    if (state == "Recording")
        return QString::fromUtf8("Recording… · 1 s settle timer");
    if (state == "Conflict") {
        const auto message = edit->property("figmaShortcutConflict").toString();
        return message.isEmpty() ? QString::fromUtf8("Conflict · already bound to Save") : message;
    }
    if (state == "Focus")
        return edit->property("figmaShortcutSourceSnapshot").toBool()
                   ? QString::fromUtf8("Press shortcut · release to commit")
                   : QString::fromUtf8("Press shortcut · 1 s settle timer");
    return state == "Empty" ? QString::fromUtf8("Press shortcut · up to 4 chords")
                            : QString::fromUtf8("Up to 4 chords · Qt 5.15.2");
}

QFont sourceFont(int pixels, bool strong = false) {
    QFont font("Inter");
    font.setPixelSize(pixels);
    font.setWeight(strong ? QFont::DemiBold : QFont::Normal);
    return font;
}

class ShortcutClear final : public QAbstractButton {
public:
    explicit ShortcutClear(QWidget* parent) : QAbstractButton(parent) {
        setObjectName("figmaShortcutClear");
        setAccessibleName(tr("Clear shortcut"));
        setToolTip(tr("Clear shortcut"));
        setFocusPolicy(Qt::TabFocus);
        setFixedSize(24, 24);
    }

protected:
    void paintEvent(QPaintEvent*) override {
        QPainter painter(this);
        QSvgRenderer glyph(QString(isEnabled() ? ":/figma/shortcut/ClosePrimary.svg"
                                               : ":/figma/shortcut/CloseDisabled.svg"));
        glyph.render(&painter, QRectF(4, 4, 16, 16));
        if (hasFocus()) {
            painter.setPen(QPen(QColor("#6235f7"), 1));
            painter.setBrush(Qt::NoBrush);
            painter.drawRoundedRect(QRectF(1, 1, 22, 22), 11, 11);
        }
    }
};

class ShortcutHint final : public QWidget {
public:
    explicit ShortcutHint(QKeySequenceEdit* edit) : QWidget(edit), edit_(edit) {
        setObjectName("figmaShortcutHint");
        setAttribute(Qt::WA_TransparentForMouseEvents);
    }

protected:
    void paintEvent(QPaintEvent*) override {
        QPainter painter(this);
        painter.setFont(sourceFont(12));
        const auto state = visualState(edit_);
        painter.setPen(QColor(state == "Disabled"   ? "#8a8a8a"
                              : state == "Conflict" ? "#e02128"
                                                    : "#444444"));
        painter.drawText(
            rect(), Qt::AlignLeft | Qt::AlignVCenter,
            painter.fontMetrics().elidedText(hintText(edit_), Qt::ElideRight, width()));
    }

private:
    QKeySequenceEdit* edit_;
};

class ShortcutStyle final : public QProxyStyle {
public:
    ShortcutStyle(QKeySequenceEdit* edit, QLineEdit* line)
        : QProxyStyle(QStyleFactory::create("Fusion")), edit_(edit) {
        setParent(line);
    }

    QRect subElementRect(SubElement element, const QStyleOption* option,
                         const QWidget* widget) const override {
        if (element == SE_LineEditContents) {
            const int left = visualState(edit_) == "Recording" ? 26 : 10;
            const bool clear = edit_->property("figmaShortcutClearVisible").toBool();
            return option->rect.adjusted(left, 2, clear ? -34 : -10, 0);
        }
        return QProxyStyle::subElementRect(element, option, widget);
    }

    void drawPrimitive(PrimitiveElement element, const QStyleOption* option, QPainter* painter,
                       const QWidget* widget) const override {
        if (element != PE_PanelLineEdit) {
            QProxyStyle::drawPrimitive(element, option, painter, widget);
            return;
        }
        const auto state = visualState(edit_);
        const bool focus = state == "Focus" || state == "Recording";
        const qreal border = focus ? 2 : 1;
        painter->save();
        painter->setRenderHint(QPainter::Antialiasing);
        painter->setPen(QPen(QColor(state == "Disabled"   ? "#d0d0d0"
                                    : state == "Conflict" ? "#e02128"
                                    : focus               ? "#6235f7"
                                                          : "#a7a7a7"),
                             border));
        painter->setBrush(QColor(state == "Disabled"   ? "#f3f3f3"
                                 : state == "Conflict" ? "#fbe4e5"
                                                       : "#ffffff"));
        painter->drawRoundedRect(
            QRectF(option->rect).adjusted(border / 2, border / 2, -border / 2, -border / 2),
            6 - border / 2, 6 - border / 2);
        if (state == "Recording") {
            QSvgRenderer indicator(QStringLiteral(":/figma/shortcut/RecordingIndicator.svg"));
            indicator.render(painter, QRectF(12, 16, 8, 8));
        }
        if (edit_->property("figmaShortcutSourceSnapshot").toBool()) {
            const bool empty = state == "Empty";
            painter->setFont(sourceFont(14, !empty));
            painter->setPen(QColor(state == "Disabled" ? "#8a8a8a"
                                   : empty             ? "#444444"
                                                       : "#191919"));
            painter->drawText(
                QRect(state == "Recording" ? 28 : 12, 10, option->rect.width() - 52, 22),
                Qt::AlignLeft | Qt::AlignVCenter,
                empty ? QString("Press shortcut") : QString("Ctrl + Shift + P"));
        }
        painter->restore();
    }

private:
    QKeySequenceEdit* edit_;
};

class ShortcutBinding final : public QObject {
public:
    explicit ShortcutBinding(QKeySequenceEdit* edit) : QObject(edit), edit_(edit) {
        setObjectName("figmaShortcutBinding");
        line_ = edit->findChild<QLineEdit*>("qt_keysequenceedit_lineedit");
        if (!line_)
            throw std::runtime_error("Qt5 native shortcut editor missing");
        style_ = new ShortcutStyle(edit_, line_);
        line_->setStyle(style_);
        line_->setFixedHeight(40);
        if (auto* layout = edit_->layout()) {
            layout->setContentsMargins(0, 0, 0, 32);
            layout->setSpacing(0);
        }
        clear_ = new ShortcutClear(line_);
        hint_ = new ShortcutHint(edit_);
        // The authored focus outline extends outside the field/root. Installing
        // this on the child line edit would clip it at the native parent edge.
        shadow_ = new surface::OuterShadowEffect(edit_);
        edit_->setGraphicsEffect(shadow_);
        edit_->installEventFilter(this);
        line_->installEventFilter(this);
        QObject::connect(clear_, &QAbstractButton::clicked, this, [this] {
            edit_->clear();
            edit_->setProperty("figmaShortcutRecording", false);
            refresh();
        });
        QObject::connect(edit_, &QKeySequenceEdit::editingFinished, this, [this] {
            edit_->setProperty("figmaShortcutRecording", false);
            refresh();
        });
        QObject::connect(edit_, &QKeySequenceEdit::keySequenceChanged, this, [this] {
            if (!keyEventPending_)
                edit_->setProperty("figmaShortcutRecording", false);
            refresh();
        });
        QObject::connect(line_, &QLineEdit::textChanged, this, [this] {
            refresh();
            // Qt resetState updates text before restoring the placeholder and may
            // return without keySequenceChanged when the candidate is unchanged.
            QTimer::singleShot(0, this, [this] {
                refresh();
            });
        });
    }

    void refresh() {
        if (refreshing_)
            return;
        refreshing_ = true;
        if (!keyEventPending_ && edit_->property("figmaShortcutRecording").toBool() &&
            !line_->placeholderText().isEmpty())
            edit_->setProperty("figmaShortcutRecording", false);
        const auto state = visualState(edit_);
        const bool snapshot = edit_->property("figmaShortcutSourceSnapshot").toBool();
        const bool showClear = edit_->property("figmaShortcutShowClear").toBool() &&
                               (snapshot ? state != "Empty" : !edit_->keySequence().isEmpty());
        edit_->setProperty("figmaShortcutClearVisible", showClear);
        edit_->setProperty("figmaShortcutVisualState", state);
        edit_->setFixedHeight(72);
        clear_->move(qMax(0, line_->width() - 32), 8);
        clear_->setVisible(showClear);
        hint_->setGeometry(4, 50, qMax(0, edit_->width() - 8), 18);
        hint_->setVisible(edit_->property("figmaShortcutShowHint").toBool());
        hint_->setToolTip(hintText(edit_));
        hint_->setAccessibleName(hintText(edit_));
        const auto font = sourceFont(14, !edit_->keySequence().isEmpty());
        if (line_->font() != font)
            line_->setFont(font);
        QPalette palette = line_->palette();
        for (const auto group : {QPalette::Active, QPalette::Inactive, QPalette::Disabled}) {
            palette.setColor(group, QPalette::Text,
                             QColor(group == QPalette::Disabled ? "#8a8a8a" : "#191919"));
            palette.setColor(group, QPalette::PlaceholderText,
                             QColor(group == QPalette::Disabled ? "#8a8a8a" : "#444444"));
            palette.setColor(group, QPalette::Highlight, QColor("#6235f7"));
            palette.setColor(group, QPalette::HighlightedText, Qt::white);
        }
        if (line_->palette() != palette)
            line_->setPalette(palette);
        surface::Recipe recipe;
        recipe.radius = 6;
        recipe.borderWidth = state == "Focus" || state == "Recording" ? 2 : 1;
        if (state == "Focus" || state == "Recording")
            recipe.shadows.append(QJsonObject{{"kind", "outer"},
                                              {"x", 0},
                                              {"y", 0},
                                              {"blur", 0},
                                              {"spread", 2},
                                              {"color", "#f3efff"},
                                              {"alpha", 1}});
        if (state == "Focus")
            recipe.shadows.append(QJsonObject{{"kind", "outer"},
                                              {"x", 0},
                                              {"y", 0},
                                              {"blur", 4},
                                              {"spread", 1},
                                              {"color", "#6136f7"},
                                              {"alpha", .14}});
        shadow_->setSurfaceRect(line_->rect());
        shadow_->setRecipe(recipe);
        line_->update();
        hint_->update();
        clear_->update();
        refreshing_ = false;
    }

protected:
    bool eventFilter(QObject* watched, QEvent* event) override {
        if (watched == line_ && event->type() == QEvent::Paint &&
            edit_->property("figmaShortcutSourceSnapshot").toBool()) {
            // Only inert source specimens paint the authored TEXT value. The
            // native editor/value remain present; live painting is untouched.
            QPainter painter(line_);
            QStyleOptionFrame option;
            option.initFrom(line_);
            style_->drawPrimitive(QStyle::PE_PanelLineEdit, &option, &painter, line_);
            return true;
        }
        // Also catches resetState with an empty/unchanged native line text. The
        // existing Qt5 child is observed only; neither its text nor timer is set.
        if (watched == line_ && event->type() == QEvent::Paint && !keyEventPending_ &&
            edit_->property("figmaShortcutRecording").toBool() &&
            !line_->placeholderText().isEmpty()) {
            edit_->setProperty("figmaShortcutRecording", false);
            QTimer::singleShot(0, this, [this] {
                refresh();
            });
        }
        if (watched == edit_ && event->type() == QEvent::KeyPress &&
            !static_cast<QKeyEvent*>(event)->isAutoRepeat()) {
            // Observe, never consume or reimplement the native parser/timer.
            keyEventPending_ = true;
            edit_->setProperty("figmaShortcutRecording", true);
            QTimer::singleShot(0, this, [this] {
                keyEventPending_ = false;
                refresh();
            });
        }
        switch (event->type()) {
        case QEvent::FocusIn:
        case QEvent::FocusOut:
        case QEvent::Resize:
        case QEvent::EnabledChange:
        case QEvent::StyleChange:
            QTimer::singleShot(0, this, [this] {
                refresh();
            });
            break;
        default:
            break;
        }
        return false;
    }

private:
    QKeySequenceEdit* edit_;
    QLineEdit* line_;
    ShortcutStyle* style_;
    ShortcutClear* clear_;
    ShortcutHint* hint_;
    surface::OuterShadowEffect* shadow_;
    bool refreshing_ = false, keyEventPending_ = false;
};
}

void applyFigmaShortcutAppearance(QKeySequenceEdit* edit, const QString& snapshotState,
                                  bool showClear, bool showHint, bool sourceSnapshot) {
    if (!edit || !QStringList{"", "Default", "Focus", "Recording", "Conflict", "Empty"}.contains(
                     snapshotState))
        throw std::invalid_argument(
            "Captured ShortcutInput requires native QKeySequenceEdit and a source-backed state");
    static const bool fonts = [] {
        QFontDatabase::addApplicationFont(":/figma/fonts/Inter-Regular.otf");
        QFontDatabase::addApplicationFont(":/figma/fonts/Inter-SemiBold.otf");
        return true;
    }();
    Q_UNUSED(fonts);
    edit->setProperty("figmaAppearance", "figma-light");
    edit->setProperty("designSystemStyleOwner", "figma-shortcut-captured");
    edit->setProperty("figmaShortcutSnapshotState", snapshotState);
    edit->setProperty("figmaShortcutShowClear", showClear);
    edit->setProperty("figmaShortcutShowHint", showHint);
    edit->setProperty("figmaShortcutSourceSnapshot", sourceSnapshot);
    auto* binding = dynamic_cast<ShortcutBinding*>(
        edit->findChild<QObject*>("figmaShortcutBinding", Qt::FindDirectChildrenOnly));
    if (!binding) {
        edit->style()->unpolish(edit);
        edit->style()->polish(edit);
        binding = new ShortcutBinding(edit);
        edit->ensurePolished();
    }
    binding->refresh();
}

void setFigmaShortcutConflict(QKeySequenceEdit* edit, const QString& message) {
    if (!edit)
        throw std::invalid_argument("Shortcut conflict requires native QKeySequenceEdit");
    edit->setProperty("figmaShortcutConflict", message);
    if (auto* binding = dynamic_cast<ShortcutBinding*>(
            edit->findChild<QObject*>("figmaShortcutBinding", Qt::FindDirectChildrenOnly)))
        binding->refresh();
}
}
