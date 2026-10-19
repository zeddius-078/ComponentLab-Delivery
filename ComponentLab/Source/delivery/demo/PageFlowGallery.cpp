#include "PageFlowGallery.h"
#include "ComponentFactory.h"
#include "PageFlowAppearance.h"
#include "FieldChrome.h"
#include "FigmaGalleryView.h"
#include "native/Z_WizardPage.h"
#include "Z_PushButton.h"
#include <QWizard>
#include <QStackedWidget>
#include <QAbstractButton>
#include <QCheckBox>
#include <QComboBox>
#include <QFormLayout>
#include <QLineEdit>
#include <QPushButton>
#include <QSpinBox>
#include <QSignalBlocker>
#include <QTimer>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLabel>
#include <QPainter>
#include <QPainterPath>
#include <QTextLayout>
#include <QSvgRenderer>
#include <QVBoxLayout>
#include <functional>
#include <stdexcept>

namespace native_gallery {
namespace {
const QStringList phases={"Start","Middle","Commit","Finish"};
const QStringList styles={"Classic","Modern","Mac","Aero"};
QRect bounds(const QJsonArray& n) {return {qRound(n[3].toDouble()),qRound(n[4].toDouble()),qRound(n[5].toDouble()),qRound(n[6].toDouble())};}
QRectF rectF(const QJsonArray& a) {return {a[0].toDouble(),a[1].toDouble(),a[2].toDouble(),a[3].toDouble()};}
QJsonObject evidence() {QFile f(":/figma/pageflow-visuals.json");if(!f.open(QIODevice::ReadOnly))throw std::runtime_error("Page Flow source evidence missing");return QJsonDocument::fromJson(f.readAll()).object();}
QColor paintColor(const QJsonArray& paints) {
    if(paints.isEmpty())return Qt::transparent;
    const auto a=paints[0].toArray();const auto c=a[0].toObject();
    return QColor::fromRgbF(c["r"].toDouble(),c["g"].toDouble(),c["b"].toDouble(),a[1].toDouble());
}
// Passive source specimen only. The real editor below uses Qt layouts and page hooks.
class SourceInk final:public QWidget {
public:
    SourceInk(QWidget* p,QString source,QString kind={},int phase=0):QWidget(p),source(source),kind(kind),phase(phase) {
        setObjectName("pageFlowSourceInk");setAttribute(Qt::WA_TransparentForMouseEvents);
        for(const auto& v:evidence()["templates"].toArray())rows.append(v.toArray());
    }
protected:
    void paintEvent(QPaintEvent*) override {QPainter p(this);p.setRenderHint(QPainter::Antialiasing);draw(p,source,true);}
private:
    void draw(QPainter& p,const QString& id,bool root=false) {
        QJsonArray n;for(const auto& row:rows)if(row[0]==id){n=row;break;}if(n.isEmpty()||!n[5].toBool())return;
        const auto r=rectF(n[4].toArray());p.save();if(!root)p.translate(r.topLeft());
        const QRectF local(QPointF(0,0),r.size());
        const auto type=n[3].toString(),name=n[2].toString();
        if(type=="INSTANCE") {
            if(name=="Completion icon") {const QString icon=source=="516:73"?"Check":"Warning";QSvgRenderer svg(":/figma/pageflow/"+icon+".svg");svg.render(&p,local);}
            p.restore();return;
        }
        if(type=="TEXT") {
            const auto text=n[12].toArray();QString value=text[0].toString();
            if(name=="Status text" && !kind.isEmpty())value.replace("Standard",kind);
            if(name=="Header title")value=phase==2?"Confirm changes":phase==3?"Complete setup":"Configure device";
            if(name=="Header subtitle")value=kind+"Style · "+phases[phase];
            if(name=="Progress")value=QString("Step %1 of 4").arg(phase+1);
            QFont font("HarmonyOS Sans");font.setPixelSize(text[2].toInt());const auto style=text[1].toObject()["style"].toString();font.setWeight(style=="Semi Bold"?QFont::DemiBold:style=="Medium"?QFont::Medium:QFont::Normal);
            p.setPen(paintColor(n[7].toArray()));p.setFont(font);
            QTextLayout layout(value,font);QTextOption option;option.setWrapMode(text[5]=="WIDTH_AND_HEIGHT"?QTextOption::NoWrap:QTextOption::WrapAtWordBoundaryOrAnywhere);layout.setTextOption(option);
            const double lineHeight=text[3].toObject()["value"].toDouble(QFontMetricsF(font).height());
            layout.beginLayout();qreal y=0;while(true){auto line=layout.createLine();if(!line.isValid())break;line.setLineWidth(r.width());line.setPosition(QPointF(0,y+(lineHeight-line.height())/2));y+=lineHeight;}layout.endLayout();layout.draw(&p,QPointF(0,0));
        } else {
            const qreal stroke=n[8].toArray().isEmpty()?0:n[9].toDouble();const qreal radius=n[10].toDouble();
            QPainterPath path;path.addRoundedRect(local.adjusted(stroke/2,stroke/2,-stroke/2,-stroke/2),qMax(0.,radius-stroke/2),qMax(0.,radius-stroke/2));p.setBrush(paintColor(n[7].toArray()));p.setPen(stroke>0?QPen(paintColor(n[8].toArray()),stroke):QPen(Qt::NoPen));p.drawPath(path);
            if(n[11].toBool()){QPainterPath clip;clip.addRoundedRect(local,radius,radius);p.setClipPath(clip,Qt::IntersectClip);}
            for(const auto& child:rows)if(child[1]==id)draw(p,child[0].toString());
        }
        p.restore();
    }
    QString source,kind;int phase;QList<QJsonArray> rows;
};
QLabel* label(QWidget* parent,const QString& text,const QRect& geometry,int px,int weight,const QString& color,bool wrap=false) {
    auto* l=new QLabel(text,parent);l->setTextFormat(Qt::PlainText);l->setGeometry(geometry);l->setWordWrap(wrap);l->setAlignment(Qt::AlignTop|Qt::AlignLeft);
    l->setStyleSheet(QString("background:transparent;border:0;color:%1;font-family:'HarmonyOS Sans';font-size:%2px;font-weight:%3;").arg(color).arg(px).arg(weight*8));
    if(!wrap)l->resize(qMax(l->width(),l->fontMetrics().horizontalAdvance(text)+1),l->height());return l;
}
class DemoPage final:public commonWidget::Z_WizardPage {
public:
    explicit DemoPage(QWidget* parent=nullptr):Z_WizardPage(parent) {
        setTitle("Configure device");setSubTitle("Complete the required fields to continue.");
        auto* layout=new QVBoxLayout(this);layout->setContentsMargins(16,14,16,16);
        heading=new QLabel(title(),this);heading->setObjectName("flowPageTitle");heading->setWordWrap(true);
        subtitle=new QLabel(subTitle(),this);subtitle->setObjectName("flowPageSubtitle");subtitle->setWordWrap(true);
        layout->addWidget(heading);layout->addWidget(subtitle);
        auto* fieldLabel=new QLabel("Device name *",this);layout->addWidget(fieldLabel);
        input=new QLineEdit(this);input->setObjectName("flowRequiredField");input->setPlaceholderText("Required field is empty");input->setAccessibleName("Device name (required)");fieldLabel->setBuddy(input);layout->addWidget(input);
        auto* row=new QHBoxLayout;statusIcon=new QLabel(this);statusIcon->setFixedSize(16,16);statusIcon->setObjectName("flowCompletionIcon");row->addWidget(statusIcon);
        status=new QLabel(this);status->setObjectName("flowCompletionStatus");status->setWordWrap(true);row->addWidget(status,1);layout->addLayout(row);layout->addStretch();
        registerField(QString("deviceName%1*").arg(quintptr(this)),input);
        connect(input,&QLineEdit::textChanged,this,[this]{if(!wizard())emit completeChanged();refresh();});
        connect(this,&QWizardPage::completeChanged,this,[this]{refresh();});
        commonWidget::applyFigmaWizardPageAppearance(this);refresh();
    }
    bool isComplete() const override {
        // A standalone specimen has no QWizard field registry; this demo's explicit
        // virtual completeness policy is NOT a new writable SDK Completion property.
        return wizard()?QWizardPage::isComplete():(!input->text().isEmpty()&&input->hasAcceptableInput());
    }
    bool validatePage() override {setProperty("validationCalls",property("validationCalls").toInt()+1);return !property("vetoValidation").toBool()&&QWizardPage::validatePage();}
    void initializePage() override {setProperty("initializeCalls",property("initializeCalls").toInt()+1);QWizardPage::initializePage();refresh();}
    void cleanupPage() override {setProperty("cleanupCalls",property("cleanupCalls").toInt()+1);QWizardPage::cleanupPage();refresh();}
    int nextId() const override {if(property("demoNextId").isValid())return property("demoNextId").toInt();return QWizardPage::nextId();}
    void refresh() {
        heading->setText(title());subtitle->setText(subTitle());subtitle->setVisible(property("showSpecimenSubtitle").isNull()||property("showSpecimenSubtitle").toBool());
        const bool complete=isComplete();const QString kind=isCommitPage()?"Commit":isFinalPage()?"Final":"Standard";
        status->setText((complete?"Complete · ":"Incomplete · ")+kind+" page");status->setAccessibleName(status->text());
        // SDK appearance owns palette and Profile resolution. The demo only supplies
        // real completion state and content; it must not shadow generated Tokens.
        status->setStyleSheet({});
        commonWidget::applyFigmaWizardPageAppearance(this);
        const QString asset=QString(":/figma/pageflow/%1.svg").arg(complete?"Check":"Warning");
        statusIcon->setPixmap(field::iconPixmap(QIcon(asset),16,devicePixelRatioF(),
            property("figmaFlowStatusIconColor").value<QColor>()));statusIcon->setProperty("figmaAsset",asset);
    }
    QLineEdit* input;QLabel *heading,*subtitle,*status,*statusIcon;
};
void populateWizard(QWizard* wizard) {
    wizard->setWindowTitle("Page Flow · native QWizard");wizard->resize(620,460);wizard->setWizardStyle(QWizard::ClassicStyle);
    for(int i=0;i<4;++i){auto* page=new DemoPage;page->setProperty("designSystemProfile",wizard->property("designSystemProfile"));page->setProperty("demoNextId",i==3?-1:i+1);page->setProperty("flowPageId",i);page->setTitle(i==2?"Confirm changes":i==3?"Complete setup":"Configure device");page->setCommitPage(i==2);page->setFinalPage(i==3);wizard->setPage(i,page);page->refresh();}
    wizard->setStartId(0);wizard->restart();commonWidget::applyFigmaWizardAppearance(wizard);
}
void sourceButtons(QWidget* parent,int phase,const std::shared_ptr<commonWidget::ComponentFactory>& factory) {
    const QStringList texts={"Back",phase==2?"Commit":phase==3?"Finish":"Next","Cancel"};
    for(int i=0;i<3;++i){auto* b=qobject_cast<commonWidget::Z_PushButton*>(factory->create("commonWidget::Z_PushButton",parent));if(!b)throw std::runtime_error("Canonical Button unavailable");b->setText(texts[i]);b->setType(i==1?commonWidget::ButtonType::Primary:commonWidget::ButtonType::Default);b->setSize(commonWidget::ButtonSize::M);b->setProperty("figmaPreviewState",i==0&&phase==0?"disabled":"default");if(i==0&&phase==0)b->setEnabled(false);b->setAutoDefault(false);b->setFocusPolicy(Qt::NoFocus);b->setAttribute(Qt::WA_TransparentForMouseEvents);const int width=i==0?57:i==2?70:phase==2?76:phase==3?65:56;b->setFixedSize(width,32);b->move(54+78*i,196);b->setProperty("figmaDesignBounds",b->geometry());}
}
}
QWidget* createPageFlowPreview(const QString& sdkClass,const std::shared_ptr<commonWidget::ComponentFactory>& factory,QWidget* parent) {
    if(sdkClass=="commonWidget::Z_WizardPage")return new DemoPage(parent);
    auto* result=factory->create(sdkClass,parent);
    if(auto* wizard=qobject_cast<QWizard*>(result)){wizard->setWindowFlags(Qt::Widget);populateWizard(wizard);}
    if(auto* stack=qobject_cast<QStackedWidget*>(result))commonWidget::populateFigmaStackedWidget(stack,3);
    return result;
}
QWidget* createPageFlowGallery(const std::shared_ptr<commonWidget::ComponentFactory>& factory,QWidget* parent) {
    QFile file(":/figma/gallery-layouts.json");if(!file.open(QIODevice::ReadOnly))throw std::runtime_error("Gallery source missing");QJsonArray gallery;
    for(const auto& item:QJsonDocument::fromJson(file.readAll()).object()["pages"].toArray())if(item.toObject()["id"]=="516:11")gallery=item.toObject()["galleries"].toArray()[0].toArray();
    auto* canvas=new QWidget;canvas->setObjectName("figmaGalleryCanvas");canvas->setFixedSize(bounds(gallery).size());canvas->setProperty("galleryNodeId","516:45");canvas->setProperty("variantSpecimenCount",25);canvas->setProperty("componentSetCount",3);canvas->setStyleSheet("QWidget#figmaGalleryCanvas{background:white;border:1px solid #d0d0d0;border-radius:12px;}");
    std::function<void(const QJsonArray&,QWidget*)> build=[&](const QJsonArray& n,QWidget* host){const auto id=n[0].toString(),name=n[1].toString(),type=n[2].toString();QWidget* node=nullptr;
        if(type=="COMPONENT") {
            QString source,kind;int phase=0;
            if(name.startsWith("State=")){node=factory->create("commonWidget::Z_StackedWidget",host);source=id;auto* stack=qobject_cast<QStackedWidget*>(node);commonWidget::applyFigmaStackedWidgetAppearance(stack);if(name!="State=Empty")stack->addWidget(new QWidget);if(name=="State=Disabled")stack->setEnabled(false);}
            else if(name.startsWith("Kind=")){node=factory->create("commonWidget::Z_WizardPage",host);source=name.endsWith("=Complete")?"516:73":"516:65";kind=name.section('=',1,1).section(',',0,0);auto* page=qobject_cast<QWizardPage*>(node);page->setTitle("Configure device");page->setSubTitle("Complete the required fields to continue.");page->setCommitPage(kind=="Commit");page->setFinalPage(kind=="Final");commonWidget::applyFigmaWizardPageAppearance(page);}
            else {node=factory->create("commonWidget::Z_Wizard",host);node->setWindowFlags(Qt::Widget);kind=name.section('=',1,1).section(',',0,0);phase=phases.indexOf(name.section('=',2));source=kind=="Mac"?"516:389":"516:116";auto* wizard=qobject_cast<QWizard*>(node);commonWidget::applyFigmaWizardAppearance(wizard);if(wizard->layout()){wizard->layout()->setSizeConstraint(QLayout::SetNoConstraint);wizard->layout()->setEnabled(false);}for(auto* child:wizard->findChildren<QWidget*>(QString(),Qt::FindDirectChildrenOnly))child->hide();}
            if(!node)throw std::runtime_error("Page Flow native root unavailable");node->setFixedSize(bounds(n).size());node->setProperty("figmaVariant",name);node->setProperty("figmaSourceSnapshot",true);
            auto* ink=new SourceInk(node,source,kind,phase);ink->setGeometry(0,0,node->width(),node->height());ink->raise();if(name.startsWith("Style="))sourceButtons(ink,phase,factory);
        } else if(type=="TEXT") {const bool title=id=="516:671",body=id=="516:672";node=label(host,n[7].toString(),bounds(n),title?14:11,body?QFont::Normal:QFont::DemiBold,id=="516:47"||id=="516:64"||id=="516:115"?"white":body?"#191919":"#6235f7",body);}
        else {node=new QWidget(host);node->setObjectName("flowGroup_"+QString(id).replace(':','_'));if(type!="COMPONENT_SET")node->setStyleSheet(QString("QWidget#%1{background:%2;border:%3;border-radius:%4px;}").arg(node->objectName(),id=="516:46"||id=="516:63"||id=="516:114"?"#6235f7":"#f3f3f3",id=="516:670"?"1px solid #6235f7":"0").arg(id=="516:670"?12:14));node->setGeometry(bounds(n));for(const auto& child:n[7].toArray())build(child.toArray(),node);}
        node->setProperty("figmaNodeId",id);node->setProperty("figmaDesignBounds",bounds(n));node->move(bounds(n).topLeft());if(type!="TEXT")node->resize(bounds(n).size());
    };
    for(const auto& n:gallery[7].toArray())build(n.toArray(),canvas);canvas->setAttribute(Qt::WA_TransparentForMouseEvents);return new FigmaGalleryView(canvas,parent);
}
void configurePageFlowEditor(QWidget* preview,QFormLayout* controls,QLineEdit* title,QVBoxLayout* hostLayout,QWidget* settings,QComboBox* variant,const std::shared_ptr<commonWidget::ComponentFactory>&) {
    controls->setRowWrapPolicy(QFormLayout::WrapAllRows);controls->setContentsMargins(12,12,12,12);variant->setMinimumContentsLength(1);variant->setSizePolicy(QSizePolicy::Ignored,QSizePolicy::Fixed);
    auto* note=new QLabel(QString::fromUtf8("Figma Variant 只选择对照样本；不会清空页面、伪造 Completion 或设置只读 currentId。下方操作使用 Qt 原生索引、页面标志与导航命令。"),settings);note->setWordWrap(true);note->setSizePolicy(QSizePolicy::Ignored,QSizePolicy::Preferred);controls->addRow(note);
    auto* readout=new QLabel(settings);readout->setObjectName("flowNativeReadout");readout->setWordWrap(true);readout->setSizePolicy(QSizePolicy::Ignored,QSizePolicy::Preferred);controls->addRow(readout);
    auto command=[&](const QString& text,const char* name,const std::function<void()>& action){auto* b=new QPushButton(text,settings);b->setObjectName(name);b->setSizePolicy(QSizePolicy::Ignored,QSizePolicy::Fixed);b->setToolTip(text);controls->addRow(b);QObject::connect(b,&QPushButton::clicked,settings,action);return b;};
    if(auto* stack=qobject_cast<QStackedWidget*>(preview)) {
        title->setEnabled(true);title->setText("Current page");if(auto* l=qobject_cast<QLabel*>(controls->labelForField(title)))l->setText(QString::fromUtf8("示例标题（非路由）"));
        stack->setMinimumSize(300,220);hostLayout->setStretchFactor(stack,1);
        auto* index=new QSpinBox(settings);index->setObjectName("flowCurrentIndexControl");index->setRange(-1,99);controls->addRow("currentIndex",index);
        auto* removedHost=new QWidget(stack->parentWidget());removedHost->setObjectName("flowRemovedPages");removedHost->hide();
        // Removed pages stay owned by the stack. This list is only a demo reinsertion handle.
        auto removed=std::make_shared<QList<QWidget*>>();
        auto refresh=[=]{QSignalBlocker b(index);index->setValue(stack->currentIndex());readout->setText(QString("count=%1\ncurrentIndex=%2\nremoved retained=%3").arg(stack->count()).arg(stack->currentIndex()).arg(removed->size()));};
        auto add=[=]{auto* page=new QWidget;page->setObjectName(QString("flowStackPage%1").arg(stack->count()));auto* layout=new QVBoxLayout(page);auto* heading=new QLabel(title->text(),page);heading->setWordWrap(true);layout->addWidget(heading);auto* edit=new QLineEdit("Recipe parameters",page);edit->setAccessibleName("Page content");layout->addWidget(edit);layout->addStretch();stack->addWidget(page);refresh();};
        command(QString::fromUtf8("添加页面"),"flowAddPageControl",add);
        command(QString::fromUtf8("removeWidget（保留所有权）"),"flowRemovePageControl",[=]{if(auto* page=stack->currentWidget()){stack->removeWidget(page);removed->append(page);}refresh();});
        command(QString::fromUtf8("重新插入移除的页面"),"flowReinsertPageControl",[=]{if(!removed->isEmpty())stack->addWidget(removed->takeLast());refresh();});
        QObject::connect(index,qOverload<int>(&QSpinBox::valueChanged),stack,[=](int n){stack->setCurrentIndex(n);refresh();});QObject::connect(stack,&QStackedWidget::currentChanged,settings,[=]{refresh();});QObject::connect(stack,&QStackedWidget::widgetRemoved,settings,[=]{refresh();});
        if(stack->count()==0){add();add();add();}refresh();return;
    }
    auto* wizard=qobject_cast<QWizard*>(preview);auto* standalone=dynamic_cast<DemoPage*>(preview);
    if(standalone){standalone->setMinimumSize(300,260);hostLayout->setStretchFactor(standalone,1);}
    auto target=[=]()->DemoPage*{return standalone?standalone:dynamic_cast<DemoPage*>(wizard->currentPage());};
    title->setEnabled(true);title->setText("Configure device");if(auto* l=qobject_cast<QLabel*>(controls->labelForField(title)))l->setText("title (current page)");
    auto* subtitle=new QLineEdit("Complete the required fields to continue.",settings);subtitle->setObjectName("flowSubtitleControl");controls->addRow("subTitle",subtitle);
    auto* showSubtitle=new QCheckBox("Show specimen subtitle",settings);showSubtitle->setChecked(true);showSubtitle->setObjectName("flowSubtitleVisibleControl");controls->addRow(showSubtitle);
    auto* kind=new QComboBox(settings);kind->setObjectName("flowKindControl");kind->addItems({"Standard","Commit","Final"});controls->addRow(QString::fromUtf8("页面标志（方法）"),kind);
    auto* field=new QLineEdit(settings);field->setObjectName("flowFieldControl");controls->addRow(QString::fromUtf8("必填字段（真实内容）"),field);
    auto* veto=new QCheckBox("validatePage veto (demo)",settings);veto->setObjectName("flowValidationVetoControl");controls->addRow(veto);
    auto refresh=[=]{auto* page=target();if(!page)return;QSignalBlocker a(title),b(subtitle),c(kind),d(field),e(veto);title->setText(page->title());subtitle->setText(page->subTitle());kind->setCurrentIndex(page->isCommitPage()?1:page->isFinalPage()?2:0);field->setText(page->input->text());veto->setChecked(page->property("vetoValidation").toBool());page->refresh();
        QString text=QString("isComplete=%1\nisCommitPage=%2 · isFinalPage=%3\ninitialize=%4 · cleanup=%5 · validate=%6").arg(page->isComplete()).arg(page->isCommitPage()).arg(page->isFinalPage()).arg(page->property("initializeCalls").toInt()).arg(page->property("cleanupCalls").toInt()).arg(page->property("validationCalls").toInt());
        if(wizard){QStringList visited;for(int n:wizard->visitedIds())visited<<QString::number(n);text+=QString("\ncurrentId=%1 · nextId=%2\nvisited=[%3]\nwizardStyle=%4 (platform fallback possible)").arg(wizard->currentId()).arg(wizard->nextId()).arg(visited.join(",")).arg(wizard->wizardStyle());}else text+=QString::fromUtf8("\n独立页：isComplete 使用演示子类校验；放入 Wizard 后使用 Qt 必填字段注册表。");readout->setText(text);
    };
    QObject::connect(title,&QLineEdit::textChanged,settings,[=](const QString& t){if(auto* p=target()){p->setTitle(t);p->refresh();}});QObject::connect(subtitle,&QLineEdit::textChanged,settings,[=](const QString& t){if(auto* p=target()){p->setSubTitle(t);p->refresh();}});
    QObject::connect(showSubtitle,&QCheckBox::toggled,settings,[=](bool on){if(auto* p=target()){p->setProperty("showSpecimenSubtitle",on);p->refresh();}});
    QObject::connect(kind,qOverload<int>(&QComboBox::currentIndexChanged),settings,[=](int n){if(auto* p=target()){p->setCommitPage(n==1);p->setFinalPage(n==2);refresh();}});
    QObject::connect(field,&QLineEdit::textChanged,settings,[=](const QString& t){if(auto* p=target())p->input->setText(t);refresh();});QObject::connect(veto,&QCheckBox::toggled,settings,[=](bool on){if(auto* p=target())p->setProperty("vetoValidation",on);refresh();});
    if(wizard) {
        auto* style=new QComboBox(settings);style->setObjectName("flowWizardStyleControl");style->addItems(styles);controls->addRow("wizardStyle",style);QObject::connect(style,qOverload<int>(&QComboBox::currentIndexChanged),wizard,[=](int n){wizard->setWizardStyle(QWizard::WizardStyle(n));commonWidget::applyFigmaWizardAppearance(wizard);refresh();});
        auto* help=new QCheckBox("HaveHelpButton (Qt option)",settings);help->setObjectName("flowHelpControl");controls->addRow(help);QObject::connect(help,&QCheckBox::toggled,wizard,[=](bool on){wizard->setOption(QWizard::HaveHelpButton,on);});
        auto* branch=new QCheckBox(QString::fromUtf8("Start → Commit（跳过 Middle）"),settings);branch->setObjectName("flowBranchControl");controls->addRow(branch);QObject::connect(branch,&QCheckBox::toggled,wizard,[=](bool on){wizard->page(0)->setProperty("demoNextId",on?2:1);refresh();});
        command("Back (native)","flowBackControl",[=]{wizard->button(QWizard::BackButton)->click();refresh();});
        command("Next / Commit / Finish (native)","flowForwardControl",[=]{auto* p=target();if(!p)return;const auto role=p->isFinalPage()?QWizard::FinishButton:p->isCommitPage()?QWizard::CommitButton:QWizard::NextButton;wizard->button(role)->click();refresh();});
        command("Restart (native)","flowRestartControl",[=]{wizard->restart();refresh();});
        command("Cancel (native)","flowCancelControl",[=]{wizard->button(QWizard::CancelButton)->click();refresh();});
        QObject::connect(wizard,&QWizard::currentIdChanged,settings,[=]{refresh();});QObject::connect(wizard,&QDialog::finished,settings,[=](int result){settings->setProperty("flowLastResult",result);refresh();});
        for(int id:wizard->pageIds())if(auto* p=dynamic_cast<DemoPage*>(wizard->page(id)))QObject::connect(p->input,&QLineEdit::textChanged,settings,[=]{refresh();});
    } else QObject::connect(standalone->input,&QLineEdit::textChanged,settings,[=]{refresh();});
    for(auto* w:settings->findChildren<QWidget*>())
        if(qobject_cast<QCheckBox*>(w)||qobject_cast<QComboBox*>(w))
            w->setSizePolicy(QSizePolicy::Ignored,QSizePolicy::Fixed);
    refresh();
}
}
