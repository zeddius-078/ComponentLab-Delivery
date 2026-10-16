#include "PageFlowAppearance.h"
#include "FocusIndicator.h"
#include "Surface.h"
#include "TokenResources.h"
#include "Z_PushButton.h"
#include "ComponentTheme.h"
#include "native/Z_WizardPage.h"
#include <QStackedWidget>
#include <QWizard>
#include <QWizardPage>
#include <QAbstractButton>
#include <QColor>
#include <QEvent>
#include <QFile>
#include <QFontDatabase>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QLabel>
#include <QRegularExpression>
#include <QTimer>
#include <QVBoxLayout>
#include <stdexcept>

namespace commonWidget {
namespace {
void fonts() {
    static const bool loaded=[] {
        for(const auto* f:{"Regular","Medium","SemiBold"})
            QFontDatabase::addApplicationFont(QString(":/figma/fonts/Inter-%1.otf").arg(f));
        return true;
    }();
    Q_UNUSED(loaded)
}
QString profileFor(const QWidget* widget) {
    const auto explicitProfile=widget->property("designSystemProfile").toString();
    if(!explicitProfile.isEmpty())return explicitProfile;
    const auto installed=component_theme::activeProfile();
    return installed.isEmpty()?QStringLiteral("light-comfortable"):installed;
}
QString qssColor(const QColor& value) {
    return QString("rgba(%1,%2,%3,%4)").arg(value.red()).arg(value.green()).arg(value.blue()).arg(value.alpha());
}
struct FlowTokens {
    QJsonObject colors;
    explicit FlowTokens(const QString& profile) {
        static const QJsonObject profiles=[] {
            QJsonParseError error;
            const auto document=QJsonDocument::fromJson(token_resources::read(":/tokens/tokens_qt.json"),&error);
            if(error.error!=QJsonParseError::NoError || !document.isObject())
                throw std::runtime_error("Invalid Page Flow Token document");
            return document.object()["profiles"].toObject();
        }();
        colors=profiles[profile].toObject()["color"].toObject();
        if(colors.isEmpty())throw std::invalid_argument("Unknown Page Flow profile");
    }
    QColor color(const char* key) const {
        const auto value=colors[QString::fromLatin1(key)].toString();
        if(value.isEmpty())throw std::runtime_error("Missing Page Flow color Token");
        const auto rgba=QRegularExpression(QStringLiteral("^rgba\\((\\d+),\\s*(\\d+),\\s*(\\d+),\\s*([0-9.]+)\\)$")).match(value);
        if(rgba.hasMatch()) {
            QColor result(rgba.captured(1).toInt(),rgba.captured(2).toInt(),rgba.captured(3).toInt());
            result.setAlphaF(rgba.captured(4).toDouble());return result;
        }
        return surface::color(value);
    }
    QString qss(const char* key) const {return qssColor(color(key));}
    QString subtleBrand() const {auto value=color("base.stoken.color.interactive.brand.default");value.setAlpha(20);return qssColor(value);}
};
void inheritProfile(QWidget* child,const QWidget* parent) {
    const auto profile=parent->property("designSystemProfile");
    if(profile.isValid())child->setProperty("designSystemProfile",profile);
}
class WizardBinding final:public QObject {
public:
    explicit WizardBinding(QWizard* w):QObject(w),wizard(w) {
        setObjectName("figmaWizardBinding");w->installEventFilter(this);
        connect(w,&QWizard::currentIdChanged,this,[this]{refresh();});
    }
    void refresh() {
        const FlowTokens tokens(profileFor(wizard));
        const auto surface=tokens.qss("semantic.color.background.surface");
        const auto hover=tokens.qss("semantic.color.background.interactive-hover");
        const auto pressed=tokens.qss("semantic.color.background.interactive-active");
        const auto text=tokens.qss("semantic.color.text.primary");
        const auto disabledSurface=tokens.qss("semantic.color.background.disabled");
        const auto disabledText=tokens.qss("base.stoken.button.base.disabled.label.color");
        const auto control=tokens.qss("semantic.color.border.control");
        const auto subtle=tokens.qss("semantic.color.border.subtle");
        const auto brand=tokens.qss("base.stoken.color.interactive.brand.default");
        const auto brandHover=tokens.qss("base.stoken.color.interactive.brand.hover");
        const auto brandPressed=tokens.qss("base.stoken.color.interactive.brand.pressed");
        const auto onBrand=tokens.qss("semantic.color.text.onBrand");
        const auto focus=tokens.qss("semantic.color.border.focus");
        // Do not replace custom buttons: Qt keeps each role's exact object and connection.
        for(int n=QWizard::BackButton;n<=QWizard::CustomButton3;++n) {
            auto* b=wizard->button(QWizard::WizardButton(n));
            if(!b || qobject_cast<Z_PushButton*>(b))continue;
            FocusIndicator::instance().watch(b);
            b->setProperty("figmaWizardButton",true);
            const bool primary=n==QWizard::NextButton || n==QWizard::CommitButton || n==QWizard::FinishButton;
            const auto family=primary?"PRIMARY":"DEFAULT";
            b->setProperty("figmaWizardButtonRecipe",family);
            const auto buttonRule=[](const QString& pseudo,const QString& fill,const QString& foreground,const QString& border){
                return QString("QAbstractButton[figmaWizardButton=true]%1{background:%2;color:%3;border:1px solid %4;border-radius:4px;padding:0 12px;min-height:32px;font-family:'Inter';font-size:14px;font-weight:400;}").arg(pseudo,fill,foreground,border);
            };
            QString css=primary
                ?buttonRule("",brand,onBrand,brand)+buttonRule(":hover",brandHover,onBrand,brandHover)+buttonRule(":pressed",brandPressed,onBrand,brandPressed)
                :buttonRule("",surface,text,control)+buttonRule(":hover",hover,text,control)+buttonRule(":pressed",pressed,text,control);
            css+=buttonRule(":disabled",disabledSurface,disabledText,subtle);
            css+=QString("QAbstractButton[figmaWizardButton=true]:focus[focusIndicatorVisible=true]{border:1px solid %1;}").arg(focus);
            if(b->styleSheet()!=css)b->setStyleSheet(css);
        }
    }
    bool eventFilter(QObject*,QEvent* e) override {
        if(!queued && (e->type()==QEvent::Show || e->type()==QEvent::LayoutRequest || e->type()==QEvent::ChildAdded || e->type()==QEvent::StyleChange)) {
            queued=true;QTimer::singleShot(0,this,[this]{refresh();queued=false;});
        }
        return false;
    }
private:QWizard* wizard;bool queued=false;
};
}
void populateFigmaStackedWidget(QStackedWidget* stack, int pageCount) {
    if(!stack || pageCount<0 || pageCount>32)
        throw std::invalid_argument("Stack and pageCount 0..32 required");
    while(stack->count()) {
        auto* page=stack->widget(0);stack->removeWidget(page);delete page;
    }
    for(int i=0;i<pageCount;++i) {
        auto* page=new QWidget;
        page->setObjectName(QString("flowStackPage%1").arg(i));
        auto* layout=new QVBoxLayout(page);layout->setContentsMargins(16,16,16,16);
        auto* heading=new QLabel(QString("Page %1").arg(i+1),page);
        heading->setObjectName("flowStackPageTitle");heading->setWordWrap(true);
        auto* body=new QLabel("One owned page is visible; route history belongs to the host.",page);
        body->setObjectName("flowStackPageBody");body->setWordWrap(true);
        layout->addWidget(heading);layout->addWidget(body);layout->addStretch();
        stack->addWidget(page);
    }
    if(pageCount)stack->setCurrentIndex(0);
    applyFigmaStackedWidgetAppearance(stack);
}
void configureFigmaStackedWidget(QStackedWidget* stack,
                                 const QMap<QString, QString>& properties) {
    if(!stack)throw std::invalid_argument("Stack required");
    const auto state=properties.value("State","Content");
    if(!QStringList{"Empty","Content","Disabled"}.contains(state))
        throw std::invalid_argument("Invalid StackedWidget State");
    populateFigmaStackedWidget(stack,state=="Empty"?0:1);
    stack->setEnabled(state!="Disabled");
    stack->setProperty("figmaFlowStatePreview",state);
}
void configureFigmaWizardPage(QWizardPage* page,
                              const QMap<QString, QString>& properties) {
    if(!page)throw std::invalid_argument("Wizard page required");
    const auto kind=properties.value("Kind","Standard");
    const auto completion=properties.value("Completion","Incomplete");
    if(!QStringList{"Standard","Commit","Final"}.contains(kind) ||
       !QStringList{"Incomplete","Complete"}.contains(completion))
        throw std::invalid_argument("Invalid WizardPage Kind/Completion");
    page->setTitle(properties.value("Title","Configure device"));
    page->setSubTitle(properties.value("Subtitle","Complete the required fields to continue."));
    page->setCommitPage(kind=="Commit");page->setFinalPage(kind=="Final");
    page->setProperty("figmaFlowKindPreview",kind);
    page->setProperty("figmaFlowCompletionPreview",completion);
    if(!page->layout()) {
        auto* layout=new QVBoxLayout(page);layout->setContentsMargins(16,14,16,16);
        auto* heading=new QLabel(page->title(),page);heading->setObjectName("flowPageTitle");heading->setWordWrap(true);
        auto* subtitle=new QLabel(page->subTitle(),page);subtitle->setObjectName("flowPageSubtitle");subtitle->setWordWrap(true);
        auto* status=new QLabel(page);status->setObjectName("flowCompletionStatus");status->setWordWrap(true);
        layout->addWidget(heading);layout->addWidget(subtitle);layout->addStretch();layout->addWidget(status);
    }
    if(auto* status=page->findChild<QLabel*>("flowCompletionStatus"))
        status->setText(QString("%1 · %2 page").arg(completion,kind));
    applyFigmaWizardPageAppearance(page);
}
void configureFigmaWizard(QWizard* wizard,const QMap<QString,QString>& properties) {
    if(!wizard)throw std::invalid_argument("Wizard required");
    const auto style=properties.value("Style","Classic");
    const auto phase=properties.value("Phase","Start");
    const QMap<QString,QWizard::WizardStyle> styles{{"Classic",QWizard::ClassicStyle},
        {"Modern",QWizard::ModernStyle},{"Mac",QWizard::MacStyle},{"Aero",QWizard::AeroStyle}};
    if(!styles.contains(style) || !QStringList{"Start","Middle","Commit","Finish"}.contains(phase))
        throw std::invalid_argument("Invalid Wizard Style/Phase");
    for(int id:wizard->pageIds()) {auto* page=wizard->page(id);wizard->removePage(id);delete page;}
    const QStringList kinds{"Standard","Standard","Commit","Final"};
    for(int i=0;i<4;++i) {
        auto* page=new Z_WizardPage;
        inheritProfile(page,wizard);
        configureFigmaWizardPage(page,{{"Kind",kinds[i]},{"Completion","Incomplete"},
            {"Title",i==2?"Confirm changes":i==3?"Complete setup":"Configure device"}});
        wizard->setPage(i,page);
    }
    wizard->setWizardStyle(styles[style]);wizard->setStartId(0);wizard->restart();
    wizard->setProperty("figmaFlowStylePreview",style);
    wizard->setProperty("figmaFlowPhasePreview",phase);
    applyFigmaWizardAppearance(wizard);
}
void applyFigmaStackedWidgetAppearance(QStackedWidget* stack) {
    if(!stack)throw std::invalid_argument("Stack required");fonts();
    stack->setProperty("designSystemStyleOwner","figma-pageflow-captured");
    const FlowTokens tokens(profileFor(stack));
    const auto root=QStringLiteral("QStackedWidget[designSystemStyleOwner=\"figma-pageflow-captured\"]");
    stack->setStyleSheet(QString("%1{background:%2;border:1px solid %3;border-radius:8px;color:%4;}"
                                 "%1 QLabel#flowStackPageTitle,%1 QLabel#flowStackPageBody{background:transparent;border:0;color:%4;}"
                                 "%1:disabled{background:%5;border-color:%3;color:%6;}"
                                 "%1:disabled QLabel#flowStackPageTitle,%1:disabled QLabel#flowStackPageBody{color:%6;}")
        .arg(root,tokens.qss("semantic.color.background.surface"),tokens.qss("semantic.color.border.subtle"),
             tokens.qss("semantic.color.text.primary"),tokens.qss("semantic.color.text.disabled"),
             tokens.qss("semantic.color.text.disabled")));
}
void applyFigmaWizardPageAppearance(QWizardPage* page) {
    if(!page)throw std::invalid_argument("Wizard page required");fonts();
    page->setProperty("designSystemStyleOwner","figma-pageflow-captured");
    const auto refresh=[page]{
        const auto preview=page->property("figmaFlowCompletionPreview").toString();
        const bool complete=preview=="Complete" || (preview.isEmpty()&&page->isComplete());
        const FlowTokens tokens(profileFor(page));
        page->setProperty("figmaFlowStatusIconColor",
            complete?tokens.color("base.stoken.color.interactive.brand.default"):
                     tokens.color("semantic.color.text.primary"));
        const auto root=QStringLiteral("QWizardPage[designSystemStyleOwner=\"figma-pageflow-captured\"]");
        const QString css=QString("%1{background:%2;color:%3;border:1px solid %4;border-radius:8px;}"
                                  "%1 QLabel#flowPageTitle{background:transparent;border:0;color:%3;}"
                                  "%1 QLabel#flowPageSubtitle{background:transparent;border:0;color:%5;}"
                                  "%1 QLabel#flowCompletionStatus{background:%6;border:0;color:%7;padding:8px;font-family:'Inter';font-size:12px;font-weight:504;}")
            .arg(root,tokens.qss("semantic.color.background.surface"),tokens.qss("semantic.color.text.primary"),
                 complete?tokens.qss("semantic.color.border.focus"):tokens.qss("semantic.color.border.subtle"),
                 tokens.qss("semantic.color.text.secondary"),complete?tokens.subtleBrand():tokens.qss("semantic.color.background.surface"),
                 complete?tokens.qss("base.stoken.color.interactive.brand.default"):tokens.qss("semantic.color.text.primary"));
        if(page->styleSheet()!=css)page->setStyleSheet(css);
    };
    if(!page->property("figmaWizardPageBound").toBool()) {
        QObject::connect(page,&QWizardPage::completeChanged,page,refresh);
        page->setProperty("figmaWizardPageBound",true);
    }
    refresh();
}
void applyFigmaWizardAppearance(QWizard* wizard) {
    if(!wizard)throw std::invalid_argument("Wizard required");fonts();
    wizard->setProperty("designSystemStyleOwner","figma-pageflow-captured");
    const FlowTokens tokens(profileFor(wizard));
    const QString css=QString("QWizard[designSystemStyleOwner=\"figma-pageflow-captured\"]{background:%1;color:%2;border:1px solid %3;border-radius:10px;}")
        .arg(tokens.qss("semantic.color.background.surface"),tokens.qss("semantic.color.text.primary"),tokens.qss("semantic.color.border.subtle"));
    if(wizard->styleSheet()!=css)wizard->setStyleSheet(css);
    auto* binding=static_cast<WizardBinding*>(wizard->findChild<QObject*>("figmaWizardBinding",Qt::FindDirectChildrenOnly));
    if(!binding)binding=new WizardBinding(wizard);
    binding->refresh();
}
}
