#include "CompositeGallery.h"
#include "ComponentFactory.h"
#include "FigmaGalleryView.h"
#include "Z_DesignComponents.h"
#include <QPainter>
#include <QPainterPath>
#include <QPaintEvent>
#include <QSvgRenderer>
#include <functional>
#include <stdexcept>

namespace native_gallery {
namespace {

const QColor kText(0x19, 0x19, 0x19);
const QColor kAccent(0x62, 0x35, 0xf7);
const QColor kMutedBg(0xf3, 0xf3, 0xf3);
const QColor kBorderSubtle(0xd0, 0xd0, 0xd0);
const QColor kDisabledText(0x8a, 0x8a, 0x8a);
const QColor kKeyBg(0xf7, 0xf9, 0xfc);
const QColor kKeyText(0x50, 0x58, 0x68);

QFont inter(int px, QFont::Weight weight) {
    QFont font(QStringLiteral("HarmonyOS Sans"));
    font.setPixelSize(px);
    font.setWeight(weight);
    return font;
}

QPainterPath roundedPath(const QRectF& rect, qreal radius) {
    QPainterPath path;
    path.addRoundedRect(rect, radius, radius);
    return path;
}

void fillRounded(QPainter& p, const QRectF& rect, qreal radius, const QColor& fill,
                 const QColor& stroke = QColor(), qreal strokeWidth = 0) {
    const QPainterPath path =
        roundedPath(rect.adjusted(strokeWidth / 2, strokeWidth / 2, -strokeWidth / 2,
                                  -strokeWidth / 2),
                    qMax(0.0, radius - strokeWidth / 2));
    p.fillPath(path, fill);
    if (strokeWidth > 0 && stroke.isValid())
        p.strokePath(path, QPen(stroke, strokeWidth));
}

void drawText(QPainter& p, const QRectF& rect, const QString& text, int px,
              QFont::Weight weight, const QColor& color, int align) {
    p.setFont(inter(px, weight));
    p.setPen(color);
    p.drawText(rect, align | Qt::TextSingleLine, text);
}

void drawIcon(QPainter& p, const QRectF& rect, const QString& asset) {
    QSvgRenderer svg(QStringLiteral(":/figma/composite/") + asset);
    svg.render(&p, rect);
}

class CompositeCanvas final : public QWidget {
  public:
    using QWidget::QWidget;
    std::function<void(QPainter&)> paintContent;

  protected:
    void paintEvent(QPaintEvent*) override {
        QPainter p(this);
        p.setRenderHint(QPainter::Antialiasing);
        p.setRenderHint(QPainter::TextAntialiasing);
        if (paintContent)
            paintContent(p);
    }
};

void drawKey(QPainter& p, const QRectF& key, const QStringList& labels,
             const QVector<QRectF>& labelRects) {
    fillRounded(p, key, 10, kKeyBg);
    for (int i = 0; i < labels.size() && i < labelRects.size(); ++i)
        drawText(p, labelRects[i], labels[i], 12, QFont::DemiBold, kKeyText,
                 Qt::AlignLeft | Qt::AlignVCenter);
}

// --- Specimen painters; each draws relative to the specimen's own origin. ---

void paintNavItem(QPainter& p, const QRect& bounds, const QString& label,
                  const QString& state, bool collapsed, bool compact) {
    const int h = compact ? 40 : 46;
    const bool hover = state == "Hover";
    const bool selected = state == "Selected";
    const bool disabled = state == "Disabled";
    p.save();
    p.translate(bounds.topLeft());
    if (disabled)
        p.setOpacity(0.35);
    if (hover || selected)
        fillRounded(p, QRectF(0, 0, bounds.width(), h), 4, kMutedBg);
    const int pad = collapsed ? 14 : 12;
    const int iconY = (h - 20) / 2;
    drawIcon(p, QRectF(pad, iconY, 16, 16), selected ? "home-accent.svg" : "home.svg");
    if (!collapsed)
        drawText(p, QRectF(pad + 30, (h - 22) / 2.0, bounds.width() - pad - 30, 22), label, 14,
                 QFont::Normal, selected ? kAccent : kText, Qt::AlignLeft | Qt::AlignVCenter);
    p.restore();
}

void paintSubItem(QPainter& p, const QRect& bounds, const QString& label,
                  const QString& state) {
    p.save();
    p.translate(bounds.topLeft());
    QColor bg(Qt::white);
    QColor border = kBorderSubtle;
    QColor fg = kText;
    if (state == "Hover")
        bg = kMutedBg;
    else if (state == "Pressed")
        bg = kMutedBg, fg = kAccent;
    else if (state == "Focus")
        border = kAccent, fg = kAccent;
    else if (state == "Selected")
        bg = kMutedBg, fg = kAccent;
    else if (state == "Disabled")
        fg = kDisabledText;
    const QRectF local(0, 0, bounds.width(), bounds.height());
    fillRounded(p, local, 0, bg, border, 1);
    drawText(p, local.adjusted(4, 0, -4, 0), label, 14, QFont::DemiBold, fg,
             Qt::AlignCenter);
    p.restore();
}

void paintMainItem(QPainter& p, const QRect& bounds, const QString& label,
                   const QString& state, bool startPlacement, const QString& iconAsset) {
    p.save();
    p.translate(bounds.topLeft());
    QColor bg(Qt::white);
    QColor border = kBorderSubtle;
    QColor fg = kText;
    QString asset = iconAsset.isEmpty() ? QStringLiteral("home.svg") : iconAsset;
    if (state == "Hover")
        bg = kMutedBg;
    else if (state == "Pressed")
        bg = kMutedBg, fg = kAccent;
    else if (state == "Focus")
        border = kAccent, fg = kAccent;
    else if (state == "Selected")
        bg = kMutedBg, fg = kAccent;
    else if (state == "Disabled")
        fg = kDisabledText;
    if (fg == kAccent && asset == QStringLiteral("home.svg"))
        asset = QStringLiteral("home-accent.svg");
    if (fg == kDisabledText && asset == QStringLiteral("home.svg"))
        asset = QStringLiteral("home-disabled.svg");
    fillRounded(p, QRectF(0, 0, bounds.width(), bounds.height()), 0, bg, border, 1);
    if (startPlacement) {
        drawIcon(p, QRectF(9, 16, 16, 16), asset);
        drawText(p, QRectF(33, 13, bounds.width() - 33, 22), label, 14, QFont::DemiBold, fg,
                 Qt::AlignLeft | Qt::AlignVCenter);
    } else {
        drawIcon(p, QRectF((bounds.width() - 16) / 2.0, 9, 16, 16), asset);
        drawText(p, QRectF(0, 33, bounds.width(), 22), label, 14, QFont::DemiBold, fg,
                 Qt::AlignCenter);
    }
    p.restore();
}

} // namespace

bool hasCompositeGallery(const QString& pageId) {
    return pageId == "380:2" || pageId == "392:2" || pageId == "1307:2";
}

QWidget* createCompositeGallery(const QString& pageId,
                                const commonWidget::ComponentFactory& factory,
                                QWidget* parent) {
    auto* canvas = new CompositeCanvas;
    canvas->setObjectName("figmaGalleryCanvas");
    canvas->setStyleSheet("QWidget#figmaGalleryCanvas {background:white;border-radius:16px;}");
    if (pageId == "380:2") {
        canvas->setFixedSize(2576, 256);
        canvas->setProperty("galleryNodeId", "2013:28");
        canvas->setProperty("variantSpecimenCount", 2);
        canvas->setProperty("componentSetCount", 1);
        canvas->paintContent = [canvas](QPainter& p) {
            drawKey(p, QRectF(917, 28, 743, 39),
                    {QStringLiteral("Assets · 1 groups"),
                     QStringLiteral("Variants · component set / component / frame"),
                     QStringLiteral("States · default / hover / selected / disabled / custom")},
                    {QRectF(929, 40, 99, 15), QRectF(1052, 40, 266, 15),
                     QRectF(1342, 40, 306, 15)});
        };
        auto* comfortable = qobject_cast<commonWidget::Z_Navbar*>(
            factory.create(QStringLiteral("commonWidget::Z_Navbar"), canvas));
        auto* compact = qobject_cast<commonWidget::Z_Navbar*>(
            factory.create(QStringLiteral("commonWidget::Z_Navbar"), canvas));
        if (!comfortable || !compact)
            throw std::runtime_error("SDK Factory did not create Navbar");
        comfortable->setObjectName(QStringLiteral("figmaNavbarComfortable"));
        comfortable->setVariantTuple(QStringLiteral("Density=Comfortable"));
        comfortable->setGeometry(68, 125, 1200, 63);
        comfortable->setProperty("figmaNodeId", QStringLiteral("380:21"));
        comfortable->setProperty("figmaDesignBounds", QRect(68, 125, 1200, 63));
        compact->setObjectName(QStringLiteral("figmaNavbarCompact"));
        compact->setVariantTuple(QStringLiteral("Density=Compact"));
        compact->setDensity(QStringLiteral("Compact"));
        compact->setGeometry(1308, 125, 1200, 56);
        compact->setProperty("figmaNodeId", QStringLiteral("380:73"));
        compact->setProperty("figmaDesignBounds", QRect(1308, 125, 1200, 56));
        canvas->setProperty("runtimeSpecimenCount", 2);
    } else if (pageId == "392:2") {
        canvas->setFixedSize(1312, 1059);
        canvas->setProperty("galleryNodeId", "2010:1360");
        canvas->setProperty("variantSpecimenCount", 20);
        canvas->setProperty("componentSetCount", 2);
        canvas->paintContent = [canvas](QPainter& p) {
            drawKey(p, QRectF(331, 28, 650, 39),
                    {QStringLiteral("Density · Comfortable / Compact"),
                     QStringLiteral("Collapsed · False / True"),
                     QStringLiteral("State · Default / Hover / Selected / Disabled")},
                    {QRectF(343, 40, 191, 15), QRectF(558, 40, 136, 15),
                     QRectF(718, 40, 251, 15)});
            struct NavItemSpec {
                int x, y, w, h;
                const char* state;
                bool collapsed;
            };
            const NavItemSpec items[] = {
                {96, 109, 248, 46, "Default", false},  {376, 109, 248, 46, "Hover", false},
                {656, 109, 248, 46, "Selected", false}, {936, 109, 248, 46, "Disabled", false},
                {196, 179, 48, 46, "Default", true},   {476, 179, 48, 46, "Hover", true},
                {756, 179, 48, 46, "Selected", true},  {1036, 179, 48, 46, "Disabled", true},
                {96, 249, 248, 40, "Default", false},  {376, 249, 248, 40, "Hover", false},
                {656, 249, 248, 40, "Selected", false}, {936, 249, 248, 40, "Disabled", false},
                {196, 319, 48, 40, "Default", true},   {476, 319, 48, 40, "Hover", true},
                {756, 319, 48, 40, "Selected", true},  {1036, 319, 48, 40, "Disabled", true},
            };
            for (const auto& item : items)
                paintNavItem(p, QRect(item.x, item.y, item.w, item.h),
                             QStringLiteral("Navigation item"), item.state, item.collapsed,
                             item.h == 40);
        };
        struct SidebarRuntimeSpec {
            QRect bounds;
            const char* tuple;
            const char* nodeId;
        };
        const SidebarRuntimeSpec specimens[] = {
            {QRect(236, 471, 280, 520), "Density=Comfortable, Collapsed=False", "392:216"},
            {QRect(556, 471, 80, 520), "Density=Comfortable, Collapsed=True", "392:216"},
            {QRect(676, 471, 280, 466), "Density=Compact, Collapsed=False", "392:216"},
            {QRect(996, 471, 80, 466), "Density=Compact, Collapsed=True", "392:216"},
        };
        for (int index = 0; index < 4; ++index) {
            auto* sidebar = qobject_cast<commonWidget::Z_Sidebar*>(
                factory.create(QStringLiteral("commonWidget::Z_Sidebar"), canvas));
            if (!sidebar)
                throw std::runtime_error("SDK Factory did not create Sidebar");
            sidebar->setObjectName(QStringLiteral("figmaSidebar%1").arg(index));
            sidebar->setVariantTuple(QString::fromLatin1(specimens[index].tuple));
            sidebar->setGeometry(specimens[index].bounds);
            sidebar->setProperty("figmaNodeId", QString::fromLatin1(specimens[index].nodeId));
            sidebar->setProperty("figmaDesignBounds", specimens[index].bounds);
        }
        canvas->setProperty("runtimeSpecimenCount", 4);
    } else if (pageId == "1307:2") {
        canvas->setFixedSize(1312, 846);
        canvas->setProperty("galleryNodeId", "2014:66");
        canvas->setProperty("variantSpecimenCount", 20);
        canvas->setProperty("componentSetCount", 3);
        canvas->paintContent = [canvas](QPainter& p) {
            drawKey(p, QRectF(284, 28, 745, 39),
                    {QStringLiteral("Assets · 4 groups"),
                     QStringLiteral("Variants · component set / component / frame"),
                     QStringLiteral("States · default / hover / selected / disabled / custom")},
                    {QRectF(296, 40, 101, 15), QRectF(421, 40, 266, 15),
                     QRectF(711, 40, 306, 15)});
            const QStringList states{QStringLiteral("Default"), QStringLiteral("Hover"),
                                     QStringLiteral("Pressed"), QStringLiteral("Focus"),
                                     QStringLiteral("Selected"), QStringLiteral("Disabled")};
            for (int i = 0; i < 6; ++i)
                paintSubItem(p, QRect(440 + (i % 3) * 152, 85 + (i / 3) * 56, 82, 32),
                             QStringLiteral("子项菜单"), states[i]);
            p.fillRect(QRectF(655, 215, 1, 24), kBorderSubtle);
            for (int row = 0; row < 2; ++row)
                for (int col = 0; col < 6; ++col) {
                    const bool startPlacement = row == 1;
                    paintMainItem(p,
                                  QRect(88 + col * 192, 257 + row * 88,
                                        startPlacement ? 70 : 120, startPlacement ? 48 : 64),
                                  QStringLiteral("首页"), states[col], startPlacement,
                                  QStringLiteral("home.svg"));
                }
        };
        auto* top = qobject_cast<commonWidget::Z_DesktopBottomNavigation*>(
            factory.create(QStringLiteral("commonWidget::Z_DesktopBottomNavigation"), canvas));
        if (!top)
            throw std::runtime_error("SDK Factory did not create DesktopBottomNavigation");
        top->setObjectName(QStringLiteral("figmaDesktopBottomNavigationTop"));
        top->setVariantTuple(QStringLiteral("IconPlacement=Top"));
        top->setGeometry(236, 475, 840, 138);
        top->setProperty("figmaNodeId", QStringLiteral("1312:26"));
        top->setProperty("figmaDesignBounds", QRect(236, 475, 840, 138));
        auto* start = qobject_cast<commonWidget::Z_DesktopBottomNavigation*>(
            factory.create(QStringLiteral("commonWidget::Z_DesktopBottomNavigation"), canvas));
        if (!start)
            throw std::runtime_error("SDK Factory did not create DesktopBottomNavigation");
        start->setObjectName(QStringLiteral("figmaDesktopBottomNavigationStart"));
        start->setVariantTuple(QStringLiteral("IconPlacement=Start"));
        start->setGeometry(236, 656, 840, 122);
        start->setProperty("figmaNodeId", QStringLiteral("1312:58"));
        start->setProperty("figmaDesignBounds", QRect(236, 656, 840, 122));
        canvas->setProperty("runtimeSpecimenCount", 2);
    } else {
        throw std::runtime_error("No composite gallery for page");
    }
    return new FigmaGalleryView(canvas, parent);
}

} // namespace native_gallery
