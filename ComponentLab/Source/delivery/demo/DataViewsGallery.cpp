#include "DataViewsGallery.h"
#include "ComponentTheme.h"
#include "FigmaGalleryView.h"
#include "TokenResources.h"
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonValue>
#include <QPainter>
#include <QPainterPath>
#include <QPaintEvent>
#include <QSvgRenderer>
#include <QtMath>
#include <functional>
#include <stdexcept>

namespace native_gallery {
namespace {

const QColor kText(0x19, 0x19, 0x19);
const QColor kAccent(0x62, 0x35, 0xf7);
const QColor kMutedBg(0xf3, 0xf3, 0xf3);
const QColor kBorderSubtle(0xd0, 0xd0, 0xd0);
const QColor kMutedText(0x44, 0x44, 0x44);
const QColor kDisabledText(0x8a, 0x8a, 0x8a);
const QColor kIconBg(0xf3, 0xef, 0xff);
const QColor kHoverBg(0xf3, 0xf3, 0xf3);
const QColor kSelectedBg(0xf3, 0xef, 0xff);
const QColor kSelectedHoverBg(0xf1, 0xec, 0xff);
const QColor kKeyBg(0xf7, 0xf9, 0xfc);
const QColor kKeyText(0x50, 0x58, 0x68);

QJsonValue tableToken(const QString& path) {
    static const QJsonObject profile = [] {
        QJsonParseError error;
        const auto document = QJsonDocument::fromJson(
            token_resources::read(QStringLiteral(":/tokens/tokens_qt.json")), &error);
        if (error.error != QJsonParseError::NoError || !document.isObject())
            throw std::runtime_error("Invalid generated V2 Token runtime document");
        const auto selected = document.object()[QStringLiteral("profiles")]
                                  .toObject()[component_theme::activeProfile()].toObject();
        if (selected.isEmpty())
            throw std::runtime_error("Active Table Token profile is unavailable");
        return selected;
    }();
    for (const auto& category : {"color", "layout", "number", "font", "boolean", "string", "asset"}) {
        const auto values = profile[QString::fromLatin1(category)].toObject();
        if (values.contains(path))
            return values[path];
    }
    throw std::runtime_error(("Missing generated V2 Table token: " + path).toStdString());
}

QColor tableColor(const QString& path) {
    const QColor result(tableToken(path).toString());
    if (!result.isValid())
        throw std::runtime_error(("Invalid generated V2 Table color: " + path).toStdString());
    return result;
}

int tableNumber(const QString& path) {
    const auto result = tableToken(path);
    if (!result.isDouble())
        throw std::runtime_error(("Non-numeric generated V2 Table token: " + path).toStdString());
    return qRound(result.toDouble());
}

QFont::Weight tableFontWeight(int cssWeight) {
    if (cssWeight <= 400) return QFont::Normal;
    if (cssWeight <= 500) return QFont::Medium;
    if (cssWeight <= 600) return QFont::DemiBold;
    return QFont::Bold;
}

struct TableVisualTokens {
    QColor container;
    QColor header;
    QColor headerText;
    QColor row;
    QColor hover;
    QColor selected;
    QColor disabled;
    QColor cellText;
    QColor disabledText;
    QColor border;
    int borderWidth;
    int cellPaddingInline;
    int headerPaddingInline;
    int headerHeight;
    int cellFontSize;
    int headerFontSize;
    QFont::Weight cellWeight;
    QFont::Weight headerWeight;
};

const TableVisualTokens& tableVisualTokens() {
    static const TableVisualTokens tokens{
        tableColor(QStringLiteral("component.table.base.shared.slot.container.background-color")),
        tableColor(QStringLiteral("component.table.base.shared.slot.header.background-color")),
        tableColor(QStringLiteral("component.table.base.shared.slot.header.color")),
        tableColor(QStringLiteral("component.table.base.striping.none.slot.row.background-color")),
        tableColor(QStringLiteral("component.table.overlay.row-interaction.hover.slot.row.background-color")),
        tableColor(QStringLiteral("component.table.overlay.cell-selection.selected.slot.cell.background-color")),
        tableColor(QStringLiteral("component.table.overlay.availability.disabled.slot.container.background-color")),
        tableColor(QStringLiteral("component.table.base.shared.slot.cell.color")),
        tableColor(QStringLiteral("component.table.overlay.availability.disabled.slot.cell.color")),
        tableColor(QStringLiteral("component.table.base.shared.slot.border.color")),
        tableNumber(QStringLiteral("component.table.base.shared.slot.border.border-width")),
        tableNumber(QStringLiteral("component.table.base.shared.slot.cell.padding-inline")),
        tableNumber(QStringLiteral("component.table.base.shared.slot.header.padding-inline")),
        tableNumber(QStringLiteral("component.table.base.shared.slot.header.line-height")) +
            2 * tableNumber(QStringLiteral("component.table.base.shared.slot.header.padding-block")),
        tableNumber(QStringLiteral("component.table.base.shared.slot.cell.font-size")),
        tableNumber(QStringLiteral("component.table.base.shared.slot.header.font-size")),
        tableFontWeight(tableNumber(QStringLiteral("component.table.base.shared.slot.cell.font-weight"))),
        tableFontWeight(tableNumber(QStringLiteral("component.table.base.shared.slot.header.font-weight"))),
    };
    return tokens;
}

QString tableDensityForLegacySize(const QString& sizeName) {
    if (sizeName == QStringLiteral("Small")) return QStringLiteral("compact");
    if (sizeName == QStringLiteral("Medium")) return QStringLiteral("small");
    return QStringLiteral("medium");
}

int tableRowHeight(const QString& sizeName) {
    return tableNumber(QStringLiteral("component.table.base.density.%1.slot.row.min-height")
                           .arg(tableDensityForLegacySize(sizeName)));
}

int tableSpecimenHeight(const QString& sizeName) {
    return tableVisualTokens().headerHeight + 3 * tableRowHeight(sizeName) +
           2 * tableVisualTokens().borderWidth;
}

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

class DataViewsCanvas final : public QWidget {
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

// One Density x State list specimen. The fixed state applies to Item/1 only, matching
// the Figma contract labels: Hover/Selected/Selected-hover/Disabled/Focus-visible.
void paintListSpecimen(QPainter& p, const QRect& bounds, const QString& contractLabel,
                       const QStringList& primaries, const QStringList& secondaries,
                       const QStringList& trailings, const QString& state,
                       bool comfortable) {
    const int itemH = comfortable ? 56 : 40;
    const bool hover = state == "Hover";
    const bool selected = state == "Selected" || state == "Selected-hover";
    const bool selectedHover = state == "Selected-hover";
    const bool disabled = state == "Disabled";
    const bool focus = state == "Focus-visible";
    fillRounded(p, QRectF(bounds), 8, Qt::white, kBorderSubtle, 1);
    p.save();
    p.setClipPath(roundedPath(QRectF(bounds).adjusted(1, 1, -1, -1), 7), Qt::IntersectClip);
    p.fillRect(QRectF(1, 1, bounds.width() - 2, 32), kMutedBg);
    drawText(p, QRectF(13, 1, 260, 32), contractLabel, 10, QFont::Normal, kMutedText,
             Qt::AlignLeft | Qt::AlignVCenter);
    drawText(p, QRectF(1, 1, bounds.width() - 14, 32),
             comfortable ? QStringLiteral("COMFORTABLE") : QStringLiteral("COMPACT"), 10,
             QFont::Normal, kMutedText, Qt::AlignRight | Qt::AlignVCenter);
    for (int i = 0; i < 4; ++i) {
        const QRectF item(1, 33 + i * itemH, bounds.width() - 2, itemH);
        p.save();
        if (disabled)
            p.setOpacity(0.82);
        if (i == 1) {
            QColor bg = Qt::white;
            if (hover)
                bg = kHoverBg;
            else if (selected)
                bg = selectedHover ? kSelectedHoverBg : kSelectedBg;
            else if (disabled)
                bg = kHoverBg;
            if (bg != Qt::white)
                p.fillRect(item, bg);
            if (focus) {
                p.setPen(QPen(kAccent, 1));
                p.drawRect(item.adjusted(0.5, 0.5, -0.5, -0.5));
            }
        }
        const int pad = comfortable ? 12 : 8;
        const int iconSize = comfortable ? 28 : 22;
        const qreal iconY = item.y() + (itemH - iconSize) / 2.0;
        fillRounded(p, QRectF(item.x() + pad, iconY, iconSize, iconSize), 6,
                    (i == 1 && selected) ? kAccent : kIconBg);
        const qreal contentX = item.x() + pad + iconSize + 10;
        const QColor fg = disabled ? kDisabledText : kText;
        if (comfortable) {
            drawText(p, QRectF(contentX, item.y() + 8, item.width() - contentX - 90, 22),
                     primaries[i], 14, (i == 1 && !disabled) ? QFont::DemiBold : QFont::Normal,
                     fg, Qt::AlignLeft | Qt::AlignVCenter);
            if (!secondaries[i].isEmpty())
                drawText(p, QRectF(contentX, item.y() + 32, item.width() - contentX - 90, 18),
                         secondaries[i], 12, QFont::Normal, disabled ? kDisabledText : kMutedText,
                         Qt::AlignLeft | Qt::AlignVCenter);
        } else {
            drawText(p, QRectF(contentX, item.y(), item.width() - contentX - 80, itemH),
                     primaries[i], 13, QFont::Normal, fg, Qt::AlignLeft | Qt::AlignVCenter);
        }
        drawText(p, QRectF(item.width() - 8 - 200, item.y(), 200, itemH), trailings[i], 12,
                 QFont::Normal, disabled ? kDisabledText : kMutedText,
                 Qt::AlignRight | Qt::AlignVCenter);
        p.restore();
    }
    p.restore();
}

struct ListGalleryDef {
    const char* galleryId;
    const char* contractLabel;
    QStringList primaries;
    QStringList secondaries;
    QStringList trailings;
};

const ListGalleryDef& listGallery(const QString& pageId) {
    static const ListGalleryDef listView{
        "2016:22888", "MODEL / VIEW",
        {QStringLiteral("Alpha index"), QStringLiteral("Beta index"),
         QStringLiteral("Gamma index"), QStringLiteral("Delta index")},
        {QStringLiteral("Model row 0"), QStringLiteral("Proxy result"),
         QStringLiteral("Delegate preview"), QStringLiteral("Model row 3")},
        {QStringLiteral("#0"), QStringLiteral("current"), QStringLiteral("#2"),
         QStringLiteral("#3")}};
    static const ListGalleryDef listWidget{
        "2016:23407", "ITEM OWNED",
        {QStringLiteral("Alpha item"), QStringLiteral("Beta item"),
         QStringLiteral("Gamma item"), QStringLiteral("Delta item")},
        {QStringLiteral("QListWidgetItem"), QStringLiteral("currentRow = 1"),
         QStringLiteral("sortingEnabled"), QStringLiteral("itemChanged signal")},
        {QStringLiteral("#0"), QStringLiteral("row 1"), QStringLiteral("#2"),
         QStringLiteral("#3")}};
    if (pageId == "707:19")
        return listView;
    if (pageId == "709:2")
        return listWidget;
    throw std::runtime_error("No data-views gallery for page");
}

// One Density x State tree specimen. Disclosure kind per row:
// 0 = expanded chevron, 1 = indent + leaf, 2 = indent + collapsed chevron, 3 = indent x2 + leaf.
void paintTreeSpecimen(QPainter& p, const QRect& bounds, const QString& contractLabel,
                       const QStringList& primaries, const QStringList& secondaries,
                       const QStringList& trailings, const QString& state, bool comfortable) {
    const int itemH = comfortable ? 56 : 40;
    const bool hover = state == "Hover";
    const bool selected = state == "Selected" || state == "Selected-hover";
    const bool selectedHover = state == "Selected-hover";
    const bool disabled = state == "Disabled";
    const bool focus = state == "Focus-visible";
    const int disclosureKind[4] = {0, 1, 2, 3};
    fillRounded(p, QRectF(bounds), 8, Qt::white, kBorderSubtle, 1);
    p.save();
    p.setClipPath(roundedPath(QRectF(bounds).adjusted(1, 1, -1, -1), 7), Qt::IntersectClip);
    p.fillRect(QRectF(1, 1, bounds.width() - 2, 32), kMutedBg);
    drawText(p, QRectF(13, 1, 300, 32), contractLabel, 10, QFont::Normal, kText,
             Qt::AlignLeft | Qt::AlignVCenter);
    drawText(p, QRectF(1, 1, bounds.width() - 14, 32),
             comfortable ? QStringLiteral("COMFORTABLE") : QStringLiteral("COMPACT"), 10,
             QFont::Normal, kText, Qt::AlignRight | Qt::AlignVCenter);
    for (int i = 0; i < 4; ++i) {
        const QRectF item(1, 33 + i * itemH, bounds.width() - 2, itemH);
        p.save();
        if (disabled)
            p.setOpacity(0.82);
        if (i == 1) {
            QColor bg = Qt::white;
            if (hover)
                bg = kHoverBg;
            else if (selected)
                bg = selectedHover ? kSelectedHoverBg : kSelectedBg;
            else if (disabled)
                bg = kHoverBg;
            if (bg != Qt::white)
                p.fillRect(item, bg);
            if (focus) {
                p.setPen(QPen(kAccent, 1));
                p.drawRect(item.adjusted(0.5, 0.5, -0.5, -0.5));
            }
        }
        const int pad = comfortable ? 12 : 8;
        const int iconSize = comfortable ? 28 : 22;
        qreal x = item.x() + pad;
        switch (disclosureKind[i]) {
        case 0:
            drawIcon(p, QRectF(x, item.y() + (itemH - 16) / 2.0, 16, 16),
                     QStringLiteral("tree-expanded.svg"));
            x += 12 + 10;
            break;
        case 1:
            p.fillRect(QRectF(x + 7, item.y() + (itemH - 12) / 2.0, 1, 12), kBorderSubtle);
            x += 16 + 10;
            drawIcon(p, QRectF(x, item.y() + (itemH - 17) / 2.0, 12, 17),
                     QStringLiteral("branch-leaf.svg"));
            x += 12 + 10;
            break;
        case 2:
            p.fillRect(QRectF(x + 7, item.y() + (itemH - 12) / 2.0, 1, 12), kBorderSubtle);
            x += 16 + 10;
            drawIcon(p, QRectF(x, item.y() + (itemH - 16) / 2.0, 16, 16),
                     QStringLiteral("tree-collapsed.svg"));
            x += 12 + 10;
            break;
        case 3:
            p.fillRect(QRectF(x + 7, item.y() + (itemH - 12) / 2.0, 1, 12), kBorderSubtle);
            x += 16 + 10;
            p.fillRect(QRectF(x + 7, item.y() + (itemH - 12) / 2.0, 1, 12), kBorderSubtle);
            x += 16 + 10;
            drawIcon(p, QRectF(x, item.y() + (itemH - 17) / 2.0, 12, 17),
                     QStringLiteral("branch-leaf.svg"));
            x += 12 + 10;
            break;
        }
        const qreal iconY = item.y() + (itemH - iconSize) / 2.0;
        fillRounded(p, QRectF(x, iconY, iconSize, iconSize), 6,
                    (i == 1 && selected) ? kAccent : kIconBg);
        const qreal contentX = x + iconSize + 10;
        const QColor fg = disabled ? kDisabledText : kText;
        if (comfortable) {
            drawText(p, QRectF(contentX, item.y() + 8, item.width() - contentX - 80, 22),
                     primaries[i], 14, (i == 1 && !disabled) ? QFont::DemiBold : QFont::Normal,
                     fg, Qt::AlignLeft | Qt::AlignVCenter);
            if (!secondaries[i].isEmpty())
                drawText(p, QRectF(contentX, item.y() + 32, item.width() - contentX - 80, 18),
                         secondaries[i], 12, QFont::Normal, disabled ? kDisabledText : kMutedText,
                         Qt::AlignLeft | Qt::AlignVCenter);
        } else {
            drawText(p, QRectF(contentX, item.y(), item.width() - contentX - 70, itemH),
                     primaries[i], 13, QFont::Normal, fg, Qt::AlignLeft | Qt::AlignVCenter);
        }
        drawText(p, QRectF(item.width() - 8 - 200, item.y(), 200, itemH), trailings[i], 12,
                 QFont::Normal, disabled ? kDisabledText : kMutedText,
                 Qt::AlignRight | Qt::AlignVCenter);
        p.restore();
    }
    p.restore();
}

struct TreeGalleryDef {
    const char* galleryId;
    const char* contractLabel;
    QStringList primaries;
    QStringList secondaries;
    QStringList trailings;
};

const TreeGalleryDef& treeGallery(const QString& pageId) {
    static const TreeGalleryDef treeView{
        "2015:27014", "MODEL / VIEW TREE",
        {QStringLiteral("Project root"), QStringLiteral("src"), QStringLiteral("assets"),
         QStringLiteral("main.cpp")},
        {QStringLiteral("4 children"), QStringLiteral("expanded · 8 files"),
         QStringLiteral("collapsed · 12 items"), QStringLiteral("leaf · C++ source")},
        {QStringLiteral("#0"), QStringLiteral("index 1"), QStringLiteral("#2"),
         QStringLiteral("#3")}};
    static const TreeGalleryDef treeWidget{
        "2015:27533", "ITEM OWNED TREE",
        {QStringLiteral("Root item"), QStringLiteral("src item"), QStringLiteral("Assets item"),
         QStringLiteral("Leaf item")},
        {QStringLiteral("4 child items"), QStringLiteral("expanded · 8 child items"),
         QStringLiteral("collapsed · 12 child items"), QStringLiteral("leaf item")},
        {QStringLiteral("#0"), QStringLiteral("current item"), QStringLiteral("#2"),
         QStringLiteral("#3")}};
    if (pageId == "712:8")
        return treeView;
    if (pageId == "713:2")
        return treeWidget;
    throw std::runtime_error("No data-views gallery for page");
}

// One Size x State table specimen (3-col fixture: Name/Status/Value, Alpha/Beta/Gamma).
// The state applies to Row/1 only; Disabled dims the whole root to 58% with muted text.
void paintTableSpecimen(QPainter& p, const QRect& bounds, const QString& sizeName,
                        const QString& state) {
    const auto& tokens = tableVisualTokens();
    const int rowH = tableRowHeight(sizeName);
    const int headerH = tokens.headerHeight;
    const int step = rowH + tokens.borderWidth;
    const bool disabled = state == "Disabled";
    const bool hover = state == "Hover";
    const bool selected = state == "Selected" || state == "Selected-hover";
    p.save();
    p.translate(bounds.topLeft());
    const QRectF localBounds(0, 0, bounds.width(), bounds.height());
    p.fillRect(localBounds, disabled ? tokens.disabled : tokens.container);
    p.setPen(QPen(tokens.border, tokens.borderWidth));
    p.drawRect(localBounds.adjusted(tokens.borderWidth / 2.0, tokens.borderWidth / 2.0,
                                    -tokens.borderWidth / 2.0, -tokens.borderWidth / 2.0));
    p.save();
    p.setClipRect(localBounds.adjusted(tokens.borderWidth, tokens.borderWidth,
                                       -tokens.borderWidth, -tokens.borderWidth));
    p.fillRect(QRectF(1, 1, 658, headerH), disabled ? tokens.disabled : tokens.header);
    p.fillRect(QRectF(1, 1 + headerH, 658, tokens.borderWidth), tokens.border);
    const QColor headerFg = disabled ? tokens.disabledText : tokens.headerText;
    const QStringList headers{QStringLiteral("Name"), QStringLiteral("Status"),
                              QStringLiteral("Value")};
    drawText(p, QRectF(1 + tokens.headerPaddingInline, 1,
                       260 - 2 * tokens.headerPaddingInline, headerH), headers[0],
             tokens.headerFontSize, tokens.headerWeight, headerFg,
             Qt::AlignLeft | Qt::AlignVCenter);
    drawIcon(p, QRectF(1 + 260 - tokens.headerPaddingInline - 16,
                       1 + (headerH - 16) / 2.0, 16, 16),
             QStringLiteral("sort.svg"));
    drawText(p, QRectF(1 + 260 + tokens.headerPaddingInline, 1,
                       180 - 2 * tokens.headerPaddingInline, headerH), headers[1],
             tokens.headerFontSize, tokens.headerWeight, headerFg,
             Qt::AlignLeft | Qt::AlignVCenter);
    drawText(p, QRectF(1 + 440 + tokens.headerPaddingInline, 1,
                       658 - 440 - 2 * tokens.headerPaddingInline, headerH), headers[2],
             tokens.headerFontSize, tokens.headerWeight, headerFg,
             Qt::AlignLeft | Qt::AlignVCenter);
    const QStringList c1{QStringLiteral("Alpha"), QStringLiteral("Beta"),
                         QStringLiteral("Gamma")};
    const QStringList c2{QStringLiteral("Ready"), QStringLiteral("Pending"),
                         QStringLiteral("Idle")};
    const QStringList c3{QStringLiteral("128"), QStringLiteral("64"), QStringLiteral("32")};
    for (int i = 0; i < 3; ++i) {
        const QRectF row(1, 1 + headerH + tokens.borderWidth + i * step, 658, rowH);
        p.fillRect(row, disabled ? tokens.disabled : tokens.row);
        if (i == 1) {
            QColor bg = tokens.row;
            if (hover)
                bg = tokens.hover;
            else if (selected)
                bg = tokens.selected;
            if (!disabled)
                p.fillRect(row, bg);
        }
        const QColor fg = disabled ? tokens.disabledText : tokens.cellText;
        drawText(p, QRectF(row.x() + tokens.cellPaddingInline, row.y(),
                           260 - 2 * tokens.cellPaddingInline, rowH), c1[i],
                 tokens.cellFontSize, tokens.cellWeight, fg,
                 Qt::AlignLeft | Qt::AlignVCenter);
        drawText(p, QRectF(row.x() + 260 + tokens.cellPaddingInline, row.y(),
                           180 - 2 * tokens.cellPaddingInline, rowH), c2[i],
                 tokens.cellFontSize, tokens.cellWeight, fg,
                 Qt::AlignLeft | Qt::AlignVCenter);
        drawText(p, QRectF(row.x() + 440 + tokens.cellPaddingInline, row.y(),
                           row.width() - 440 - 2 * tokens.cellPaddingInline, rowH), c3[i],
                 tokens.cellFontSize, tokens.cellWeight, fg,
                 Qt::AlignLeft | Qt::AlignVCenter);
        p.fillRect(QRectF(row.x(), row.y() + rowH, row.width(), tokens.borderWidth),
                   tokens.border);
    }
    for (const int x : {260, 440})
        p.fillRect(QRectF(x, 1, tokens.borderWidth, localBounds.height() - 2), tokens.border);
    p.restore();
    p.restore();
}

// One Density x State column-path specimen: three columns, the first two holding the
// permanently selected path ancestors, the state applying to the leaf item in column 2.
void paintColumnSpecimen(QPainter& p, const QRect& bounds, const QString& state,
                         bool comfortable) {
    const int itemH = comfortable ? 56 : 40;
    const bool hover = state == "Hover";
    const bool selected = state == "Selected" || state == "Selected-hover";
    const bool selectedHover = state == "Selected-hover";
    const bool disabled = state == "Disabled";
    const bool focus = state == "Focus-visible";
    fillRounded(p, QRectF(bounds), 8, Qt::white, kBorderSubtle, 1);
    p.save();
    p.setClipPath(roundedPath(QRectF(bounds).adjusted(1, 1, -1, -1), 7), Qt::IntersectClip);
    p.fillRect(QRectF(1, 1, 498, 32), kMutedBg);
    drawText(p, QRectF(13, 1, 260, 32), QStringLiteral("COLUMN MODEL"), 10, QFont::Normal,
             disabled ? kDisabledText : kMutedText, Qt::AlignLeft | Qt::AlignVCenter);
    drawText(p, QRectF(1, 1, 498 - 14, 32),
             comfortable ? QStringLiteral("COMFORTABLE") : QStringLiteral("COMPACT"), 10,
             QFont::Normal, kMutedText, Qt::AlignRight | Qt::AlignVCenter);
    const QStringList columns[3] = {
        {QStringLiteral("Workspace"), QStringLiteral("Projects"), QStringLiteral("Archive"),
         QStringLiteral("Shared")},
        {QStringLiteral("ComponentLab"), QStringLiteral("DesignSystem"),
         QStringLiteral("Runtime"), QStringLiteral("Tests")},
        {QStringLiteral("Button.cpp"), QStringLiteral("Input.cpp"), QStringLiteral("Tree.cpp"),
         QStringLiteral("Tokens.json")}};
    const int colX[3] = {1, 172, 343};
    const int colW[3] = {155, 155, 156};
    for (int c = 0; c < 3; ++c) {
        if (c < 2)
            p.fillRect(QRectF(colX[c] + colW[c], 33, 1, bounds.height() - 34), kBorderSubtle);
        for (int i = 0; i < 4; ++i) {
            const QRectF item(colX[c], 33 + i * itemH, colW[c], itemH);
            QColor bg = Qt::white;
            if ((c == 0 && i == 1) || (c == 1 && i == 0))
                bg = kSelectedBg;
            else if (c == 2 && i == 0) {
                if (hover)
                    bg = kHoverBg;
                else if (selected)
                    bg = selectedHover ? kSelectedHoverBg : kSelectedBg;
                else if (disabled)
                    bg = kHoverBg;
            }
            p.save();
            if (disabled)
                p.setOpacity(0.82);
            if (bg != Qt::white)
                p.fillRect(item, bg);
            if (c == 2 && i == 0 && focus) {
                p.setPen(QPen(kAccent, 1));
                p.drawRect(item.adjusted(0.5, 0.5, -0.5, -0.5));
            }
            const int pad = comfortable ? 12 : 8;
            const QColor fg = disabled ? kDisabledText : kText;
            drawText(p, QRectF(item.x() + pad, item.y() + (comfortable ? 8 : 4),
                               item.width() - 2 * pad, 18),
                     columns[c][i], 12,
                     (c == 2 && i == 0 && !disabled) ? QFont::DemiBold : QFont::Normal, fg,
                     Qt::AlignLeft | Qt::AlignVCenter);
            p.restore();
        }
    }
    p.restore();
}

// One Density x State undo-stack specimen: four commands with CURRENT/CLEAN markers.
// Empty shows only the centered empty label; the state applies to Item/1.
void paintUndoSpecimen(QPainter& p, const QRect& bounds, const QString& state,
                       bool comfortable) {
    const int itemH = comfortable ? 56 : 40;
    const bool hover = state == "Hover";
    const bool selected = state == "Selected" || state == "Selected-hover";
    const bool selectedHover = state == "Selected-hover";
    const bool disabled = state == "Disabled";
    const bool focus = state == "Focus-visible";
    const bool empty = state == "Empty";
    fillRounded(p, QRectF(bounds), 8, Qt::white, kBorderSubtle, 1);
    p.save();
    p.setClipPath(roundedPath(QRectF(bounds).adjusted(1, 1, -1, -1), 7), Qt::IntersectClip);
    p.fillRect(QRectF(1, 1, 498, 32), kMutedBg);
    drawText(p, QRectF(13, 1, 200, 32), QStringLiteral("UNDO STACK"), 10, QFont::Normal,
             disabled ? kDisabledText : kMutedText, Qt::AlignLeft | Qt::AlignVCenter);
    drawText(p, QRectF(1, 1, 498 - 14, 32),
             comfortable ? QStringLiteral("COMFORTABLE") : QStringLiteral("COMPACT"), 10,
             QFont::Normal, kMutedText, Qt::AlignRight | Qt::AlignVCenter);
    if (empty) {
        drawText(p, QRectF(1, 33, 498, bounds.height() - 34), QStringLiteral("No commands yet"),
                 10, QFont::Normal, kMutedText, Qt::AlignCenter);
        p.restore();
        return;
    }
    const QStringList labels{QStringLiteral("Rename layer"), QStringLiteral("Move component"),
                             QStringLiteral("Edit token"), QStringLiteral("Initial state")};
    const QStringList secondary{QStringLiteral("command #3"), QStringLiteral("current command"),
                                QStringLiteral("clean point"), QStringLiteral("stack origin")};
    const QStringList badges{QStringLiteral("#0"), QStringLiteral("CURRENT"),
                             QStringLiteral("CLEAN"), QStringLiteral("#3")};
    for (int i = 0; i < 4; ++i) {
        const QRectF item(1, 33 + i * itemH, 498, itemH);
        p.save();
        if (disabled)
            p.setOpacity(0.82);
        if (i == 1) {
            QColor bg = Qt::white;
            if (hover)
                bg = kHoverBg;
            else if (selected)
                bg = selectedHover ? kSelectedHoverBg : kSelectedBg;
            else if (disabled)
                bg = kHoverBg;
            if (bg != Qt::white)
                p.fillRect(item, bg);
            if (focus) {
                p.setPen(QPen(kAccent, 1));
                p.drawRect(item.adjusted(0.5, 0.5, -0.5, -0.5));
            }
        }
        const int pad = comfortable ? 12 : 8;
        const int iconSize = comfortable ? 28 : 22;
        const qreal iconY = item.y() + (itemH - iconSize) / 2.0;
        fillRounded(p, QRectF(item.x() + pad, iconY, iconSize, iconSize), 6,
                    (i == 1 && selected) ? kAccent : kIconBg);
        const qreal contentX = item.x() + pad + iconSize + 10;
        const QColor fg = disabled ? kDisabledText : kText;
        if (comfortable) {
            drawText(p, QRectF(contentX, item.y() + 8, item.width() - contentX - 90, 22),
                     labels[i], 14, (i == 1 && !disabled) ? QFont::DemiBold : QFont::Normal, fg,
                     Qt::AlignLeft | Qt::AlignVCenter);
            if (state != "Focus-visible")
                drawText(p, QRectF(contentX, item.y() + 32, item.width() - contentX - 90, 18),
                         secondary[i], 12, QFont::Normal,
                         disabled ? kDisabledText : kMutedText,
                         Qt::AlignLeft | Qt::AlignVCenter);
        } else {
            drawText(p, QRectF(contentX, item.y(), item.width() - contentX - 90, itemH),
                     labels[i], 13, QFont::Normal, fg, Qt::AlignLeft | Qt::AlignVCenter);
        }
        const QColor badgeFg = (i == 2 && !disabled) ? kAccent : (disabled ? kDisabledText : kMutedText);
        drawText(p, QRectF(item.width() - 8 - 60, item.y(), 60, itemH), badges[i], 12,
                 QFont::Normal, badgeFg, Qt::AlignRight | Qt::AlignVCenter);
        p.restore();
    }
    p.restore();
}

struct TableGalleryDef {
    const char* galleryId;
};

const char* tableGalleryId(const QString& pageId) {
    if (pageId == "717:2")
        return "2016:28";
    if (pageId == "1111:2")
        return "2016:2206";
    if (pageId == "363:2")
        return "2015:25457";
    return nullptr;
}

} // namespace

bool hasDataViewsGallery(const QString& pageId) {
    return pageId == "707:19" || pageId == "709:2" || pageId == "712:8" || pageId == "713:2" ||
           pageId == "717:2" || pageId == "1111:2" || pageId == "720:6" ||
           pageId == "721:5";
}

QWidget* createDataViewsGallery(const QString& pageId, QWidget* parent) {
    auto* canvas = new DataViewsCanvas;
    canvas->setObjectName("figmaGalleryCanvas");
    canvas->setStyleSheet("QWidget#figmaGalleryCanvas {background:white;border-radius:16px;}");
    canvas->setAttribute(Qt::WA_TransparentForMouseEvents);
    canvas->setFixedSize(1596, 1157);
    const bool tree = pageId == "712:8" || pageId == "713:2";
    const bool table = pageId == "717:2" || pageId == "1111:2" || pageId == "363:2";
    const bool column = pageId == "720:6";
    const bool undo = pageId == "721:5";
    if (column || undo) {
        if (undo)
            canvas->setFixedSize(1596, 1445);
        canvas->setProperty("galleryNodeId",
                            column ? "2016:1168" : "2016:1687");
        canvas->setProperty("variantSpecimenCount", column ? 12 : 14);
        canvas->setProperty("componentSetCount", 1);
        canvas->paintContent = [canvas, column](QPainter& p) {
            drawKey(p, QRectF(427, 28, 743, 39),
                    {QStringLiteral("Assets · 1 groups"),
                     QStringLiteral("Variants · component set / component / frame"),
                     QStringLiteral("States · default / hover / selected / disabled / custom")},
                    {QRectF(439, 40, 99, 15), QRectF(562, 40, 266, 15),
                     QRectF(852, 40, 306, 15)});
            struct Spec {
                int x, y;
                const char* state;
                bool comfortable;
            };
            if (column) {
                const Spec specs[] = {
                    {28, 85, "Default", false},        {548, 85, "Hover", false},
                    {1068, 85, "Selected", false},     {1068, 309, "Disabled", false},
                    {28, 309, "Selected-hover", false}, {548, 309, "Focus-visible", false},
                    {28, 553, "Default", true},        {548, 553, "Hover", true},
                    {1068, 553, "Selected", true},     {1068, 841, "Disabled", true},
                    {28, 841, "Selected-hover", true}, {548, 841, "Focus-visible", true},
                };
                for (const auto& spec : specs)
                    paintColumnSpecimen(p,
                                        QRect(spec.x, spec.y, 500, spec.comfortable ? 258 : 194),
                                        spec.state, spec.comfortable);
            } else {
                const Spec specs[] = {
                    {28, 85, "Default", false},        {548, 85, "Hover", false},
                    {1068, 85, "Selected", false},     {1068, 309, "Disabled", false},
                    {28, 309, "Selected-hover", false}, {548, 309, "Focus-visible", false},
                    {28, 553, "Empty", false},
                    {28, 841, "Default", true},        {548, 841, "Hover", true},
                    {1068, 841, "Selected", true},     {1068, 1129, "Disabled", true},
                    {28, 1129, "Selected-hover", true}, {548, 1129, "Focus-visible", true},
                    {548, 553, "Empty", true},
                };
                for (const auto& spec : specs)
                    paintUndoSpecimen(p,
                                      QRect(spec.x, spec.y, 500, spec.comfortable ? 258 : 194),
                                      spec.state, spec.comfortable);
            }
        };
    } else if (table) {
        canvas->setFixedSize(2116, 1373);
        canvas->setProperty("galleryNodeId", tableGalleryId(pageId));
        canvas->setProperty("variantSpecimenCount", 15);
        canvas->setProperty("componentSetCount", 1);
        canvas->paintContent = [canvas](QPainter& p) {
            drawKey(p, QRectF(687, 28, 743, 39),
                    {QStringLiteral("Assets · 1 groups"),
                     QStringLiteral("Variants · component set / component / frame"),
                     QStringLiteral("States · default / hover / selected / disabled / custom")},
                    {QRectF(699, 40, 99, 15), QRectF(822, 40, 266, 15),
                     QRectF(1112, 40, 306, 15)});
            struct Spec {
                int x, y;
                const char* size;
                const char* state;
            };
            const Spec specs[] = {
                {28, 85, "Small", "Default"},   {728, 85, "Medium", "Default"},
                {1428, 85, "Large", "Default"}, {28, 335, "Small", "Hover"},
                {728, 335, "Medium", "Hover"},  {1428, 335, "Large", "Hover"},
                {28, 585, "Small", "Selected"}, {728, 585, "Medium", "Selected"},
                {1428, 585, "Large", "Selected"}, {28, 835, "Small", "Selected-hover"},
                {728, 835, "Medium", "Selected-hover"}, {1428, 835, "Large", "Selected-hover"},
                {28, 1085, "Small", "Disabled"}, {728, 1085, "Medium", "Disabled"},
                {1428, 1085, "Large", "Disabled"},
            };
            for (const auto& spec : specs) {
                const int h = tableSpecimenHeight(QString::fromLatin1(spec.size));
                paintTableSpecimen(p, QRect(spec.x, spec.y, 660, h), spec.size, spec.state);
            }
        };
    } else if (tree) {
        const TreeGalleryDef& def = treeGallery(pageId);
        canvas->setProperty("galleryNodeId", def.galleryId);
        canvas->paintContent = [canvas, def](QPainter& p) {
            drawKey(p, QRectF(427, 28, 743, 39),
                    {QStringLiteral("Assets · 1 groups"),
                     QStringLiteral("Variants · component set / component / frame"),
                     QStringLiteral("States · default / hover / selected / disabled / custom")},
                    {QRectF(439, 40, 99, 15), QRectF(562, 40, 266, 15),
                     QRectF(852, 40, 306, 15)});
            struct Spec {
                int x, y;
                const char* state;
                bool comfortable;
            };
            const Spec specs[] = {
                {28, 85, "Default", false},         {548, 85, "Hover", false},
                {1068, 85, "Selected", false},      {1068, 309, "Disabled", false},
                {28, 309, "Selected-hover", false}, {548, 309, "Focus-visible", false},
                {28, 563, "Default", true},         {548, 563, "Hover", true},
                {1068, 563, "Selected", true},      {1068, 851, "Disabled", true},
                {28, 851, "Selected-hover", true},  {548, 851, "Focus-visible", true},
            };
            for (const auto& spec : specs)
                paintTreeSpecimen(p, QRect(spec.x, spec.y, 500, spec.comfortable ? 258 : 194),
                                  def.contractLabel, def.primaries, def.secondaries,
                                  def.trailings, spec.state, spec.comfortable);
        };
    } else {
        const ListGalleryDef& def = listGallery(pageId);
        canvas->setProperty("galleryNodeId", def.galleryId);
        canvas->paintContent = [canvas, def](QPainter& p) {
            drawKey(p, QRectF(427, 28, 743, 39),
                    {QStringLiteral("Assets · 1 groups"),
                     QStringLiteral("Variants · component set / component / frame"),
                     QStringLiteral("States · default / hover / selected / disabled / custom")},
                    {QRectF(439, 40, 99, 15), QRectF(562, 40, 266, 15),
                     QRectF(852, 40, 306, 15)});
            struct Spec {
                int x, y;
                const char* state;
                bool comfortable;
            };
            const Spec specs[] = {
                {28, 85, "Default", false},         {548, 85, "Hover", false},
                {1068, 85, "Selected", false},      {1068, 309, "Disabled", false},
                {28, 309, "Selected-hover", false}, {548, 309, "Focus-visible", false},
                {28, 563, "Default", true},         {548, 563, "Hover", true},
                {1068, 563, "Selected", true},      {1068, 851, "Disabled", true},
                {28, 851, "Selected-hover", true},  {548, 851, "Focus-visible", true},
            };
            for (const auto& spec : specs)
                paintListSpecimen(p, QRect(spec.x, spec.y, 500, spec.comfortable ? 258 : 194),
                                  def.contractLabel, def.primaries, def.secondaries,
                                  def.trailings, spec.state, spec.comfortable);
        };
    }
    canvas->setProperty("variantSpecimenCount", table ? 15 : (undo ? 14 : 12));
    canvas->setProperty("componentSetCount", 1);
    return new FigmaGalleryView(canvas, parent);
}

} // namespace native_gallery
