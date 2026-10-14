#include "MdiGallery.h"
#include "MdiAppearance.h"
#include "ComponentFactory.h"
#include "FigmaGalleryView.h"
#include <QMdiArea>
#include <QMdiSubWindow>
#include <QTabBar>
#include <QAbstractButton>
#include <QCloseEvent>
#include <QCheckBox>
#include <QComboBox>
#include <QFormLayout>
#include <QLineEdit>
#include <QPushButton>
#include <QSignalBlocker>
#include <QTimer>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLabel>
#include <QPainterPath>
#include <QVBoxLayout>
#include <functional>
#include <stdexcept>

namespace native_gallery {
namespace {
QRect bounds(const QJsonArray& n) {
    return QRect(qRound(n[3].toDouble()), qRound(n[4].toDouble()), qRound(n[5].toDouble()),
                 qRound(n[6].toDouble()));
}
QLabel* label(QWidget* parent, const QString& text, const QRect& r, int px, int weight,
              const QString& color, bool wrap = true) {
    auto* l = new QLabel(text, parent);
    l->setTextFormat(Qt::PlainText);
    l->setGeometry(r);
    l->setWordWrap(wrap);
    l->setAlignment(Qt::AlignTop | Qt::AlignLeft);
    QFont f("Inter");
    f.setPixelSize(px);
    f.setWeight(weight);
    l->setFont(f);
    l->setStyleSheet(QString("background:transparent;border:0;color:%1;font-family:'Inter';font-"
                             "size:%2px;font-weight:%3;")
                         .arg(color)
                         // Qt 5's QSS parser divides numeric weights by eight (not CSS mapping).
                         .arg(px)
                         .arg(weight * 8));
    return l;
}
class Document final : public QWidget {
  public:
    explicit Document(QWidget* p = nullptr) : QWidget(p) {
        setObjectName("mdiDocument");
        setStyleSheet("QWidget#mdiDocument{background:white;border:0;}");
    }

  protected:
    void closeEvent(QCloseEvent* e) override {
        if (property("vetoClose").toBool())
            e->ignore();
        else
            QWidget::closeEvent(e);
    }
};
void populate(QMdiSubWindow* sub, const QString& state, bool source, bool tabbed = false) {
    if (sub->widget()) {
        auto* old = sub->widget();
        sub->setWidget(nullptr);
        delete old;
    }
    auto* page = new Document;
    sub->setWidget(page);
    if (source && (state == "Minimized" || state == "Shaded")) {
        page->hide();
        return;
    }
    const QString color = state == "Disabled" ? "#8a8a8a" : "#191919";
    if (tabbed) {
        page->setStyleSheet(
            "QWidget#mdiDocument{background:white;border:1px solid #d0d0d0;border-radius:6px;}");
        auto* heading = label(page, "Trend document · one active page", QRect(18, 20, 214, 44), 14,
                              QFont::Medium, color);
        auto* body = label(page,
                           "TabbedView uses a QTabBar presentation, but the document remains a "
                           "managed QMdiSubWindow.",
                           QRect(18, 52, 520, 40), 12, QFont::Normal, color);
        if (!source) {
            auto* layout = new QVBoxLayout(page);
            layout->setContentsMargins(18, 20, 18, 18);
            layout->addWidget(heading);
            layout->addWidget(body);
            layout->addStretch();
        }
        return;
    }
    auto* heading =
        label(page, state == "Maximized" ? "Maximized document content" : "Document content slot",
              QRect(16, 20, state == "Maximized" ? 169 : 130, 18), 12, QFont::Normal, color);
    auto* card = new QWidget(page);
    card->setObjectName("mdiDataPreview");
    card->setGeometry(16, 54, 246, 56);
    card->setStyleSheet(
        "QWidget#mdiDataPreview{background:white;border:1px solid #d0d0d0;border-radius:8px;}");
    auto* detail =
        label(card, "Pressure trend · 1.20 Pa", QRect(12, 18, 123, 36), 12, QFont::Normal, color);
    if (!source) {
        auto* inner = new QVBoxLayout(card);
        inner->setContentsMargins(12, 12, 12, 12);
        inner->addWidget(detail);
        auto* outer = new QVBoxLayout(page);
        outer->setContentsMargins(16, 20, 16, 20);
        outer->setSpacing(16);
        outer->addWidget(heading);
        outer->addWidget(card);
        outer->addStretch();
        heading->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Preferred);
        detail->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Preferred);
    }
}
QMdiSubWindow* makeSub(const std::shared_ptr<commonWidget::ComponentFactory>& factory,
                       QWidget* parent, const QString& state, bool source, bool tabbed = false) {
    auto* sub =
        qobject_cast<QMdiSubWindow*>(factory->create("commonWidget::Z_MdiSubWindow", parent));
    if (!sub)
        throw std::runtime_error("MdiSubWindow unavailable");
    sub->setWindowTitle(source ? "Trend window" : "Trend");
    sub->setAttribute(Qt::WA_DeleteOnClose, false);
    sub->setProperty("figmaMdiSourceSnapshot", source);
    if (source) {
        commonWidget::applyFigmaMdiSubWindowAppearance(sub, state);
        populate(sub, state, true, tabbed);
    } else {
        commonWidget::populateFigmaMdiSubWindow(sub);
        commonWidget::applyFigmaMdiSubWindowAppearance(sub, state);
    }
    return sub;
}
class SnapshotLayout final : public QObject {
  public:
    explicit SnapshotLayout(QMdiArea* a) : QObject(a), area(a) {
        a->installEventFilter(this);
        a->viewport()->installEventFilter(this);
    }
    void refresh() {
        if (area->viewMode() == QMdiArea::TabbedView) {
            if (auto* bar = area->findChild<QTabBar*>(QString(), Qt::FindDirectChildrenOnly)) {
                bar->setGeometry(12, 48, 580, 36);
                for (int i = 0; i < bar->count(); ++i)
                    for (auto side : {QTabBar::LeftSide, QTabBar::RightSide})
                        if (auto* b = bar->tabButton(i, side))
                            b->setVisible(i == 0);
            }
            area->viewport()->setGeometry(12, 92, 580, 250);
            for (auto* sub : area->subWindowList())
                sub->setGeometry(0, 0, 580, 250);
        } else
            area->viewport()->setGeometry(0, 0, 604, 360);
    }
    bool eventFilter(QObject* watched, QEvent* e) override {
        if (!queued && (e->type() == QEvent::Show || e->type() == QEvent::Resize ||
                        e->type() == QEvent::LayoutRequest)) {
            queued = true;
            QTimer::singleShot(0, this, [this] {
                refresh();
                queued = false;
            });
        }
        // Only the frozen gallery suppresses the native viewport-resize callback,
        // which would immediately reset the authored TabbedView composition.
        return watched == area->viewport() && e->type() == QEvent::Resize;
    }

  private:
    QMdiArea* area;
    bool queued = false;
};
QMdiArea* sourceArea(const std::shared_ptr<commonWidget::ComponentFactory>& factory,
                     QWidget* parent, const QString& presentation) {
    auto* area = qobject_cast<QMdiArea*>(factory->create("commonWidget::Z_MdiArea", parent));
    if (!area)
        throw std::runtime_error("MdiArea unavailable");
    area->setProperty("figmaMdiSourceSnapshot", true);
    area->setFixedSize(604, 360);
    commonWidget::applyFigmaMdiAreaAppearance(area);
    area->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    area->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    area->setActivationOrder(QMdiArea::ActivationHistoryOrder);
    const QList<QRect> positions =
        presentation == "Free" ? QList<QRect>{{36, 70, 280, 170}, {292, 158, 280, 170}}
        : presentation == "Cascade"
            ? QList<QRect>{{18, 52, 280, 170}, {104, 92, 280, 170}, {190, 132, 280, 170}}
            : QList<QRect>{{12, 42, 280, 170},
                           {312, 42, 280, 170},
                           {12, 188, 280, 170},
                           {312, 188, 280, 170}};
    const bool tabbed = presentation == "Tabbed";
    const int count = tabbed ? 3 : positions.size();
    for (int i = 0; i < count; ++i) {
        auto* sub = makeSub(
            factory, nullptr,
            tabbed ? QString()
                   : ((presentation == "Cascade" ? i == 2 : i == 0) ? "Active" : "Inactive"),
            true, tabbed);
        area->addSubWindow(sub);
        if (tabbed)
            sub->setWindowTitle(QStringList{"Trend", "Recipe", "Alarm"}[i]);
        else
            sub->setGeometry(positions[i]);
        sub->show();
    }
    if (tabbed) {
        area->setTabsClosable(true);
        area->setViewMode(QMdiArea::TabbedView);
        area->setActiveSubWindow(area->subWindowList().first());
        commonWidget::applyFigmaMdiAreaAppearance(area);
    }
    auto* header = new QWidget(area);
    header->setGeometry(0, 0, 604, 36);
    header->setStyleSheet("background:white;border:0;");
    label(header, "Documents", QRect(12, 10, 117, 18), 12, QFont::DemiBold, "#191919");
    label(header, tabbed ? "TabbedView" : "ActivationHistoryOrder", QRect(390, 11, 109, 15), 10,
          QFont::Normal, "#191919");
    header->raise();
    new SnapshotLayout(area);
    return area;
}
} // namespace
QWidget* createMdiGallery(const std::shared_ptr<commonWidget::ComponentFactory>& factory,
                          QWidget* parent) {
    QFile file(":/figma/gallery-layouts.json");
    if (!file.open(QIODevice::ReadOnly))
        throw std::runtime_error("Gallery evidence missing");
    QJsonArray gallery;
    for (const auto& item : QJsonDocument::fromJson(file.readAll()).object()["pages"].toArray())
        if (item.toObject()["id"] == "511:11")
            gallery = item.toObject()["galleries"].toArray()[0].toArray();
    auto* canvas = new QWidget;
    canvas->setObjectName("figmaGalleryCanvas");
    canvas->setFixedSize(bounds(gallery).size());
    canvas->setProperty("galleryNodeId", "511:45");
    canvas->setProperty("variantSpecimenCount", 10);
    canvas->setProperty("componentSetCount", 2);
    canvas->setStyleSheet("QWidget#figmaGalleryCanvas{background:white;border:1px solid "
                          "#d0d0d0;border-radius:12px;}");
    std::function<void(const QJsonArray&, QWidget*)> build = [&](const QJsonArray& n,
                                                                 QWidget* host) {
        const auto id = n[0].toString(), name = n[1].toString(), type = n[2].toString();
        QWidget* node = nullptr;
        if (type == "COMPONENT") {
            if (name.startsWith("State="))
                node = makeSub(factory, host, name.mid(6), true);
            else
                node = sourceArea(factory, host, name.section('=', 1));
            node->setFixedSize(bounds(n).size());
            node->setProperty("figmaVariant", name);
        } else if (type == "TEXT") {
            const bool badge =
                id == "511:47" || id == "511:126" || id == "511:285" || id == "511:287";
            node = label(host, n[7].toString(), bounds(n),
                         badge             ? 11
                         : id == "511:282" ? 14
                                           : 12,
                         id == "511:283" ? QFont::Normal : QFont::DemiBold,
                         id == "511:47" || id == "511:126" ? "white"
                         : id == "511:283"                 ? "#191919"
                                                           : "#6235f7",
                         !badge && id != "511:282");
        } else {
            node = new QWidget(host);
            node->setObjectName("mdiGroup_" + QString(id).replace(':', '_'));
            if (type != "COMPONENT_SET")
                node->setStyleSheet(
                    QString("QWidget#%1{background:%2;border:%3;border-radius:%4px;}")
                        .arg(node->objectName(),
                             id == "511:284" || id == "511:286" ? "white" : "#f3f3f3",
                             id == "511:281" ? "1px solid #6235f7" : "0")
                        .arg(id == "511:281" ? 12 : 14));
            node->setGeometry(bounds(n));
            for (const auto& child : n[7].toArray())
                build(child.toArray(), node);
        }
        node->setProperty("figmaNodeId", id);
        node->setProperty("figmaDesignBounds", bounds(n));
        node->setGeometry(bounds(n));
        // Figma single-line TEXT does not clip at its own measured bounds; its
        // enclosing frame is the clipping boundary. Keep source bounds as metadata.
        if (auto* text = qobject_cast<QLabel*>(node); text && !text->wordWrap())
            text->resize(
                qMax(text->width(), text->fontMetrics().horizontalAdvance(text->text()) + 1),
                text->height());
    };
    for (const auto& child : gallery[7].toArray())
        build(child.toArray(), canvas);
    canvas->setAttribute(Qt::WA_TransparentForMouseEvents);
    return new FigmaGalleryView(canvas, parent);
}
void configureMdiEditor(QWidget* preview, QFormLayout* controls, QLineEdit* title,
                        QVBoxLayout* hostLayout, QWidget* settings, QComboBox* variant,
                        const std::shared_ptr<commonWidget::ComponentFactory>& factory) {
    controls->setRowWrapPolicy(QFormLayout::WrapAllRows);
    controls->setContentsMargins(12, 12, 12, 12);
    variant->setMinimumContentsLength(1);
    variant->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Fixed);
    auto* rootSub = qobject_cast<QMdiSubWindow*>(preview);
    auto* area = qobject_cast<QMdiArea*>(preview);
    if (rootSub) {
        hostLayout->removeWidget(rootSub);
        area = qobject_cast<QMdiArea*>(
            factory->create("commonWidget::Z_MdiArea", hostLayout->parentWidget()));
        area->setObjectName("mdiEditorWorkspace");
        hostLayout->insertWidget(2, area, 1);
    } else
        hostLayout->setStretchFactor(area, 1);
    area->setMinimumSize(300, 360);
    area->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    area->setActivationOrder(QMdiArea::ActivationHistoryOrder);
    commonWidget::applyFigmaMdiAreaAppearance(area);
    QList<QMdiSubWindow*> documents;
    if (!rootSub) {
        commonWidget::configureFigmaMdiArea(
            area, {{QStringLiteral("Presentation"), QStringLiteral("Free")}});
        documents = area->subWindowList();
    } else {
        for (int i = 0; i < 3; ++i) {
            auto* sub = i == 0 ? rootSub : makeSub(factory, nullptr, {}, false);
            if (sub == rootSub) {
                commonWidget::populateFigmaMdiSubWindow(sub);
                commonWidget::applyFigmaMdiSubWindowAppearance(sub);
            }
            sub->setWindowTitle(QStringList{"Trend", "Recipe", "Alarm"}[i]);
            sub->setObjectName(sub == rootSub ? "componentInteractivePreview"
                                              : "mdiEditorDocument" + QString::number(i));
            sub->setMinimumSize(220, 140);
            sub->setMaximumSize(QWIDGETSIZE_MAX, QWIDGETSIZE_MAX);
            sub->setAttribute(Qt::WA_DeleteOnClose, false);
            area->addSubWindow(sub);
            sub->setGeometry(14 + i * 28, 18 + i * 32, 280, 190);
            sub->show();
            documents.append(sub);
        }
    }
    area->setActiveSubWindow(documents.first());
    auto* target = new QComboBox(settings);
    target->setObjectName("mdiTargetControl");
    for (auto* document : documents)
        target->addItem(document->windowTitle());
    controls->addRow(QString::fromUtf8("操作的文档"), target);
    title->setEnabled(true);
    if (auto* l = qobject_cast<QLabel*>(controls->labelForField(title)))
        l->setText("windowTitle (target)");
    auto* mode = new QComboBox(settings);
    mode->setObjectName("mdiViewModeControl");
    mode->addItems({"SubWindowView", "TabbedView"});
    controls->addRow("viewMode", mode);
    const auto check = [&](const QString& text, const char* name, bool on) {
        auto* c = new QCheckBox(text, settings);
        c->setObjectName(name);
        c->setChecked(on);
        controls->addRow(c);
        return c;
    };
    auto* live = check(QString::fromUtf8("跟随真实窗口状态"), "mdiLiveAppearanceControl", !rootSub);
    auto* closeGlyph = check("Show Close glyph", "mdiCloseGlyphControl", true);
    auto* minGlyph = check("Show Minimize glyph", "mdiMinimizeGlyphControl", true);
    auto* maxGlyph = check("Show Maximize glyph", "mdiMaximizeGlyphControl", true);
    auto* closeHint = check(QString::fromUtf8("允许标题栏关闭"), "mdiCloseHintControl", true);
    closeHint->setToolTip("WindowCloseButtonHint");
    auto* minHint = check(QString::fromUtf8("允许标题栏最小化"), "mdiMinimizeHintControl", true);
    minHint->setToolTip("WindowMinimizeButtonHint");
    auto* maxHint = check(QString::fromUtf8("允许标题栏最大化"), "mdiMaximizeHintControl", true);
    maxHint->setToolTip("WindowMaximizeButtonHint");
    auto* targetEnabled =
        check(QString::fromUtf8("启用目标子窗口"), "mdiTargetEnabledControl", true);
    auto* veto = check(QString::fromUtf8("内容拒绝关闭（演示）"), "mdiVetoCloseControl", false);
    auto* tabsClose = check("tabsClosable", "mdiTabsClosableControl", true);
    auto* tabsMove = check("tabsMovable", "mdiTabsMovableControl", false);
    auto* rtl = check("Right to left", "mdiRtlControl", false);
    auto* order = new QComboBox(settings);
    order->setObjectName("mdiActivationOrderControl");
    order->addItems({"CreationOrder", "StackingOrder", "ActivationHistoryOrder"});
    order->setCurrentIndex(2);
    order->setMinimumContentsLength(1);
    order->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Fixed);
    controls->addRow("activationOrder", order);
    auto* readout = new QLabel(settings);
    readout->setObjectName("mdiNativeReadout");
    readout->setWordWrap(true);
    readout->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Preferred);
    controls->addRow(readout);
    const auto selected = [=] { return documents[target->currentIndex()]; };
    const auto sync = [=] {
        auto* s = selected();
        const QSignalBlocker a(title), b(targetEnabled), c(veto), d(closeHint), e(minHint),
            f(maxHint);
        title->setText(s->windowTitle());
        targetEnabled->setChecked(s->isEnabled());
        veto->setChecked(s->widget()->property("vetoClose").toBool());
        closeHint->setChecked(s->windowFlags().testFlag(Qt::WindowCloseButtonHint));
        minHint->setChecked(s->windowFlags().testFlag(Qt::WindowMinimizeButtonHint));
        maxHint->setChecked(s->windowFlags().testFlag(Qt::WindowMaximizeButtonHint));
        readout->setText(
            QString("viewMode=%1 / windows=%2\nactive=%3\ntarget: max=%4 / min=%5\nshaded=%6 / "
                    "visible=%7\nenabled=%8 / close veto=%9")
                .arg(area->viewMode() == QMdiArea::TabbedView ? "TabbedView" : "SubWindowView")
                .arg(area->subWindowList().size())
                .arg(area->activeSubWindow() ? area->activeSubWindow()->windowTitle() : "none")
                .arg(s->isMaximized())
                .arg(s->isMinimized())
                .arg(s->isShaded())
                .arg(s->isVisible())
                .arg(s->isEnabled())
                .arg(s->widget()->property("vetoClose").toBool()));
    };
    const auto appearance = [=] {
        auto* s = rootSub ? rootSub : selected();
        commonWidget::applyFigmaMdiSubWindowAppearance(
            s,
            live->isChecked() ? QString()
            : rootSub         ? variant->currentText().section('=', 1)
                              : QString(),
            closeGlyph->isChecked(), minGlyph->isChecked(), maxGlyph->isChecked());
        sync();
    };
    const auto button = [&](const QString& text, const char* name,
                            const std::function<void()>& action) {
        auto* b = new QPushButton(text, settings);
        b->setObjectName(name);
        b->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Fixed);
        controls->addRow(b);
        QObject::connect(b, &QPushButton::clicked, area, [=] {
            action();
            sync();
        });
    };
    button(QString::fromUtf8("激活目标"), "mdiActivateControl", [=] {
        selected()->show();
        area->setActiveSubWindow(selected());
    });
    button(QString::fromUtf8("最大化目标"), "mdiMaximizeControl",
           [=] { selected()->showMaximized(); });
    button(QString::fromUtf8("最小化目标"), "mdiMinimizeControl",
           [=] { selected()->showMinimized(); });
    button(QString::fromUtf8("卷起目标（Shaded）"), "mdiShadeControl",
           [=] { selected()->showShaded(); });
    button(QString::fromUtf8("恢复目标"), "mdiRestoreControl", [=] { selected()->showNormal(); });
    button(QString::fromUtf8("关闭目标（可拒绝）"), "mdiCloseControl",
           [=] { selected()->close(); });
    button(QString::fromUtf8("显示所有文档"), "mdiShowAllControl", [=] {
        for (auto* s : documents)
            s->show();
    });
    button("cascadeSubWindows()", "mdiCascadeControl", [=] {
        mode->setCurrentIndex(0);
        area->cascadeSubWindows();
    });
    button("tileSubWindows()", "mdiTileControl", [=] {
        mode->setCurrentIndex(0);
        area->tileSubWindows();
    });
    button("closeAllSubWindows()", "mdiCloseAllControl", [=] { area->closeAllSubWindows(); });
    if (!rootSub)
        button(QString::fromUtf8("应用所选布局（执行命令）"), "mdiApplyPresentationControl", [=] {
            const auto choice = variant->currentText().section('=', 1);
            if (choice == "Tabbed")
                mode->setCurrentIndex(1);
            else {
                mode->setCurrentIndex(0);
                for (auto* s : documents)
                    s->showNormal();
                if (choice == "Cascade")
                    area->cascadeSubWindows();
                else if (choice == "Tile")
                    area->tileSubWindows();
                else
                    for (int i = 0; i < documents.size(); ++i)
                        documents[i]->setGeometry(14 + i * 28, 18 + i * 32, 280, 190);
            }
        });
    auto* note =
        new QLabel(QString::fromUtf8(
                       "Figma State 仅固定外观，不执行窗口命令。Free／Cascade／Tile "
                       "不是 viewMode 枚举；选变体后需点击“应用所选布局”执行命令。图标显隐不更改 "
                       "WindowFlags。关闭由内容接受或拒绝；此演示关闭仅隐藏，文档不删"
                       "除。系统菜单、键盘移动／缩放仍由 Qt 管理。"),
                   settings);
    note->setWordWrap(true);
    controls->addRow(note);
    QObject::connect(target, qOverload<int>(&QComboBox::currentIndexChanged), area,
                     [=] { sync(); });
    QObject::connect(title, &QLineEdit::textChanged, area,
                     [=](const QString& text) { selected()->setWindowTitle(text); });
    QObject::connect(mode, qOverload<int>(&QComboBox::currentIndexChanged), area, [=](int n) {
        area->setViewMode(n ? QMdiArea::TabbedView : QMdiArea::SubWindowView);
        commonWidget::applyFigmaMdiAreaAppearance(area);
        sync();
    });
    for (auto* c : {live, closeGlyph, minGlyph, maxGlyph})
        QObject::connect(c, &QCheckBox::toggled, area, appearance);
    const auto flags = [=] {
        auto* s = selected();
        auto f = s->windowFlags();
        f.setFlag(Qt::WindowCloseButtonHint, closeHint->isChecked());
        f.setFlag(Qt::WindowMinimizeButtonHint, minHint->isChecked());
        f.setFlag(Qt::WindowMaximizeButtonHint, maxHint->isChecked());
        s->setWindowFlags(f);
        s->show();
        sync();
    };
    for (auto* c : {closeHint, minHint, maxHint})
        QObject::connect(c, &QCheckBox::toggled, area, flags);
    QObject::connect(targetEnabled, &QCheckBox::toggled, area, [=](bool on) {
        selected()->setEnabled(on);
        sync();
    });
    QObject::connect(veto, &QCheckBox::toggled, area, [=](bool on) {
        selected()->widget()->setProperty("vetoClose", on);
        sync();
    });
    QObject::connect(tabsClose, &QCheckBox::toggled, area, &QMdiArea::setTabsClosable);
    area->setTabsClosable(true);
    QObject::connect(tabsMove, &QCheckBox::toggled, area, &QMdiArea::setTabsMovable);
    QObject::connect(rtl, &QCheckBox::toggled, area, [=](bool on) {
        area->setLayoutDirection(on ? Qt::RightToLeft : Qt::LeftToRight);
    });
    QObject::connect(order, qOverload<int>(&QComboBox::currentIndexChanged), area, [=](int n) {
        area->setActivationOrder(static_cast<QMdiArea::WindowOrder>(n));
        sync();
    });
    QObject::connect(area, &QMdiArea::subWindowActivated, settings, [=] { sync(); });
    for (auto* s : documents)
        QObject::connect(s, &QMdiSubWindow::windowStateChanged, settings, [=] { sync(); });
    QObject::connect(variant, &QComboBox::currentTextChanged, area, [=] {
        if (rootSub)
            live->setChecked(false);
        QTimer::singleShot(0, area, appearance);
    });
    appearance();
}
} // namespace native_gallery
