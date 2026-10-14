#include "PageFlowAppearance.h"
#include "FocusIndicator.h"
#include "Z_PushButton.h"
#include "native/Z_WizardPage.h"
#include <QStackedWidget>
#include <QWizard>
#include <QWizardPage>
#include <QAbstractButton>
#include <QEvent>
#include <QFile>
#include <QFontDatabase>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QLabel>
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
QString color(const QJsonValue& v) {
    const auto a=v.toArray();
    return QString("rgba(%1,%2,%3,%4)").arg(a[0].toInt()).arg(a[1].toInt()).arg(a[2].toInt()).arg(qRound(255*a[3].toDouble(1)));
}
const QJsonObject& recipes() {
    static const auto value=[] {
        QFile f(":/figma/button-visuals.json");
        if(!f.open(QIODevice::ReadOnly))throw std::runtime_error("Button recipes unavailable");
        return QJsonDocument::fromJson(f.readAll()).object()["recipes"].toObject();
    }();
    return value;
}
class WizardBinding final:public QObject {
public:
    explicit WizardBinding(QWizard* w):QObject(w),wizard(w) {
        setObjectName("figmaWizardBinding");w->installEventFilter(this);
        connect(w,&QWizard::currentIdChanged,this,[this]{refresh();});
    }
    void refresh() {
        // Do not replace custom buttons: Qt keeps each role's exact object and connection.
        for(int n=QWizard::BackButton;n<=QWizard::CustomButton3;++n) {
            auto* b=wizard->button(QWizard::WizardButton(n));
            if(!b || qobject_cast<Z_PushButton*>(b))continue;
            FocusIndicator::instance().watch(b);
            b->setProperty("figmaWizardButton",true);
            const auto family=(n==QWizard::NextButton || n==QWizard::CommitButton || n==QWizard::FinishButton)?"PRIMARY":"DEFAULT";
            b->setProperty("figmaWizardButtonRecipe",family);
            QString css;
            for(const auto& pair:QList<QPair<QString,QString>>{{"","default"},{":hover","hover"},{":pressed","active"},{":disabled","disabled"}}) {
                const auto r=recipes()[family].toObject()[pair.second].toObject();
                css+=QString("QAbstractButton[figmaWizardButton=true]%1{background:%2;color:%3;border:%4px solid %5;border-radius:%6px;padding:0 12px;min-height:32px;font-family:'Inter';font-size:14px;font-weight:400;}")
                    .arg(pair.first,color(r["fill"]),color(r["foreground"])).arg(r["strokeWidth"].toDouble()).arg(color(r["stroke"])).arg(r["radius"].toDouble());
            }
            css+="QAbstractButton[figmaWizardButton=true]:focus[focusIndicatorVisible=true]{border:1px solid #6235f7;}";
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
    stack->setStyleSheet("QStackedWidget[designSystemStyleOwner=\"figma-pageflow-captured\"]{background:white;border:1px solid #d0d0d0;border-radius:8px;color:#191919;}QStackedWidget[designSystemStyleOwner=\"figma-pageflow-captured\"]:disabled{background:#8a8a8a;color:#8a8a8a;}");
}
void applyFigmaWizardPageAppearance(QWizardPage* page) {
    if(!page)throw std::invalid_argument("Wizard page required");fonts();
    page->setProperty("designSystemStyleOwner","figma-pageflow-captured");
    const auto refresh=[page]{
        const auto preview=page->property("figmaFlowCompletionPreview").toString();
        const bool complete=preview=="Complete" || (preview.isEmpty()&&page->isComplete());
        const QString css=QString("QWizardPage[designSystemStyleOwner=\"figma-pageflow-captured\"]{background:white;color:#191919;border:1px solid %1;border-radius:8px;}").arg(complete?"#6235f7":"#d0d0d0");
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
    const QString css="QWizard[designSystemStyleOwner=\"figma-pageflow-captured\"]{background:white;color:#191919;border:1px solid #d0d0d0;border-radius:10px;}";
    if(wizard->styleSheet()!=css)wizard->setStyleSheet(css);
    auto* binding=static_cast<WizardBinding*>(wizard->findChild<QObject*>("figmaWizardBinding",Qt::FindDirectChildrenOnly));
    if(!binding)binding=new WizardBinding(wizard);
    binding->refresh();
}
}
