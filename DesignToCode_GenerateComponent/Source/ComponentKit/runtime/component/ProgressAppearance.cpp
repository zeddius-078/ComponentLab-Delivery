#include "ProgressAppearance.h"
#include <QEvent>
#include <QElapsedTimer>
#include <QFile>
#include <QFontDatabase>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QPainter>
#include <QPainterPath>
#include <QProgressBar>
#include <QStyle>
#include <QTimer>
#include <QVariant>
#include <cmath>
#include <stdexcept>

namespace commonWidget {
namespace {
const QJsonArray& recipes() {
    static const QJsonArray data = [] {
        QFile file(":/figma/progress-visuals.json");
        if (!file.open(QIODevice::ReadOnly))
            throw std::runtime_error("Progress appearance evidence missing");
        return QJsonDocument::fromJson(file.readAll()).object()["variants"].toArray();
    }();
    return data;
}

QColor paintColor(const QJsonValue& paints) {
    const auto paint = paints.toArray().first().toObject();
    const auto color = paint["color"].toObject();
    return QColor::fromRgbF(color["r"].toDouble(), color["g"].toDouble(), color["b"].toDouble(),
                            paint["opacity"].toDouble(1));
}

class ProgressRenderer final : public QObject {
public:
    explicit ProgressRenderer(QProgressBar* bar) : QObject(bar), bar_(bar), timer_(this) {
        setObjectName("figmaProgressRenderer");
        bar->installEventFilter(this);
        timer_.setInterval(33);
        connect(&timer_, &QTimer::timeout, bar, [this] {
            if (!animate())
                timer_.stop();
            bar_->update();
        });
        clock_.start();
    }

protected:
    bool eventFilter(QObject*, QEvent* event) override {
        if (event->type() == QEvent::Hide)
            timer_.stop();
        if (event->type() != QEvent::Paint)
            return false;
        if (animate()) {
            if (!timer_.isActive())
                timer_.start();
        } else
            timer_.stop();
        draw();
        return true;
    }

private:
    bool busy() const {
        return bar_->minimum() == 0 && bar_->maximum() == 0;
    }

    bool animate() const {
        return busy() && bar_->isVisible() && bar_->isEnabled() &&
               !bar_->property("figmaProgressSnapshot").toBool() &&
               !bar_->property("reducedMotion").toBool();
    }

    void draw() {
        const bool vertical = bar_->orientation() == Qt::Vertical;
        const bool inverted = bar_->invertedAppearance();
        QString state = bar_->property("figmaProgressStatus").toString();
        if (busy())
            state = "Indeterminate";
        else if (!bar_->isEnabled())
            state = "Disabled";
        else if (state == "Disabled" || state == "Indeterminate")
            state = "Determinate";
        // Only the horizontal axis has source-backed semantic status variants.
        // Unsupported combinations use the native basic role, not invented tokens.
        if (vertical && state != "Indeterminate" && state != "Disabled")
            state = "Determinate";
        const QString key =
            QString("Orientation=%1, State=%2, Inverted=%3")
                .arg(vertical ? "Vertical" : "Horizontal", state, inverted ? "True" : "False");
        QJsonObject recipe;
        for (const auto& candidate : recipes())
            if (candidate.toObject()["name"] == key) {
                recipe = candidate.toObject();
                break;
            }
        if (recipe.isEmpty())
            return;
        QPainter p(bar_);
        p.setRenderHint(QPainter::Antialiasing);
        QPainterPath clip;
        clip.addRoundedRect(bar_->rect(), 4, 4);
        p.setClipPath(clip);
        // Horizontal responsive adaptation keeps the 92px label slot and 24px
        // authored gap; at 270x24 all coordinates equal the captured source.
        QRectF rail = vertical
                          ? QRectF(1, 1, qMax(0, bar_->width() - 2), qMax(0, bar_->height() - 2))
                          : QRectF(inverted ? 17 : 1, (bar_->height() - 8) / 2.0,
                                   qMax(0, bar_->width() - 120), 8);
        QRectF textRect = vertical ? QRectF(0, (bar_->height() - 36) / 2.0, bar_->width(), 36)
                                   : QRectF(bar_->width() - 94, 0, 92, bar_->height());
        if (!vertical && bar_->layoutDirection() == Qt::RightToLeft) {
            rail.moveLeft(bar_->width() - rail.right());
            textRect.moveLeft(2);
        }
        if (vertical) {
            p.setPen(QPen(paintColor(recipe["strokes"]), 1));
            p.setBrush(paintColor(recipe["fills"]));
            p.drawRoundedRect(QRectF(bar_->rect()).adjusted(.5, .5, -.5, -.5), 3.5, 3.5);
        }
        QJsonObject chunk, label;
        for (const auto& value : recipe["children"].toArray()) {
            const auto node = value.toObject();
            if (node["name"].toString().startsWith("Rail /")) {
                p.setPen(Qt::NoPen);
                p.setBrush(paintColor(node["fills"]));
                p.drawRoundedRect(rail, 4, 4);
            } else if (node["type"] == "TEXT")
                label = node;
            else
                chunk = node;
        }
        const qreal extent = vertical ? rail.height() : rail.width();
        const double span = double(bar_->maximum()) - double(bar_->minimum());
        const double fraction =
            span <= 0 ? 0
                      : qBound(0.0, (double(bar_->value()) - double(bar_->minimum())) / span, 1.0);
        qreal length = busy() ? qMin(64.0, extent) : qRound(extent * fraction);
        qreal start = 0;
        const bool reverse =
            vertical ? !inverted : inverted != (bar_->layoutDirection() == Qt::RightToLeft);
        if (busy()) {
            // The static review slice is centered exactly as in the source.
            // 1200ms live oscillation is deliberately documented as runtime-only.
            const double phase =
                animate()
                    ? .5 - .5 * std::cos((clock_.elapsed() % 1200) * 6.283185307179586 / 1200.0)
                    : .5;
            start = (extent - length) * (reverse ? 1 - phase : phase);
        } else if (reverse)
            start = extent - length;
        QRectF indicator = vertical
                               ? QRectF(rail.left(), rail.top() + start, rail.width(), length)
                               : QRectF(rail.left() + start, rail.top(), length, rail.height());
        p.setPen(Qt::NoPen);
        p.setBrush(paintColor(chunk["fills"]));
        if (length > 0)
            p.drawRoundedRect(indicator, 4, 4);
        if (!busy() && bar_->isTextVisible()) {
            p.setPen(paintColor(label["fills"]));
            p.setFont(bar_->font());
            const auto alignment = QStyle::visualAlignment(bar_->layoutDirection(), bar_->alignment());
            p.drawText(textRect, alignment | Qt::AlignVCenter | Qt::TextSingleLine, bar_->text());
        }
    }

    QProgressBar* bar_;
    QTimer timer_;
    QElapsedTimer clock_;
};
} // namespace

void applyFigmaProgressAppearance(QProgressBar* progress, const QString& status, bool snapshot) {
    if (!progress || !QStringList{"Determinate", "Indeterminate", "Disabled", "Paused", "Error",
                                  "In-progress", "Completed"}
                          .contains(status))
        throw std::invalid_argument("Unknown Figma Progress status");
    static const int fontId = QFontDatabase::addApplicationFont(":/figma/fonts/Inter-Medium.otf");
    Q_UNUSED(fontId);
    QFont font("Inter");
    font.setPixelSize(14);
    font.setWeight(QFont::Medium);
    progress->setFont(font);
    progress->setProperty("figmaAppearance", "figma-light");
    progress->setProperty("figmaProgressStatus", status);
    progress->setProperty("figmaProgressSnapshot", snapshot);
    progress->setStyleSheet(
        "QProgressBar {background:transparent;border:0;padding:0;min-height:0;"
        "font-family:'Inter';font-size:14px;font-weight:500;}");
    if (!progress->findChild<QObject*>("figmaProgressRenderer", Qt::FindDirectChildrenOnly))
        new ProgressRenderer(progress);
    progress->update();
}
}
