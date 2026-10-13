#include "ButtonAppearanceAdapter.h"
#include "FocusIndicator.h"
#include <QDynamicPropertyChangeEvent>
#include <QLabel>
#include <QPainter>
#include <QTimer>

ButtonAppearanceAdapter::ButtonAppearanceAdapter(commonWidget::Z_PushButton* b,
                                                 std::shared_ptr<const surface::Catalog> c)
    : QObject(b), button(b), catalog(std::move(c)) {
    setObjectName("APPEARANCE_ADAPTER");
    inset = new surface::InsetLayer(b);
    inset->lower();
    solid = new surface::solid_inner_shadow::Layer(b);
    solid->hide();
    outer = new surface::OuterShadowEffect(b);
    b->setGraphicsEffect(outer);
    b->installEventFilter(this);
    refresh();
}
void ButtonAppearanceAdapter::setAppearance(const QString& id) {
    if(id!="figma-light") catalog->label(id);
    preset = id;
    button->setProperty("figmaAppearance",id=="figma-light");
    refresh();
}
void ButtonAppearanceAdapter::refresh() {
    const bool captured=preset=="figma-light" && !button->property("ghost").toBool() &&
        (!button->property("danger").toBool() || button->property("buttonType")=="PRIMARY");
    button->setProperty("figmaAppearanceSupported",captured);
    if(captured) {
        inset->hide();solid->hide();outer->setEnabled(false);
        for(const char* name:{"innerShadowRole","innerShadowBaseColor","innerShadowColor","innerShadowBlackMix"})
            button->setProperty(name,QVariant());
        const QString qss="commonWidget--Z_PushButton {background:transparent;border:0;padding:0;}";
        if(button->styleSheet()!=qss)button->setStyleSheet(qss);
        button->update();return;
    }
    outer->setEnabled(true);
    QString state = button->property("visualState").toString().toLower();
    auto r = catalog->button(
        preset=="figma-light"?"solid":preset, button->property("buttonType").toString(), button->property("danger").toBool(),
        button->property("ghost").toBool(), state, button->property("buttonSize").toString());
    QString fg = QString("rgba(%1,%2,%3,%4)")
                     .arg(r.foreground.red())
                     .arg(r.foreground.green())
                     .arg(r.foreground.blue())
                     .arg(r.foreground.alpha());
    QString qss = "commonWidget--Z_PushButton {" + r.qss +
                  QString("border-width:%1px;").arg(r.borderWidth) +
                  "}\ncommonWidget--Z_PushButton QLabel {color:" + fg + ";}";
    auto filled = r.iconFilledForeground;
    qss += QString("\ncommonWidget--Z_PushButton QFrame[iconContainerStyle=\"FILLED\"] QLabel "
                   "{color:rgba(%1,%2,%3,%4);}")
               .arg(filled.red())
               .arg(filled.green())
               .arg(filled.blue())
               .arg(filled.alpha());
    if (commonWidget::FocusIndicator::instance().visibleFor(button))
        qss += "\ncommonWidget--Z_PushButton:focus {border-color:" +
               button->focusIndicatorColor().name(QColor::HexArgb) +
               ";}";
    if (button->styleSheet() != qss)
        button->setStyleSheet(qss);
    if (r.solidInnerShadow.isEmpty()) {
        for (const char* key :
             {"innerShadowRole", "innerShadowBaseColor", "innerShadowColor", "innerShadowBlackMix"})
            button->setProperty(key, QVariant());
        solid->hide();
        inset->setGeometry(button->rect());
        inset->setRecipe(r);
        inset->lower();
        inset->show();
    } else {
        inset->hide();
        auto visual = r.solidInnerShadow["visual"].toObject();
        button->setProperty("innerShadowRole", r.solidInnerShadow["role"].toString());
        button->setProperty("innerShadowBaseColor", visual["shadowBaseColor"].toString());
        button->setProperty("innerShadowColor", visual["shadowColor"].toString());
        button->setProperty("innerShadowBlackMix", visual["blackMix"].toDouble());
        if (commonWidget::FocusIndicator::instance().visibleFor(button) && state != "disabled")
            visual["borderColor"] = button->focusIndicatorColor().name(QColor::HexArgb);
        solid->setGeometry(button->rect());
        solid->setTokens(r.solidInnerShadow["geometry"].toObject(), visual, state);
        solid->lower();
        solid->show();
    }
    outer->setRecipe(r);
    for (auto* label : button->findChildren<QLabel*>("GLYPH")) {
        auto source = label->pixmap(Qt::ReturnByValue);
        if (source.isNull())
            continue;
        auto tint = label->parentWidget()->property("iconContainerStyle") == "FILLED"
                        ? r.iconFilledForeground
                        : r.foreground;
        QPainter p(&source);
        p.setCompositionMode(QPainter::CompositionMode_SourceIn);
        p.fillRect(source.rect(), tint);
        p.end();
        label->setPixmap(source);
    }
}
bool ButtonAppearanceAdapter::eventFilter(QObject*, QEvent* e) {
    bool update = e->type() == QEvent::Enter || e->type() == QEvent::Leave ||
                  e->type() == QEvent::MouseButtonPress || e->type() == QEvent::MouseMove ||
                  e->type() == QEvent::MouseButtonRelease || e->type() == QEvent::KeyPress ||
                  e->type() == QEvent::KeyRelease || e->type() == QEvent::EnabledChange ||
                  e->type() == QEvent::FocusIn || e->type() == QEvent::FocusOut ||
                  e->type() == QEvent::Resize || e->type() == QEvent::Show;
    if (e->type() == QEvent::DynamicPropertyChange) {
        const auto name = static_cast<QDynamicPropertyChangeEvent*>(e)->propertyName();
        update = QList<QByteArray>{"buttonType", "danger",           "ghost",
                                   "loading",    "disabledByPolicy", "buttonSize",
                                   "focusIndicatorVisible", "figmaPreviewState"}
                     .contains(name);
    }
    if (update && !queued) {
        queued = true;
        QTimer::singleShot(0, this, [this] {
            queued = false;
            refresh();
        });
    }
    return false;
}
