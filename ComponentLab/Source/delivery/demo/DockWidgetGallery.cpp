#include "DockWidgetGallery.h"
#include "DockWidgetAppearance.h"
#include "ComponentFactory.h"
#include "FigmaGalleryView.h"
#include <QAbstractButton>
#include <QDockWidget>
#include <QAction>
#include <QCheckBox>
#include <QComboBox>
#include <QFormLayout>
#include <QLineEdit>
#include <QMainWindow>
#include <QPushButton>
#include <QSignalBlocker>
#include <QTimer>
#include <QFile>
#include <QFontDatabase>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLabel>
#include <QPainterPath>
#include <QRegion>
#include <QVBoxLayout>
#include <functional>
#include <stdexcept>

namespace native_gallery {
namespace {
QRect bounds(const QJsonArray& n) {
    return QRect(qRound(n[3].toDouble()), qRound(n[4].toDouble()), qRound(n[5].toDouble()),
                 qRound(n[6].toDouble()));
}
QLabel* label(QWidget* p, const QString& text, const QRect& r, int px, int weight,
              const QString& color, bool wrap = false) {
    auto* l = new QLabel(text, p);
    l->setTextFormat(Qt::PlainText);
    l->setGeometry(r);
    l->setWordWrap(wrap);
    l->setAlignment(Qt::AlignLeft | Qt::AlignTop);
    QFont f("HarmonyOS Sans");
    f.setPixelSize(px);
    f.setWeight(weight);
    l->setFont(f);
    l->setStyleSheet(
        QString("background:transparent;border:0;color:%1;font-family:'HarmonyOS Sans';font-size:%2px;")
            .arg(color)
            .arg(px));
    return l;
}
} // namespace
void configureDockWidgetEditor(QDockWidget* dock, QFormLayout* controls, QLineEdit* title,
                               QVBoxLayout* hostLayout, QWidget* settings, QComboBox* variant) {
    controls->setRowWrapPolicy(QFormLayout::WrapAllRows);
    controls->setContentsMargins(12, 12, 12, 12);
    variant->setMinimumContentsLength(1);
    variant->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Fixed);
    hostLayout->removeWidget(dock);
    auto* main = new QMainWindow(hostLayout->parentWidget());
    main->setObjectName("dockWorkspace");
    main->setWindowFlags(Qt::Widget);
    main->setDockOptions(QMainWindow::AllowNestedDocks | QMainWindow::AllowTabbedDocks);
    main->setMinimumSize(280, 320);
    main->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    hostLayout->insertWidget(2, main, 1);
    dock->setWindowTitle("Inspector");
    commonWidget::populateFigmaDockWidget(
        dock, QString::fromUtf8("可编辑的原生停靠面板内容"));
    dock->widget()->setMinimumSize(220, 120);
    main->addDockWidget(Qt::LeftDockWidgetArea, dock);
    title->setEnabled(true);
    title->setText(dock->windowTitle());
    if (auto* l = qobject_cast<QLabel*>(controls->labelForField(title)))
        l->setText("windowTitle");
    QObject::connect(title, &QLineEdit::textChanged, dock, &QDockWidget::setWindowTitle);
    const auto check = [&](const QString& text, const char* name, bool checked) {
        auto* c = new QCheckBox(text, settings);
        c->setObjectName(name);
        c->setChecked(checked);
        controls->addRow(c);
        return c;
    };
    auto* live =
        check(QString::fromUtf8("外观跟随实际停靠位置"), "dockLiveAppearanceControl", false);
    auto* glyphClose = check("Show Close glyph", "dockCloseGlyphControl", true);
    auto* glyphFloat = check("Show Undock glyph", "dockFloatGlyphControl", true);
    auto* closable = check("features: Closable", "dockClosableControl", true);
    auto* movable = check("features: Movable", "dockMovableControl", true);
    auto* floatable = check("features: Floatable", "dockFloatableControl", true);
    auto* vertical = check(QString::fromUtf8("竖向标题栏"), "dockVerticalControl", false);
    vertical->setToolTip("features: DockWidgetVerticalTitleBar");
    auto* floating =
        check(QString::fromUtf8("floating（真实窗口）"), "dockFloatingControl", false);
    auto* rtl = check("Right to left", "dockRtlControl", false);
    auto* destination = new QComboBox(settings);
    destination->setObjectName("dockAreaControl");
    for (const auto& a : QStringList{"Left", "Right", "Top", "Bottom"})
        destination->addItem(a);
    controls->addRow(QString::fromUtf8("实际停靠目标"), destination);
    const QList<Qt::DockWidgetArea> areas{Qt::LeftDockWidgetArea, Qt::RightDockWidgetArea,
                                          Qt::TopDockWidgetArea, Qt::BottomDockWidgetArea};
    QList<QCheckBox*> permitted;
    for (int i = 0; i < 4; ++i) {
        auto* c = check("allowedAreas: " + destination->itemText(i),
                        qPrintable("dockAllow" + destination->itemText(i)), true);
        permitted.append(c);
    }
    auto* readout = new QLabel(settings);
    readout->setObjectName("dockNativeReadout");
    readout->setWordWrap(true);
    readout->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Preferred);
    controls->addRow(readout);
    const auto button = [&](const QString& text, const char* name) {
        auto* b = new QPushButton(text, settings);
        b->setObjectName(name);
        b->setMinimumWidth(0);
        b->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Fixed);
        controls->addRow(b);
        return b;
    };
    auto* move = button(QString::fromUtf8("停靠到所选区域"), "dockMoveControl");
    auto* show = button(QString::fromUtf8("显示面板"), "dockShowControl");
    auto* toggle = button(QString::fromUtf8("原生显示／关闭动作"), "dockToggleControl");
    auto* tabify = button(QString::fromUtf8("与辅助面板页签化"), "dockTabifyControl");
    auto* save = button(QString::fromUtf8("暂存布局（仅内存）"), "dockSaveControl");
    auto* restore = button(QString::fromUtf8("恢复暂存布局"), "dockRestoreControl");
    restore->setEnabled(false);
    auto* note = new QLabel(
        QString::fromUtf8(
            "Area／Placement 仅为设计预览，不等于 allowedAreas 或 floating。图标显隐不更改 "
            "features，也不关闭原生命令；需禁止命令请改 features。实际浮动和竖向标题栏沿用 Qt "
            "平台外观。关闭只隐藏，不删除；布局暂存不写磁盘。停靠控件不是带遮罩的 Drawer。"),
        settings);
    note->setWordWrap(true);
    controls->addRow(note);
    const auto sync = [=] {
        const QSignalBlocker block(floating);
        floating->setChecked(dock->isFloating());
        const auto a = main->dockWidgetArea(dock);
        readout->setText(
            QString("floating=%1 / area=%2\nfeatures=%3 / allowedAreas=%4\nvisible=%5 / child=%6")
                .arg(dock->isFloating())
                .arg(dock->isFloating() ? QString::fromUtf8("不适用") : QString::number(int(a)))
                .arg(int(dock->features()))
                .arg(int(dock->allowedAreas()))
                .arg(dock->isVisible())
                .arg(dock->widget()->objectName()));
        toggle->setEnabled(dock->toggleViewAction()->isEnabled());
    };
    const auto appearance = [=] {
        QString a, p;
        if (!live->isChecked()) {
            for (const auto& part : variant->currentText().split(", ")) {
                if (part.startsWith("Area="))
                    a = part.mid(5);
                if (part.startsWith("Placement="))
                    p = part.mid(10);
            }
        }
        commonWidget::applyFigmaDockWidgetAppearance(dock, a, p, glyphClose->isChecked(),
                                                     glyphFloat->isChecked());
        sync();
    };
    const auto features = [=] {
        QDockWidget::DockWidgetFeatures f;
        if (closable->isChecked())
            f |= QDockWidget::DockWidgetClosable;
        if (movable->isChecked())
            f |= QDockWidget::DockWidgetMovable;
        if (floatable->isChecked())
            f |= QDockWidget::DockWidgetFloatable;
        if (vertical->isChecked())
            f |= QDockWidget::DockWidgetVerticalTitleBar;
        dock->setFeatures(f);
        sync();
    };
    for (auto* c : {closable, movable, floatable, vertical})
        QObject::connect(c, &QCheckBox::toggled, dock, features);
    for (auto* c : {live, glyphClose, glyphFloat})
        QObject::connect(c, &QCheckBox::toggled, dock, appearance);
    for (auto* c : permitted)
        QObject::connect(c, &QCheckBox::toggled, dock, [=] {
            Qt::DockWidgetAreas flags;
            for (int i = 0; i < 4; ++i)
                if (permitted[i]->isChecked())
                    flags |= areas[i];
            dock->setAllowedAreas(flags);
            sync();
        });
    QObject::connect(floating, &QCheckBox::toggled, dock, [=](bool on) {
        dock->setFloating(on);
        dock->show();
        sync();
    });
    QObject::connect(move, &QPushButton::clicked, dock, [=] {
        auto a = areas[destination->currentIndex()];
        if (!dock->isAreaAllowed(a)) {
            readout->setText(QString::fromUtf8("目标不在 allowedAreas 中；未移动。"));
            return;
        }
        dock->setFloating(false);
        main->addDockWidget(a, dock);
        dock->show();
        sync();
    });
    QObject::connect(show, &QPushButton::clicked, dock, [=] {
        dock->show();
        dock->raise();
        sync();
    });
    QObject::connect(toggle, &QPushButton::clicked, dock, [=] {
        dock->toggleViewAction()->trigger();
        sync();
    });
    QObject::connect(tabify, &QPushButton::clicked, dock, [=] {
        auto* other = main->findChild<QDockWidget*>("dockCompanion", Qt::FindDirectChildrenOnly);
        if (!other) {
            other = new QDockWidget("Companion", main);
            other->setObjectName("dockCompanion");
            commonWidget::populateFigmaDockWidget(other, QString::fromUtf8("Qt 页签化示例"),
                                                   QStringLiteral("Companion"));
            commonWidget::applyFigmaDockWidgetAppearance(other);
            main->addDockWidget(Qt::LeftDockWidgetArea, other);
        }
        dock->setFloating(false);
        other->show();
        dock->show();
        main->tabifyDockWidget(dock, other);
        dock->raise();
        sync();
    });
    QObject::connect(save, &QPushButton::clicked, dock, [=] {
        main->setProperty("dockSavedState", main->saveState(1));
        restore->setEnabled(true);
    });
    QObject::connect(restore, &QPushButton::clicked, dock, [=] {
        const bool ok = main->restoreState(main->property("dockSavedState").toByteArray(), 1);
        sync();
        readout->setText(readout->text() + QString("\nrestoreState=%1").arg(ok));
    });
    QObject::connect(rtl, &QCheckBox::toggled, main, [=](bool on) {
        main->setLayoutDirection(on ? Qt::RightToLeft : Qt::LeftToRight);
    });
    QObject::connect(dock, &QDockWidget::topLevelChanged, settings, [=] { sync(); });
    QObject::connect(dock, &QDockWidget::visibilityChanged, settings, [=] { sync(); });
    QObject::connect(dock, &QDockWidget::dockLocationChanged, settings, [=] { sync(); });
    QObject::connect(variant, &QComboBox::currentTextChanged, dock,
                     [=] { QTimer::singleShot(0, dock, appearance); });
    appearance();
}
QWidget* createDockWidgetGallery(const std::shared_ptr<commonWidget::ComponentFactory>& factory,
                                 QWidget* parent) {
    QFile file(":/figma/gallery-layouts.json");
    if (!file.open(QIODevice::ReadOnly))
        throw std::runtime_error("Gallery evidence missing");
    QJsonArray gallery;
    for (const auto& item : QJsonDocument::fromJson(file.readAll()).object()["pages"].toArray())
        if (item.toObject()["id"] == "507:2")
            gallery = item.toObject()["galleries"].toArray()[0].toArray();
    for (const auto* font : {"Regular", "Medium", "SemiBold"})
        QFontDatabase::addApplicationFont(QString(":/figma/fonts/Inter-%1.otf").arg(font));
    auto* canvas = new QWidget;
    canvas->setObjectName("figmaGalleryCanvas");
    canvas->setFixedSize(bounds(gallery).size());
    canvas->setProperty("galleryNodeId", "507:29");
    canvas->setProperty("variantSpecimenCount", 8);
    canvas->setProperty("componentSetCount", 1);
    canvas->setStyleSheet("QWidget#figmaGalleryCanvas{background:white;border-radius:16px;}");
    std::function<void(const QJsonArray&, QWidget*)> build = [&](const QJsonArray& n,
                                                                 QWidget* host) {
        const auto id = n[0].toString(), name = n[1].toString(), type = n[2].toString();
        if (type == "COMPONENT" || type == "INSTANCE") {
            auto* dock =
                qobject_cast<QDockWidget*>(factory->create("commonWidget::Z_DockWidget", host));
            if (!dock)
                throw std::runtime_error("DockWidget unavailable");
            const QString area = id == "507:445"   ? "Left"
                                 : id == "507:453" ? "Right"
                                                   : name.section(',', 0, 0).section('=', 1);
            const QString placement = id == "507:445"   ? "Docked"
                                      : id == "507:453" ? "Floating"
                                                        : name.section("Placement=", 1);
            dock->setWindowTitle(id == "507:445"   ? "Properties"
                                 : id == "507:453" ? "Floating tools"
                                                   : "Inspector");
            dock->setProperty("figmaDockSourceSnapshot", true);
            dock->setProperty("figmaDockSourceTitleTransparent", id == "507:453");
            dock->setProperty("figmaDockSourceBorderWidth", id == "507:453"           ? 1
                                                            : placement == "Floating" ? 2
                                                                                      : 1);
            commonWidget::populateFigmaDockWidget(dock, area + " dock · " + placement,
                                                   dock->windowTitle());
            commonWidget::applyFigmaDockWidgetAppearance(dock, area, placement);
            dock->setProperty("figmaNodeId", id);
            dock->setProperty("figmaVariant", name);
            dock->setProperty("figmaDesignBounds", bounds(n));
            dock->setGeometry(bounds(n));
            dock->setFixedSize(bounds(n).size());
            dock->ensurePolished();
            dock->layout()->setEnabled(false);
            dock->widget()->setGeometry(0, 37, 260, 123);
            for (auto* b :
                 dock->findChildren<QAbstractButton*>(QString(), Qt::FindDirectChildrenOnly))
                if (b->inherits("QDockWidgetTitleButton"))
                    b->setGeometry(b->objectName() == "qt_dockwidget_closebutton"
                                       ? QRect(233, 10, 16, 16)
                                       : QRect(209, 10, 16, 16));
            dock->setAttribute(Qt::WA_TransparentForMouseEvents);
            return;
        }
        if (type == "TEXT") {
            const int size = id == "507:462"   ? 11
                             : id == "507:463" ? 16
                             : id == "507:465" ? 12
                                               : 13;
            auto* l = label(host, n[7].toString(), bounds(n), size,
                            id == "507:462" || id == "507:463"   ? QFont::DemiBold
                            : id == "507:330" || id == "507:465" ? QFont::Medium
                                                                 : QFont::Normal,
                            id == "507:462"   ? "#6235f7"
                            : id == "507:465" ? "#8a8a8a"
                                              : "#191919",
                            id == "507:464");
            l->setProperty("figmaNodeId", id);
            return;
        }
        auto* group = new QWidget(host);
        group->setGeometry(bounds(n));
        group->setProperty("figmaNodeId", id);
        group->setObjectName("dockGroup_" + QString(id).replace(':', '_'));
        if (type == "COMPONENT_SET") {
            group->setProperty("figmaComponentSetId", id);
            QPainterPath clip;
            clip.addRoundedRect(QRectF(group->rect()), 5, 5);
            group->setMask(QRegion(clip.toFillPolygon().toPolygon()));
        } else
            group->setStyleSheet(QString("QWidget#%1{background:%2;border:%3;border-radius:%4px;}")
                                     .arg(group->objectName())
                                     .arg(id == "507:461" ? "white" : "#f3f3f3")
                                     .arg(id == "507:461" ? "1px solid #d0d0d0" : "0")
                                     .arg(id == "507:444" ? 12 : 10));
        for (const auto& child : n[7].toArray())
            build(child.toArray(), group);
    };
    for (const auto& child : gallery[7].toArray())
        build(child.toArray(), canvas);
    return new FigmaGalleryView(canvas, parent);
}
} // namespace native_gallery
