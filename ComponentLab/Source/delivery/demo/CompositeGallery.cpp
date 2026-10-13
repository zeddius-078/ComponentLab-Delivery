#include "CompositeGallery.h"
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
const QColor kOnBrand(Qt::white);
const QColor kMutedBg(0xf3, 0xf3, 0xf3);
const QColor kBorderSubtle(0xd0, 0xd0, 0xd0);
const QColor kBorderInput(0xa7, 0xa7, 0xa7);
const QColor kPlaceholder(0x67, 0x67, 0x67);
const QColor kDisabledText(0x8a, 0x8a, 0x8a);
const QColor kKeyBg(0xf7, 0xf9, 0xfc);
const QColor kKeyText(0x50, 0x58, 0x68);

QFont inter(int px, QFont::Weight weight) {
    QFont font(QStringLiteral("Inter"));
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

void paintNavbarSpecimen(QPainter& p, const QRect& bounds, bool compact) {
    p.save();
    p.translate(bounds.topLeft());
    p.fillRect(QRectF(0, 0, bounds.width(), bounds.height()), Qt::white);
    const qreal brandX = compact ? 16 : 24;
    const qreal brandY = compact ? 12 : 15.5;
    const qreal brandLogoX = brandX;
    fillRounded(p, QRectF(brandLogoX, brandY, 32, 32), 10, kAccent);
    drawText(p, QRectF(brandLogoX, brandY + 7, 32, 18), QStringLiteral("CL"), 12,
             QFont::DemiBold, kOnBrand, Qt::AlignCenter);
    drawText(p, QRectF(brandX + 42, brandY + 5, 104, 22), QStringLiteral("ComponentLab"), 14,
             QFont::Medium, kText, Qt::AlignLeft | Qt::AlignVCenter);
    const qreal searchX = brandX + 146 + (compact ? 12 : 16);
    const qreal searchY = compact ? 12 : 11.5;
    const qreal searchH = compact ? 32 : 40;
    const qreal searchPad = compact ? 12 : 16;
    fillRounded(p, QRectF(searchX, searchY, 320, searchH), 4, Qt::white, kBorderInput, 1);
    drawIcon(p, QRectF(searchX + searchPad, searchY + (searchH - 14) / 2, 14, 14),
             QStringLiteral("search.svg"));
    drawText(p, QRectF(searchX + searchPad + 22, searchY + (searchH - 20) / 2,
                       320 - searchPad - 22, 20),
             QStringLiteral("Search workspace"), 14, QFont::Normal, kPlaceholder,
             Qt::AlignLeft | Qt::AlignVCenter);
    const qreal notifyX = compact ? 993 : 971;
    const qreal notifyY = compact ? 12 : 10.5;
    const qreal notifySize = compact ? 32 : 42;
    drawIcon(p, QRectF(notifyX, notifyY, notifySize, notifySize),
             compact ? "notifications-compact.svg" : "notifications.svg");
    const qreal profileX = compact ? 1037 : 1029;
    const qreal profileY = compact ? 12 : 10.5;
    const qreal avatarY = compact ? profileY : profileY + 5;
    fillRounded(p, QRectF(profileX, avatarY, 32, 32), 16, kAccent);
    drawText(p, QRectF(profileX, avatarY + 7, 32, 18), QStringLiteral("AM"), 12,
             QFont::DemiBold, kOnBrand, Qt::AlignCenter);
    drawText(p, QRectF(profileX + 40, profileY + (compact ? 5 : 10), 83, 22),
             QStringLiteral("Alex Morgan"), 14, QFont::Normal, kText,
             Qt::AlignLeft | Qt::AlignVCenter);
    drawIcon(p, QRectF(profileX + 131, profileY + (compact ? 8 : 13), 16, 16),
             QStringLiteral("chevron-down.svg"));
    p.fillRect(QRectF(0, bounds.height() - 1, bounds.width(), 1), kBorderSubtle);
    p.restore();
}

void paintSidebarSpecimen(QPainter& p, const QRect& bounds, bool collapsed, bool compact) {
    p.save();
    p.translate(bounds.topLeft());
    p.fillRect(QRectF(0, 0, bounds.width(), bounds.height()), Qt::white);
    const int itemH = compact ? 40 : 46;
    const qreal innerW = collapsed ? 48 : 248;
    const QRectF logo(16, 20, 32, 32);
    fillRounded(p, logo, 10, kAccent);
    drawText(p, QRectF(logo.x(), logo.y() + 7, 32, 18), QStringLiteral("CL"), 12,
             QFont::DemiBold, kOnBrand, Qt::AlignCenter);
    if (!collapsed) {
        drawText(p, QRectF(16 + 42, 16 + 9, 180, 22), QStringLiteral("ComponentLab"), 14,
                 QFont::Medium, kText, Qt::AlignLeft | Qt::AlignVCenter);
        drawIcon(p, QRectF(16 + 232, 16 + 12, 16, 16), QStringLiteral("chevron-left.svg"));
    } else {
        drawIcon(p, QRectF(16 + 40, 16 + 12, 16, 16), QStringLiteral("chevron-right.svg"));
    }
    p.fillRect(QRectF(16, 66, innerW, 1), kBorderSubtle);
    const QStringList labels{QStringLiteral("Dashboard"), QStringLiteral("Orders"),
                             QStringLiteral("Products"), QStringLiteral("Integrations"),
                             QStringLiteral("Chats")};
    const qreal step = compact ? 44 : 50;
    for (int i = 0; i < labels.size(); ++i)
        paintNavItem(p, QRect(16, 77 + int(i * step), int(innerW), itemH), labels[i],
                     i == 0 ? QStringLiteral("Selected") : QStringLiteral("Default"),
                     collapsed, compact);
    const qreal settingsY = compact ? 410 : 458;
    paintNavItem(p, QRect(16, int(settingsY), int(innerW), itemH), QStringLiteral("Settings"),
                 QStringLiteral("Default"), collapsed, compact);
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

void paintDbnSpecimen(QPainter& p, const QRect& bounds, bool startPlacement) {
    p.save();
    p.translate(bounds.topLeft());
    fillRounded(p, QRectF(0, 0, bounds.width(), bounds.height()), 0, Qt::white, kBorderSubtle,
                1);
    p.fillRect(QRectF(9, 9, 822, 48), kMutedBg);
    paintSubItem(p, QRect(321, 17, 55, 32), QStringLiteral("概览"),
                 QStringLiteral("Selected"));
    p.fillRect(QRectF(384, 21, 1, 24), kBorderSubtle);
    paintSubItem(p, QRect(393, 17, 55, 32), QStringLiteral("最近"),
                 QStringLiteral("Default"));
    p.fillRect(QRectF(456, 21, 1, 24), kBorderSubtle);
    paintSubItem(p, QRect(465, 17, 55, 32), QStringLiteral("收藏"),
                 QStringLiteral("Default"));
    const qreal tierY = 65;
    if (startPlacement) {
        paintMainItem(p, QRect(9, int(tierY), 71, 48), QStringLiteral("首页"),
                      QStringLiteral("Selected"), true, QStringLiteral("home.svg"));
        paintMainItem(p, QRect(385, int(tierY), 71, 48), QStringLiteral("内容"),
                      QStringLiteral("Default"), true, QStringLiteral("document.svg"));
        paintMainItem(p, QRect(760, int(tierY), 71, 48), QStringLiteral("设置"),
                      QStringLiteral("Default"), true, QStringLiteral("settings.svg"));
    } else {
        paintMainItem(p, QRect(9, int(tierY), 120, 64), QStringLiteral("首页"),
                      QStringLiteral("Selected"), false, QStringLiteral("home.svg"));
        paintMainItem(p, QRect(360, int(tierY), 120, 64), QStringLiteral("内容"),
                      QStringLiteral("Default"), false, QStringLiteral("document.svg"));
        paintMainItem(p, QRect(711, int(tierY), 120, 64), QStringLiteral("设置"),
                      QStringLiteral("Default"), false, QStringLiteral("settings.svg"));
    }
    p.restore();
}

} // namespace

bool hasCompositeGallery(const QString& pageId) {
    return pageId == "380:2" || pageId == "392:2" || pageId == "1307:2";
}

QWidget* createCompositeGallery(const QString& pageId, QWidget* parent) {
    auto* canvas = new CompositeCanvas;
    canvas->setObjectName("figmaGalleryCanvas");
    canvas->setStyleSheet("QWidget#figmaGalleryCanvas {background:white;border-radius:16px;}");
    canvas->setAttribute(Qt::WA_TransparentForMouseEvents);
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
            paintNavbarSpecimen(p, QRect(68, 125, 1200, 63), false);
            paintNavbarSpecimen(p, QRect(1308, 125, 1200, 56), true);
        };
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
            paintSidebarSpecimen(p, QRect(236, 471, 280, 520), false, false);
            paintSidebarSpecimen(p, QRect(556, 471, 80, 520), true, false);
            paintSidebarSpecimen(p, QRect(676, 471, 280, 466), false, true);
            paintSidebarSpecimen(p, QRect(996, 471, 80, 466), true, true);
        };
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
        auto* top = new commonWidget::Z_DesktopBottomNavigation(canvas);
        top->setObjectName(QStringLiteral("figmaDesktopBottomNavigationTop"));
        top->setVariantTuple(QStringLiteral("IconPlacement=Top"));
        top->setGeometry(236, 475, 840, 138);
        top->setProperty("figmaNodeId", QStringLiteral("1312:26"));
        top->setProperty("figmaDesignBounds", QRect(236, 475, 840, 138));
        auto* start = new commonWidget::Z_DesktopBottomNavigation(canvas);
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
