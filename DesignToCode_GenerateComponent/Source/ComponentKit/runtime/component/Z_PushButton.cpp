// Button Contract implementation. Public integration is described in component/README.md.
#include "Z_PushButton.h"
#include "IconSource.h"
#include "FocusIndicator.h"
#include <QApplication>
#include <QElapsedTimer>
#include <QDynamicPropertyChangeEvent>
#include <QFile>
#include <QFontDatabase>
#include <QFontMetricsF>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QPainterPath>
#include <QSvgRenderer>
#include <QEvent>
#include <QFrame>
#include <QHBoxLayout>
#include <QLabel>
#include <QPainter>
#include <QPointer>
#include <QRegularExpression>
#include <QResizeEvent>
#include <QStyle>
#include <QStyleOptionButton>
#include <QTimer>
#include <QtMath>
#include <stdexcept>

namespace commonWidget {
namespace {
const QJsonObject& figmaButtonVisuals() {
    static const QJsonObject data = [] {
        QFile file(":/figma/button-visuals.json");
        if (!file.open(QIODevice::ReadOnly))
            throw std::runtime_error("Captured Button appearance missing");
        QFontDatabase::addApplicationFont(":/figma/fonts/Inter-Regular.otf");
        return QJsonDocument::fromJson(file.readAll()).object();
    }();
    return data;
}
QColor capturedColor(const QJsonValue& value) {
    const auto a=value.toArray();
    return QColor(a[0].toInt(),a[1].toInt(),a[2].toInt(),qRound(a[3].toDouble()*255));
}
QFont capturedButtonFont(bool large) {
    figmaButtonVisuals();
    QFont font("Inter"); font.setPixelSize(large?16:14); font.setWeight(QFont::Normal);
    return font;
}
void drawCapturedSvg(QPainter& painter,const QString& name,const QRectF& bounds) {
    QSvgRenderer svg(":/figma/button/"+name+".svg");
    svg.render(&painter,bounds);
}
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
QString key(ButtonType v) {
    switch (v) {
    case ButtonType::Primary: return "PRIMARY";
    case ButtonType::Default: return "DEFAULT";
    case ButtonType::Dashed: return "DASHED";
    case ButtonType::Text: return "TEXT";
    case ButtonType::Link: return "LINK";
    }
    throw std::invalid_argument("ButtonType");
}
QString key(ButtonSize v) {
    switch (v) {
    case ButtonSize::XS: return "XS";
    case ButtonSize::S: return "S";
    case ButtonSize::M: return "M";
    case ButtonSize::L: return "L";
    }
    throw std::invalid_argument("ButtonSize");
}
QString key(IconContainerStyle v) {
    switch (v) {
    case IconContainerStyle::Filled: return "FILLED";
    case IconContainerStyle::Outlined: return "OUTLINED";
    case IconContainerStyle::Bare: return "BARE";
    }
    throw std::invalid_argument("IconContainerStyle");
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
            update();
            if(parentWidget()->property("figmaAppearance").toBool()) parentWidget()->update();
        });
    }
    void configure(const button_detail::Metrics& m, bool spinning) {
        setFixedSize(m.indicatorSize, m.indicatorSize);
        stroke = m.indicatorStroke;
        if (spinning && !timer.isActive()) {
            elapsed.start(); timer.start(rt().frameMs);
        } else if (!spinning) timer.stop();
        update();
    }
    int angle() const {
        return timer.isActive()?int((elapsed.elapsed()%rt().cycleMs)*360/rt().cycleMs):0;
    }
protected:
    void paintEvent(QPaintEvent*) override {
        QPainter p(this);
        p.setRenderHint(QPainter::Antialiasing);
        p.setPen(QPen(palette().color(QPalette::WindowText), stroke,
                      Qt::SolidLine, Qt::RoundCap));
        const qreal inset = stroke / 2.0;
        const QRectF bounds = QRectF(rect()).adjusted(inset,inset,-inset,-inset);
        const int angle = timer.isActive()
            ? int((elapsed.elapsed() % rt().cycleMs) * 360 / rt().cycleMs) : 0;
        p.drawArc(bounds, -angle * 16, rt().sweepDegrees * 16);
    }
private:
    QTimer timer;
    QElapsedTimer elapsed;
    qreal stroke = 0;
};
}

namespace button_detail {
QString renderButtonQss(const QString& source,
                       const QVector<QMap<QString, QString>>& rows) {
    if (source.isEmpty() || rows.isEmpty())
        throw std::invalid_argument("Missing generated Button QSS input");
    QString result;
    const QRegularExpression unresolved("@[A-Za-z][A-Za-z0-9]*@");
    const QRegularExpression emptyValue(":\\s*;");
    for (const auto& row : rows) {
        QString rule = source;
        for (auto it = row.cbegin(); it != row.cend(); ++it) {
            if (it.value().trimmed().isEmpty())
                throw std::invalid_argument("Empty QSS field");
            rule.replace("@" + it.key() + "@", it.value());
        }
        if (unresolved.match(rule).hasMatch() || emptyValue.match(rule).hasMatch())
            throw std::invalid_argument("Unresolved or empty QSS declaration");
        result += rule + '\n';
    }
    return result;
}
void installButtonRuntime(QApplication& app, const Runtime& input) {
    if (runtime) throw std::logic_error("Example bootstrap is install-once");
    if (!input.renderIcon || input.cycleMs <= 0 || input.frameMs <= 0 ||
        input.sweepDegrees <= 0 || input.sweepDegrees > 360)
        throw std::invalid_argument("Incomplete Button adapter");
    for (const auto& m : input.sizes) {
        if (m.minimumHeight <= 0 || m.paddingInline < 0 || m.paddingBlock < 0 ||
            m.gap < 0 || m.borderWidth < 0 || m.iconCanvas <= 0 ||
            m.iconContainer < m.iconCanvas || m.indicatorSize <= 0 ||
            m.indicatorStroke <= 0 || m.indicatorStroke >= m.indicatorSize ||
            m.iconOnlySize < 32 || m.iconOnlySize < qMax(m.iconContainer,m.indicatorSize)+2*m.borderWidth)
            throw std::invalid_argument("Unapproved Button metrics");
    }
    const QString qss = input.generatedQss.isEmpty()
        ? renderButtonQss(input.qssTemplate, input.qssRows) : input.generatedQss;
    if (qss.trimmed().isEmpty() || QRegularExpression("@[A-Za-z][A-Za-z0-9]*@").match(qss).hasMatch())
        throw std::invalid_argument("Invalid generated QSS");
    runtime = std::make_shared<Runtime>(input);
    app.setStyleSheet(app.styleSheet() + '\n' + qss);
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
    ButtonType type = ButtonType::Default;
    ButtonSize size = ButtonSize::M;
    bool danger = false, ghost = false, loading = false, iconOnly = false;
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
    if (!d->requestedEnabled || (parentWidget() && !parentWidget()->isEnabled())) return "DISABLED";
    if (d->loading) return "DEFAULT";
    if (isDown()) return "PRESSED";
    return underMouse() ? "HOVER" : "DEFAULT";
}

Z_PushButton::Recipe Z_PushButton::resolveRecipe() const {
    Recipe r{ButtonColor::Neutral, ButtonVariant::Outlined};
    switch (d->type) {
    case ButtonType::Primary: r={ButtonColor::Primary,ButtonVariant::Solid}; break;
    case ButtonType::Default: break;
    case ButtonType::Dashed: r.variant=ButtonVariant::Dashed; break;
    case ButtonType::Text: r.variant=ButtonVariant::Text; break;
    case ButtonType::Link: r={ButtonColor::Primary,ButtonVariant::Link}; break;
    }
    if (d->danger) r.color = ButtonColor::Danger;
    return r;
}
void Z_PushButton::sync() {
    if (d->syncing) return;
    d->syncing = true;
    const auto r = resolveRecipe();
    const char* colors[] = {"PRIMARY","NEUTRAL","DANGER"};
    const char* variants[] = {"SOLID","OUTLINED","DASHED","TEXT","LINK"};
    const bool policyDisabled = !d->requestedEnabled ||
        (parentWidget() && !parentWidget()->isEnabled());
    applyDynamicProperty(this,"buttonType",key(d->type));
    applyDynamicProperty(this,"buttonColor",QString::fromLatin1(colors[int(r.color)]));
    applyDynamicProperty(this,"buttonVariant",QString::fromLatin1(variants[int(r.variant)]));
    applyDynamicProperty(this,"buttonSize",key(d->size));
    applyDynamicProperty(this,"danger",d->danger);
    applyDynamicProperty(this,"ghost",d->ghost);
    applyDynamicProperty(this,"loading",d->loading);
    applyDynamicProperty(this,"disabledByPolicy",policyDisabled);
    QPushButton::setEnabled(!policyDisabled && !d->loading);
    const auto& m = rt().sizes.at(static_cast<size_t>(d->size));
    if (!d->loading) {
        const int horizontal = d->iconOnly ? m.borderWidth : m.paddingInline+m.borderWidth;
        const int vertical = d->iconOnly ? m.borderWidth : m.paddingBlock+m.borderWidth;
        d->layout->setContentsMargins(horizontal,vertical,horizontal,vertical);
        d->layout->setSpacing(d->iconOnly ? 0 : m.gap);
        if (d->iconOnly) setFixedSize(m.iconOnlySize,m.iconOnlySize);
        else setMinimumHeight(m.minimumHeight);
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
    d->indicator->configure(m,d->loading && !policyDisabled && isVisible() && property("figmaInputGlyph").toString().isEmpty() &&
                             property("figmaPreviewState").toString().isEmpty());
    d->indicator->setVisible(d->loading);
    d->indicator->move((width()-d->indicator->width())/2,
                       (height()-d->indicator->height())/2);
    d->indicator->raise();
    if(hasFigmaAppearance() || !property("figmaInputGlyph").toString().isEmpty()) {
        d->leading.host->hide(); d->content->hide(); d->trailing.host->hide();
        d->indicator->hide();
        if(!property("figmaInputGlyph").toString().isEmpty()) setFixedSize(32,32);
    }
    d->syncing = false;
    refreshVisual(); updateGeometry(); update();
}
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
    if(e->type()==QEvent::DynamicPropertyChange) {
        const auto name=static_cast<QDynamicPropertyChangeEvent*>(e)->propertyName();
        if(name=="figmaAppearance" || name=="figmaPreviewState" || name=="figmaInputGlyph") sync();
    }
    if (e->type()==QEvent::ParentChange) { watchParents(); sync(); }
    else if (e->type()==QEvent::Show || e->type()==QEvent::Hide) sync();
    else if (e->type()==QEvent::Enter || e->type()==QEvent::Leave ||
             e->type()==QEvent::MouseButtonPress || e->type()==QEvent::MouseButtonRelease ||
             e->type()==QEvent::KeyPress || e->type()==QEvent::KeyRelease ||
             e->type()==QEvent::FocusIn || e->type()==QEvent::FocusOut) {
        if (!d->refreshQueued) {
            d->refreshQueued=true;
            QTimer::singleShot(0,this,[this] {
                d->refreshQueued=false; refreshVisual();
            });
        }
    }
    return result;
}
void Z_PushButton::paintEvent(QPaintEvent*) {
    if(hasFigmaAppearance()) {
        QPainter painter(this); paintFigmaAppearance(painter); return;
    }
    QStyleOptionButton option; initStyleOption(&option);
    if (!FocusIndicator::instance().visibleFor(this))
        option.state &= ~QStyle::State_HasFocus;
    option.text.clear(); option.icon=QIcon();
    QPainter p(this);
    style()->drawControl(QStyle::CE_PushButton,&option,&p,this);
}
bool Z_PushButton::hasFigmaAppearance() const {
    // A captured Light recipe is explicit, not a substitute for global Theme/Profile.
    // Ghost and non-primary Danger are not authored in these seven Figma sets.
    return property("figmaAppearance").toBool() && !d->ghost &&
           (!d->danger || d->type==ButtonType::Primary);
}
QString Z_PushButton::figmaState() const {
    if(!d->requestedEnabled || (parentWidget() && !parentWidget()->isEnabled())) return "disabled";
    if(d->loading) return "loading";
    const auto preview=property("figmaPreviewState").toString().toLower();
    if(QStringList{"default","hover","active","focus-visible"}.contains(preview)) return preview;
    if(isDown()) return "active";
    if(FocusIndicator::instance().visibleFor(this)) return "focus-visible";
    return underMouse()?"hover":"default";
}
void Z_PushButton::paintFigmaAppearance(QPainter& painter) {
    const auto state=figmaState();
    const auto family=d->danger?QString("DANGER"):key(d->type);
    const auto recipe=figmaButtonVisuals()["recipes"].toObject()[family].toObject()[state].toObject();
    const qreal stroke=recipe["strokeWidth"].toDouble();
    const qreal radius=recipe["radius"].toDouble();
    // Composite opacity once, including overlapping fill/stroke/icon edges.
    QPixmap layer(size()*devicePixelRatioF()); layer.setDevicePixelRatio(devicePixelRatioF());
    layer.fill(Qt::transparent);
    QPainter p(&layer); p.setRenderHint(QPainter::Antialiasing);
    QPainterPath path; path.addRoundedRect(QRectF(rect()).adjusted(.5*stroke,.5*stroke,-.5*stroke,-.5*stroke),
                                           qMax(0.,radius-.5*stroke),qMax(0.,radius-.5*stroke));
    QPen pen(capturedColor(recipe["stroke"]),stroke);
    pen.setCapStyle(Qt::FlatCap);
    QVector<qreal> dash; for(const auto& v:recipe["dash"].toArray()) dash.append(v.toDouble()/stroke);
    if(!dash.isEmpty()) pen.setDashPattern(dash);
    p.setPen(pen);p.setBrush(capturedColor(recipe["fill"]));p.drawPath(path);
    if(d->loading) {
        QString asset=d->danger?"loading-danger":d->type==ButtonType::Primary?"loading-primary":
                      d->type==ButtonType::Link?"loading-link":"loading-default";
        QRectF slot((width()-16.)/2,(height()-16.)/2,16,16);
        // Authored text-button loading slots are offset inside the 18/20/24 canvas.
        // Preserve their captured location in the state gallery, but center live animation.
        if(!d->iconOnly && !property("figmaPreviewState").toString().isEmpty()) {
            const auto sizeKey=d->size==ButtonSize::L?"large":d->size==ButtonSize::M?"medium":"small";
            const auto a=figmaButtonVisuals()["loadingSlots"].toObject()[sizeKey].toArray();
            slot=QRectF(a[0].toDouble(),a[1].toDouble(),a[2].toDouble(),a[3].toDouble());
        }
        p.save();p.translate(slot.center());p.rotate(d->indicator->angle());
        drawCapturedSvg(p,asset,QRectF(-8,-8,16,16));p.restore();
    } else if(d->iconOnly && d->leading.source==":/figma/button/plus-default.svg") {
        const auto suffix=QStringList{"hover","active","disabled"}.contains(state)?state:QString("default");
        drawCapturedSvg(p,"plus-"+suffix,QRectF((width()-16.)/2,(height()-16.)/2,16,16));
    } else {
        const auto font=capturedButtonFont(d->size==ButtonSize::L);p.setFont(font);
        const auto foreground=capturedColor(recipe["foreground"]);p.setPen(foreground);
        const bool left=d->leading.visible && !d->leading.icon.isNull();
        const bool right=d->trailing.visible && !d->trailing.icon.isNull() && (!d->iconOnly || !left);
        const qreal textWidth=d->iconOnly?0:QFontMetricsF(font).horizontalAdvance(text());
        qreal content=textWidth+(left?16:0)+(right?16:0);
        if(textWidth>0)content+=(left?4:0)+(right?4:0);
        else if(left && right)content+=4;
        qreal x=(width()-content)/2.;
        const auto icon=[&](const Impl::Slot& s) {
            const auto pix=rt().renderIcon(s.icon,16,foreground,isEnabled()?QIcon::Normal:QIcon::Disabled,devicePixelRatioF());
            p.drawPixmap(QRectF(x,(height()-16.)/2,16,16),pix,QRectF(pix.rect()));x+=20;
        };
        if(left)icon(d->leading);
        if(textWidth>0) {p.drawText(QRectF(x,0,textWidth,height()),Qt::AlignCenter,text());x+=textWidth+4;}
        if(right)icon(d->trailing);
    }
    p.end();painter.setOpacity(recipe["opacity"].toDouble(1));painter.drawPixmap(0,0,layer);
}
QColor Z_PushButton::focusIndicatorColor() const {
    const auto source = d->danger ? rt().dangerFocusColor
                        : d->ghost ? rt().ghostFocusColor : rt().focusColor;
    return FocusIndicator::instance().color(source);
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
    if(hasFigmaAppearance()) {
        const bool large=d->size==ButtonSize::L;
        if(d->iconOnly) return QSize(large?40:32,large?40:32);
        qreal width=QFontMetricsF(capturedButtonFont(large)).horizontalAdvance(text());
        for(const auto* slot:{&d->leading,&d->trailing})
            if(slot->visible && !slot->icon.isNull()) width+=16+(text().isEmpty()?0:4);
        const int padding=large?16:d->size==ButtonSize::M?12:8;
        return QSize(qCeil(width)+2*padding,large?40:32);
    }
    QSize s=d->layout->sizeHint();
    const auto& m=rt().sizes.at(static_cast<size_t>(d->size));
    if (d->iconOnly) return QSize(m.iconOnlySize,m.iconOnlySize);
    s.setHeight(qMax(s.height(),m.minimumHeight));
    return s.expandedTo(QSize(m.indicatorSize+2*(m.paddingInline+m.borderWidth),
                             m.indicatorSize+2*(m.paddingBlock+m.borderWidth)));
}
QSize Z_PushButton::minimumSizeHint() const { return sizeHint(); }
void Z_PushButton::setType(ButtonType v) { key(v); d->type=v; sync(); }
void Z_PushButton::setDanger(bool v) { d->danger=v; sync(); }
void Z_PushButton::setGhost(bool v) { d->ghost=v; sync(); }
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
    if (v==d->loading) return;
    if (v) {
        d->savedMinimum=minimumSize(); d->savedMaximum=maximumSize();
        d->frozenSize=isVisible()?size():sizeHint();
        if(hasFigmaAppearance()) d->frozenSize=d->frozenSize.expandedTo(minimumSize()).boundedTo(maximumSize());
        for (auto* host : {d->leading.host,d->content,d->trailing.host}) {
            auto p=host->sizePolicy();
            // Only previously visible content reserves space.
            p.setRetainSizeWhenHidden(!host->isHidden()); host->setSizePolicy(p);
            host->hide();
        }
        d->loading=true; setDown(false); setFixedSize(d->frozenSize);
    } else {
        d->loading=false;
        setMinimumSize(d->savedMinimum); setMaximumSize(d->savedMaximum);
    }
    sync();
    if (!v) {
        setMinimumSize(d->savedMinimum);
        setMaximumSize(d->savedMaximum);
        updateGeometry();
    }
}
void Z_PushButton::SetPrimaryButton() {
    d->type=ButtonType::Primary; d->danger=false; d->ghost=false; sync();
}
void Z_PushButton::SetSecondaryButton() {
    d->type=ButtonType::Default; d->danger=false; d->ghost=false; sync();
}
void Z_PushButton::SetDangerousButton() {
    d->type=ButtonType::Primary; d->danger=true; d->ghost=false; sync();
}
void Z_PushButton::applyLegacy(const QString& name) {
    if (!rt().legacyRecipe) throw std::logic_error("Legacy mapping not approved");
    const auto r=rt().legacyRecipe(name);
    key(r.type); key(r.iconStyle);
    d->type=r.type; d->danger=r.danger; d->ghost=r.ghost;
    d->leading.style=r.iconStyle; d->trailing.style=r.iconStyle;
    sync();
}
void Z_PushButton::SetOperateIco(int index) {
    if (index<0 || index>2) return;
    if (!rt().legacyOperationIcon) throw std::logic_error("Operation assets missing");
    applyLegacy("SetOperateIco");
    setText(QString()); setLeadingIcon(rt().legacyOperationIcon(index));
    setLeadingIconVisible(true); setTrailingIconVisible(false);
}
void Z_PushButton::SetIcoButton() { applyLegacy("SetIcoButton"); }
void Z_PushButton::SetDarkButton(const QIcon& icon) {
    applyLegacy("SetDarkButton");
    setLeadingIcon(icon); setLeadingIconVisible(true);
}
void Z_PushButton::SetIcoRightButton(const QString& value,const QIcon& icon) {
    applyLegacy("SetIcoRightButton");
    setText(value); setTrailingIcon(icon);
    setTrailingIconVisible(true); setLeadingIconVisible(false);
}
void Z_PushButton::SetFont(const QFont& font) {
    if (!rt().legacyFontSize) throw std::logic_error("Legacy font mapping missing");
    setSize(rt().legacyFontSize(font));
}
}
