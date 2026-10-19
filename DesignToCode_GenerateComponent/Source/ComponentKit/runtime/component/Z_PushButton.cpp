// Button V2 runtime. Values come only from generated V2 Token outputs.
#include "Z_PushButton.h"
#include "IconSource.h"
#include "FocusIndicator.h"
#include "Surface.h"
#include <QApplication>
#include <QElapsedTimer>
#include <QDynamicPropertyChangeEvent>
#include <QEvent>
#include <QFrame>
#include <QHBoxLayout>
#include <QLabel>
#include <QPainter>
#include <QPointer>
#include <QResizeEvent>
#include <QStyle>
#include <QStyleOptionButton>
#include <QTimer>
#include <QtMath>
#include <stdexcept>

namespace commonWidget {
namespace {
std::shared_ptr<const button_detail::Runtime> runtime;
const button_detail::Runtime& rt() {
    if (!runtime) throw std::logic_error("Install Button runtime first");
    return *runtime;
}
void applyDynamicProperty(QWidget* w, const char* name, const QVariant& value) {
    if (w->property(name) == value) return;
    w->setProperty(name, value);
    w->style()->unpolish(w);
    w->style()->polish(w);
    w->update();
}
void polish(QWidget* w) {
    w->style()->unpolish(w);
    w->style()->polish(w);
    w->update();
}
QString key(ButtonIntent v) {
    switch (v) {
    case ButtonIntent::Primary: return "primary";
    case ButtonIntent::Secondary: return "secondary";
    case ButtonIntent::Operation: return "operation";
    case ButtonIntent::Danger: return "danger";
    }
    throw std::invalid_argument("ButtonIntent");
}
QString key(ButtonAppearance v) {
    switch (v) {
    case ButtonAppearance::Filled: return "filled";
    case ButtonAppearance::Outlined: return "outlined";
    case ButtonAppearance::Dashed: return "dashed";
    case ButtonAppearance::Text: return "text";
    case ButtonAppearance::Link: return "link";
    }
    throw std::invalid_argument("ButtonAppearance");
}
QString key(ButtonSize v) {
    switch (v) {
    case ButtonSize::Small: return "small";
    case ButtonSize::Medium: return "medium";
    case ButtonSize::Large: return "large";
    }
    throw std::invalid_argument("ButtonSize");
}
QString key(IconContainerStyle v) {
    switch (v) {
    case IconContainerStyle::Filled: return "filled";
    case IconContainerStyle::Outlined: return "outlined";
    case IconContainerStyle::Bare: return "bare";
    }
    throw std::invalid_argument("IconContainerStyle");
}
QJsonValue token(const QString& path) {
    for (const auto& category : {"color", "layout", "number", "font", "boolean", "string", "asset"}) {
        const auto group = rt().values[category].toObject();
        if (group.contains(path)) return group[path];
    }
    throw std::invalid_argument(("Missing V2 Button token: " + path).toStdString());
}
qreal numberToken(const QString& path) {
    const auto value = token(path);
    if (!value.isDouble()) throw std::invalid_argument(("Non-numeric V2 Button token: " + path).toStdString());
    return value.toDouble();
}
QColor colorToken(const QString& path) {
    const QColor value = surface::color(token(path).toString());
    if (!value.isValid()) throw std::invalid_argument(("Invalid V2 Button color: " + path).toStdString());
    return value;
}
QString rgba(const QColor& color) {
    return QString("rgba(%1,%2,%3,%4)").arg(color.red()).arg(color.green()).arg(color.blue()).arg(color.alpha());
}
void part(QWidget* w, const char* value) {
    w->setObjectName(QString::fromLatin1(value));
    w->setAttribute(Qt::WA_TransparentForMouseEvents);
    w->setFocusPolicy(Qt::NoFocus);
    applyDynamicProperty(w, "buttonPart", QString::fromLatin1(value));
    auto policy = w->sizePolicy();
    policy.setRetainSizeWhenHidden(true);
    w->setSizePolicy(policy);
}
class Indicator final : public QLabel {
public:
    explicit Indicator(QWidget* parent) : QLabel(parent) {
        part(this, "LOADING");
        setObjectName("LoadingIndicator");
        connect(&timer, &QTimer::timeout, this, [this] {
            currentAngle = int((elapsed.elapsed() % rt().cycleMs) * 360 / rt().cycleMs);
            setProperty("spinnerAngle", currentAngle);
            update();
        });
    }
    void configure(const button_detail::Metrics& m, bool spinning) {
        setFixedSize(m.indicatorSize, m.indicatorSize);
        stroke = m.indicatorStroke;
        if (spinning && !timer.isActive()) {
            currentAngle = 0;
            setProperty("spinnerAngle", currentAngle);
            elapsed.start(); timer.start(rt().frameMs);
        } else if (!spinning) {
            timer.stop();
            currentAngle = 0;
            setProperty("spinnerAngle", currentAngle);
        }
        update();
    }
    int angle() const {
        return currentAngle;
    }
protected:
    void paintEvent(QPaintEvent*) override {
        QPainter p(this);
        p.setRenderHint(QPainter::Antialiasing);
        const auto foreground = palette().color(QPalette::WindowText);
        const qreal inset = stroke / 2.0;
        const QRectF bounds = QRectF(rect()).adjusted(inset,inset,-inset,-inset);
        auto track = foreground;
        track.setAlphaF(rt().spinnerTrackOpacity);
        p.setPen(QPen(track, stroke, Qt::SolidLine, Qt::RoundCap));
        p.drawEllipse(bounds);
        p.setPen(QPen(foreground, stroke, Qt::SolidLine, Qt::RoundCap));
        p.drawArc(bounds, qRound((rt().spinnerStartDegrees - currentAngle) * 16),
                  qRound(rt().spinnerSweepDegrees * 16));
    }
private:
    QTimer timer;
    QElapsedTimer elapsed;
    int currentAngle = 0;
    qreal stroke = 0;
};
}

namespace button_detail {
void installButtonRuntime(QApplication& app, const Runtime& input) {
    if (runtime) throw std::logic_error("Example bootstrap is install-once");
    if (!input.renderIcon || input.cycleMs <= 0 || input.frameMs <= 0 ||
        input.spinnerSweepDegrees <= 0 || input.spinnerSweepDegrees > 360 ||
        input.spinnerTrackOpacity < 0 || input.spinnerTrackOpacity > 1 || input.values.isEmpty())
        throw std::invalid_argument("Incomplete Button adapter");
    for (const auto& m : input.sizes) {
        if (m.minimumHeight <= 0 || m.paddingInline < 0 || m.paddingBlock < 0 ||
            m.gap < 0 || m.borderWidth < 0 || m.iconCanvas <= 0 ||
            m.iconContainer < m.iconCanvas || m.indicatorSize <= 0 ||
            m.indicatorStroke <= 0 || m.indicatorStroke >= m.indicatorSize ||
            m.iconOnlySize < qMax(m.iconContainer,m.indicatorSize))
            throw std::invalid_argument("Unapproved Button metrics");
    }
    runtime = std::make_shared<Runtime>(input);
    Q_UNUSED(app)
}
}

struct Z_PushButton::Impl {
    struct Slot {
        QWidget* host;
        QFrame* container;
        QLabel* glyph;
        QIcon icon;
        QString source;
        bool visible = true;
        IconContainerStyle style = IconContainerStyle::Bare;
    };
    ButtonIntent intent = ButtonIntent::Secondary;
    ButtonAppearance appearance = ButtonAppearance::Outlined;
    ButtonSize size = ButtonSize::Medium;
    bool loading = false, iconOnly = false;
    bool requestedEnabled = true, syncing = false, refreshQueued = false;
    Slot leading{}, trailing{};
    QWidget* content = nullptr;
    QLabel* label = nullptr;
    Indicator* indicator = nullptr;
    QHBoxLayout* layout = nullptr;
    QSize savedMinimum, savedMaximum, frozenSize;
    QSize textMinimum, textMaximum, textSize;
    QVector<QPointer<QWidget>> parents;
};

Z_PushButton::Z_PushButton(QWidget* parent) : QPushButton(parent), d(new Impl) {
    rt();
    setProperty("designSystemNative", true);
    setProperty("designSystemClass", "commonWidget::Z_PushButton");
    setProperty("nativeQtClass", "QPushButton");
    setCheckable(false);
    setAutoRepeat(false);
    setAutoDefault(false);
    setFocusPolicy(Qt::StrongFocus);
    d->layout = new QHBoxLayout(this);
    d->layout->setSizeConstraint(QLayout::SetNoConstraint);
    d->layout->addStretch();
    auto makeSlot = [this](const char* name) {
        Impl::Slot s;
        s.host = new QWidget(this); part(s.host, name);
        auto* box = new QHBoxLayout(s.host);
        box->setContentsMargins(0,0,0,0); box->setSpacing(0);
        s.container = new QFrame(s.host); part(s.container, "ICON_CONTAINER");
        auto* inner = new QHBoxLayout(s.container);
        inner->setContentsMargins(0,0,0,0); inner->setSpacing(0);
        s.glyph = new QLabel(s.container); part(s.glyph, "GLYPH");
        s.glyph->setAlignment(Qt::AlignCenter);
        inner->addWidget(s.glyph, 0, Qt::AlignCenter);
        box->addWidget(s.container);
        return s;
    };
    d->leading = makeSlot("LEADING");
    d->trailing = makeSlot("TRAILING");
    d->content = new QWidget(this); part(d->content, "CONTENT");
    auto* contentLayout = new QHBoxLayout(d->content);
    contentLayout->setContentsMargins(0,0,0,0); contentLayout->setSpacing(0);
    d->label = new QLabel(d->content); part(d->label, "LABEL");
    d->label->setTextFormat(Qt::PlainText);
    d->label->setAlignment(Qt::AlignCenter);
    contentLayout->addWidget(d->label);
    d->layout->addWidget(d->leading.host);
    d->layout->addWidget(d->content);
    d->layout->addWidget(d->trailing.host);
    d->layout->addStretch();
    d->indicator = new Indicator(this);
    watchParents(); sync();
    FocusIndicator::instance().watch(this);
}
Z_PushButton::~Z_PushButton() = default;

QString Z_PushButton::visualState() const {
    if (!d->requestedEnabled || (parentWidget() && !parentWidget()->isEnabled())) return "disabled";
    if (d->loading) return "loading";
    if (isDown()) return "pressed";
    if (FocusIndicator::instance().visibleFor(this)) return "focused";
    return underMouse() ? "hover" : "default";
}

QString Z_PushButton::contentMode() const {
    if (d->iconOnly) return "icon-only";
    const bool leading = d->leading.visible && !d->leading.icon.isNull();
    const bool trailing = d->trailing.visible && !d->trailing.icon.isNull();
    if (leading && trailing) return "icon-left-text-icon-right";
    if (leading) return "icon-left-text";
    if (trailing) return "text-icon-right";
    return "text-only";
}
void Z_PushButton::sync() {
    if (d->syncing) return;
    d->syncing = true;
    bool snapshotGeometry = property("figmaSnapshotGeometry").toBool();
    for (auto* ancestor = parentWidget(); ancestor && !snapshotGeometry;
         ancestor = ancestor->parentWidget())
        snapshotGeometry = ancestor->property("figmaDesignBounds").isValid();
    const auto previousState = property("buttonState").toString();
    const bool policyDisabled = !d->requestedEnabled ||
        (parentWidget() && !parentWidget()->isEnabled());
    const auto intent = key(d->intent);
    const auto appearance = key(d->appearance);
    const auto state = visualState();
    const auto content = contentMode();
    applyDynamicProperty(this,"buttonIntent",intent);
    applyDynamicProperty(this,"buttonAppearance",appearance);
    applyDynamicProperty(this,"buttonSize",key(d->size));
    applyDynamicProperty(this,"buttonContent",content);
    applyDynamicProperty(this,"buttonState",state);
    applyDynamicProperty(this,"loading",d->loading);
    applyDynamicProperty(this,"disabledByPolicy",policyDisabled);
    QPushButton::setEnabled(!policyDisabled && !d->loading);
    const auto& m = rt().sizes.at(static_cast<size_t>(d->size));
    const auto base = QString("component.button.intent.%1.appearance.%2.state.%3.content.%4.slot.")
                          .arg(intent, appearance, state, content);
    const auto background = colorToken(base + "container.background-color");
    const auto border = colorToken(base + "border.color");
    const auto label = colorToken(base + "label.color");
    const auto icon = colorToken(base + "icon.color");
    const auto spinner = colorToken(base + "spinner.color");
    const auto radius = numberToken("component.button.base.shared.slot.container.border-radius");
    const auto borderWidth = numberToken(base + "border.border-width");
    const auto borderStyle = appearance == "dashed" ? "dashed" : "solid";
    const auto fontFamily = token("component.button.base.typography.slot.label.font-family").toString();
    const auto fontSize = numberToken("component.button.base.typography.slot.label.font-size");
    const auto fontWeight = numberToken("component.button.base.typography.slot.label.font-weight");
    auto style = QString(
        "commonWidget--Z_PushButton {background:%1;border:%2px %3 %4;border-radius:%5px;padding:0;}"
        "commonWidget--Z_PushButton QLabel[buttonPart=\"LABEL\"] {color:%6;font-family:\"%7\";font-size:%8px;font-weight:%9;}"
        "commonWidget--Z_PushButton QLabel[buttonPart=\"GLYPH\"] {color:%10;}"
        "commonWidget--Z_PushButton QLabel[buttonPart=\"LOADING\"] {color:%11;}");
    for (const auto& replacement : QStringList{
             rgba(background), QString::number(borderWidth), borderStyle, rgba(border),
             QString::number(radius), rgba(label), fontFamily, QString::number(fontSize),
             QString::number(fontWeight), rgba(icon), rgba(spinner)})
        style = style.arg(replacement);
    style += stateStyleOverride(state);
    setStyleSheet(style);
    if (!d->loading) {
        const int horizontal = d->iconOnly ? m.borderWidth : m.paddingInline+m.borderWidth;
        const int vertical = d->iconOnly ? m.borderWidth : m.paddingBlock+m.borderWidth;
        d->layout->setContentsMargins(horizontal,vertical,horizontal,vertical);
        d->layout->setSpacing(d->iconOnly ? 0 : m.gap);
        if (!snapshotGeometry) {
            if (d->iconOnly) setFixedSize(m.iconOnlySize,m.iconOnlySize);
            else setMinimumHeight(m.minimumHeight);
        }
        d->label->setText(text());
        auto updateSlot = [&m](Impl::Slot& s) {
            s.container->setFixedSize(m.iconContainer,m.iconContainer);
            s.glyph->setFixedSize(m.iconCanvas,m.iconCanvas);
            auto policy=s.host->sizePolicy();
            policy.setRetainSizeWhenHidden(false); s.host->setSizePolicy(policy);
            s.host->setVisible(s.visible && !s.icon.isNull());
        };
        updateSlot(d->leading); updateSlot(d->trailing);
        if (d->iconOnly) {
            const bool leading = d->leading.visible && !d->leading.icon.isNull();
            d->leading.host->setVisible(leading);
            d->trailing.host->setVisible(!leading && d->trailing.visible && !d->trailing.icon.isNull());
        }
        auto policy=d->content->sizePolicy();
        policy.setRetainSizeWhenHidden(false); d->content->setSizePolicy(policy);
        d->content->setVisible(!d->iconOnly && !text().isEmpty());
    }
    applyDynamicProperty(d->leading.container,"iconContainerStyle",key(d->leading.style));
    applyDynamicProperty(d->trailing.container,"iconContainerStyle",key(d->trailing.style));
    d->indicator->configure(m,d->loading && !policyDisabled && isVisible());
    d->indicator->setVisible(d->loading);
    d->indicator->move((width()-d->indicator->width())/2,
                       (height()-d->indicator->height())/2);
    d->indicator->raise();
    d->syncing = false;
    refreshVisual(); updateGeometry(); update();
    if (previousState != state)
        emit visualStateChanged(state);
}

QString Z_PushButton::stateStyleOverride(const QString&) const { return {}; }
void Z_PushButton::refreshState() { sync(); }
void Z_PushButton::refreshVisual() {
    // Descendant palettes need the same refresh as the root selector.
    polish(this);
    for (auto* child : findChildren<QWidget*>()) polish(child);
    if (d->loading) return;
    const auto& m = rt().sizes.at(static_cast<size_t>(d->size));
    for (auto* s : {&d->leading,&d->trailing}) {
        if (!s->icon.isNull()) {
            const QColor color=s->glyph->palette().color(QPalette::WindowText);
            const auto mode=isEnabled()?QIcon::Normal:QIcon::Disabled;
            s->glyph->setPixmap(rt().renderIcon(s->icon,m.iconCanvas,color,
                                              mode,devicePixelRatioF()));
        }
    }
}
void Z_PushButton::watchParents() {
    for (const auto& p : d->parents) if (p) p->removeEventFilter(this);
    d->parents.clear();
    for (auto* p=parentWidget(); p; p=p->parentWidget()) {
        p->installEventFilter(this); d->parents.append(p);
    }
}
bool Z_PushButton::eventFilter(QObject*, QEvent* e) {
    if (e->type()==QEvent::EnabledChange || e->type()==QEvent::ParentChange)
        QTimer::singleShot(0,this,[this] { watchParents(); sync(); });
    return false;
}
bool Z_PushButton::event(QEvent* e) {
    const bool result=QPushButton::event(e);
    if (!d || d->syncing) return result;
    if (e->type()==QEvent::ParentChange) { watchParents(); sync(); }
    else if (e->type()==QEvent::Show || e->type()==QEvent::Hide) sync();
    else if (e->type()==QEvent::Enter || e->type()==QEvent::Leave ||
             e->type()==QEvent::MouseButtonPress || e->type()==QEvent::MouseButtonRelease ||
             e->type()==QEvent::KeyPress || e->type()==QEvent::KeyRelease ||
             e->type()==QEvent::FocusIn || e->type()==QEvent::FocusOut) {
        if (!d->refreshQueued) {
            d->refreshQueued=true;
            QTimer::singleShot(0,this,[this] {
                d->refreshQueued=false;
                refreshVisual();
            });
        }
    }
    return result;
}
void Z_PushButton::paintEvent(QPaintEvent*) {
    QStyleOptionButton option; initStyleOption(&option);
    const bool focusVisible = FocusIndicator::instance().visibleFor(this);
    if (!focusVisible)
        option.state &= ~QStyle::State_HasFocus;
    option.text.clear(); option.icon=QIcon();
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    style()->drawControl(QStyle::CE_PushButton,&option,&p,this);
    if (focusVisible) {
        const auto path = QString("component.button.intent.%1.appearance.%2.state.focused.content.%3.slot.focus-ring.border-width")
                              .arg(key(d->intent), key(d->appearance), contentMode());
        const qreal width = numberToken(path);
        const qreal radius = numberToken(
            "component.button.base.shared.slot.container.border-radius");
        p.setBrush(Qt::NoBrush);
        p.setPen(QPen(focusIndicatorColor(), width, Qt::SolidLine, Qt::RoundCap,
                      Qt::RoundJoin));
        const qreal inset = width / 2.0;
        p.drawRoundedRect(QRectF(rect()).adjusted(inset, inset, -inset, -inset),
                          qMax<qreal>(0, radius - inset),
                          qMax<qreal>(0, radius - inset));
    }
}
QColor Z_PushButton::focusIndicatorColor() const {
    const auto path = QString("component.button.intent.%1.appearance.%2.state.focused.content.%3.slot.focus-ring.color")
                          .arg(key(d->intent), key(d->appearance), contentMode());
    return FocusIndicator::instance().color(colorToken(path));
}
void Z_PushButton::resizeEvent(QResizeEvent* e) {
    QPushButton::resizeEvent(e);
    if (d && d->indicator)
        d->indicator->move((width()-d->indicator->width())/2,
                           (height()-d->indicator->height())/2);
}
QSize Z_PushButton::sizeHint() const {
    if (!d || !d->layout) return QPushButton::sizeHint();
    if (d->loading) return d->frozenSize;
    QSize s=d->layout->sizeHint();
    const auto& m=rt().sizes.at(static_cast<size_t>(d->size));
    if (d->iconOnly) return QSize(m.iconOnlySize,m.iconOnlySize);
    s.setHeight(qMax(s.height(),m.minimumHeight));
    return s.expandedTo(QSize(m.indicatorSize+2*(m.paddingInline+m.borderWidth),
                             m.indicatorSize+2*(m.paddingBlock+m.borderWidth)));
}
QSize Z_PushButton::minimumSizeHint() const { return sizeHint(); }
void Z_PushButton::setIntent(ButtonIntent v) { key(v); d->intent=v; sync(); }
void Z_PushButton::setAppearance(ButtonAppearance v) { key(v); d->appearance=v; sync(); }
void Z_PushButton::setType(ButtonType v) {
    switch (v) {
    case ButtonType::Primary: setIntent(ButtonIntent::Primary); setAppearance(ButtonAppearance::Filled); break;
    case ButtonType::Default: setIntent(ButtonIntent::Secondary); setAppearance(ButtonAppearance::Outlined); break;
    case ButtonType::Dashed: setIntent(ButtonIntent::Secondary); setAppearance(ButtonAppearance::Dashed); break;
    case ButtonType::Text: setIntent(ButtonIntent::Secondary); setAppearance(ButtonAppearance::Text); break;
    case ButtonType::Link: setIntent(ButtonIntent::Primary); setAppearance(ButtonAppearance::Link); break;
    }
}
void Z_PushButton::setDanger(bool v) { setIntent(v ? ButtonIntent::Danger : ButtonIntent::Secondary); }
void Z_PushButton::setGhost(bool v) { setAppearance(v ? ButtonAppearance::Text : ButtonAppearance::Outlined); }
void Z_PushButton::setSize(ButtonSize v) {
    key(v);
    if (v==d->size) return;
    if (d->loading) throw std::logic_error("Change size after Loading");
    d->size=v; sync();
}
void Z_PushButton::setEnabled(bool v) { d->requestedEnabled=v; sync(); }
void Z_PushButton::setText(const QString& v) { QPushButton::setText(v); sync(); }
void Z_PushButton::requireIconContent(const QIcon& leading, bool left, const QIcon& trailing, bool right) const {
    if (d->iconOnly && !(left && !leading.isNull()) && !(right && !trailing.isNull()))
        throw std::invalid_argument("Icon-only mode requires at least one visible icon");
}
void Z_PushButton::setLeadingIcon(const QIcon& v) {
    requireIconContent(v,d->leading.visible,d->trailing.icon,d->trailing.visible);
    d->leading.icon=v; d->leading.source.clear(); sync();
}
void Z_PushButton::setTrailingIcon(const QIcon& v) {
    requireIconContent(d->leading.icon,d->leading.visible,v,d->trailing.visible);
    d->trailing.icon=v; d->trailing.source.clear(); sync();
}
void Z_PushButton::setLeadingIconVisible(bool v) {
    requireIconContent(d->leading.icon,v,d->trailing.icon,d->trailing.visible);
    d->leading.visible=v; sync();
}
void Z_PushButton::setTrailingIconVisible(bool v) {
    requireIconContent(d->leading.icon,d->leading.visible,d->trailing.icon,v);
    d->trailing.visible=v; sync();
}
QString Z_PushButton::leadingIconSource() const { return d->leading.source; }
QString Z_PushButton::trailingIconSource() const { return d->trailing.source; }
void Z_PushButton::setLeadingIconSource(const QString& source) {
    const QString path=IconSource::resolve(source);
    const QIcon icon=IconSource::load(path);
    setLeadingIcon(icon); d->leading.source=source.trimmed();
}
void Z_PushButton::setTrailingIconSource(const QString& source) {
    const QString path=IconSource::resolve(source);
    const QIcon icon=IconSource::load(path);
    setTrailingIcon(icon); d->trailing.source=source.trimmed();
}
bool Z_PushButton::isIconOnly() const { return d->iconOnly; }
void Z_PushButton::setIconOnly(bool value) {
    if (value==d->iconOnly) return;
    if (d->loading) throw std::logic_error("Change content mode after Loading");
    if (value) {
        if (!(d->leading.visible && !d->leading.icon.isNull()) &&
            !(d->trailing.visible && !d->trailing.icon.isNull()))
            throw std::invalid_argument("Load an icon before entering icon-only mode");
        if (accessibleName().trimmed().isEmpty()) {
            if (text().trimmed().isEmpty()) throw std::invalid_argument("Icon-only mode requires accessibleName");
            setAccessibleName(text());
        }
        d->textMinimum=minimumSize(); d->textMaximum=maximumSize();
        d->textSize=isVisible()?size():sizeHint();
        d->iconOnly=true;
    } else {
        d->iconOnly=false;
        setMinimumSize(d->textMinimum); setMaximumSize(d->textMaximum);
    }
    sync();
    if (!value) resize(d->textSize.expandedTo(sizeHint()));
}
void Z_PushButton::setLeadingIconStyle(IconContainerStyle v) {
    key(v); d->leading.style=v; sync();
}
void Z_PushButton::setTrailingIconStyle(IconContainerStyle v) {
    key(v); d->trailing.style=v; sync();
}
void Z_PushButton::setLoading(bool v) {
    bool snapshotGeometry = property("figmaSnapshotGeometry").toBool();
    for (auto* ancestor = parentWidget(); ancestor && !snapshotGeometry;
         ancestor = ancestor->parentWidget())
        snapshotGeometry = ancestor->property("figmaDesignBounds").isValid();
    if (v==d->loading) return;
    if (v) {
        d->savedMinimum=minimumSize(); d->savedMaximum=maximumSize();
        d->frozenSize=isVisible()?size():sizeHint();
        for (auto* host : {d->leading.host,d->content,d->trailing.host}) {
            auto p=host->sizePolicy();
            // Only previously visible content reserves space.
            p.setRetainSizeWhenHidden(!host->isHidden()); host->setSizePolicy(p);
            host->hide();
        }
        d->loading=true; setDown(false);
        if (!snapshotGeometry)
            setFixedSize(d->frozenSize);
    } else {
        d->loading=false;
        if (!snapshotGeometry) {
            setMinimumSize(d->savedMinimum); setMaximumSize(d->savedMaximum);
        }
    }
    sync();
    if (!v) {
        if (!snapshotGeometry) {
            setMinimumSize(d->savedMinimum);
            setMaximumSize(d->savedMaximum);
        }
        updateGeometry();
    }
}
}
