#include "LabelAppearance.h"
#include <QFontDatabase>
#include <QEvent>
#include <QLabel>
#include <QVariant>
#include <stdexcept>

namespace commonWidget {
namespace {
void resolveLogicalAlignment(QLabel* label) {
    const auto alignment = label->property("figmaLogicalAlignment").toString();
    const bool right = (alignment == "Start") == (label->layoutDirection() == Qt::RightToLeft);
    // QLabel otherwise resolves Left/Right against the text's own direction.
    // Resolve the Figma logical axis against the UI direction once, then keep
    // that physical result absolute so mixed-language text cannot flip it again.
    label->setAlignment(Qt::AlignVCenter | Qt::AlignAbsolute |
                        (alignment == "Center" ? Qt::AlignHCenter
                         : right               ? Qt::AlignRight
                                               : Qt::AlignLeft));
}

class LabelDirectionBinding final : public QObject {
public:
    explicit LabelDirectionBinding(QLabel* label) : QObject(label) {
        setObjectName("figmaLabelDirectionBinding");
        label->installEventFilter(this);
    }

protected:
    bool eventFilter(QObject* object, QEvent* event) override {
        if (event->type() == QEvent::LayoutDirectionChange)
            resolveLogicalAlignment(static_cast<QLabel*>(object));
        return false;
    }
};
} // namespace

void applyFigmaLabelAppearance(QLabel* label, const QString& alignment, bool wrap) {
    if (!label || !QStringList{"Start", "Center", "End"}.contains(alignment))
        throw std::invalid_argument("Label requires Start, Center or End logical alignment");
    static const bool fonts = [] {
        QFontDatabase::addApplicationFont(":/fonts/HarmonyOS_Sans_Regular.ttf");
        QFontDatabase::addApplicationFont(":/fonts/HarmonyOS_Sans_SC_Regular.ttf");
        return true;
    }();
    Q_UNUSED(fonts);
    QFont font;
    font.setFamilies({"HarmonyOS Sans", "HarmonyOS Sans SC"});
    font.setPixelSize(14);
    font.setWeight(QFont::Normal);
    label->setFont(font);
    label->setProperty("figmaAppearance", "figma-light");
    label->setProperty("figmaLogicalAlignment", alignment);
    if (!label->findChild<QObject*>("figmaLabelDirectionBinding", Qt::FindDirectChildrenOnly))
        new LabelDirectionBinding(label);
    resolveLogicalAlignment(label);
    label->setWordWrap(wrap);
    label->setMargin(0);
    label->setIndent(0);
    label->setMinimumHeight(wrap ? 72 : 32);
    label->setStyleSheet("QLabel {background:transparent;border:0;padding:0;color:#191919;"
                         "font-family:'HarmonyOS Sans','HarmonyOS Sans SC';font-size:14px;font-weight:400;}"
                         "QLabel:disabled {color:#8a8a8a;}");
}

void configureFigmaLabel(QLabel* label, const QMap<QString, QString>& properties) {
    if (!label)
        throw std::invalid_argument("Label target is null");
    const QString alignment = properties.value(QStringLiteral("Alignment"));
    const QString wrap = properties.value(QStringLiteral("Wrap"));
    const QString state = properties.value(QStringLiteral("State"));
    if (!QStringList{QStringLiteral("True"), QStringLiteral("False")}.contains(wrap))
        throw std::invalid_argument("Label Wrap requires True or False");
    if (!QStringList{QStringLiteral("Default"), QStringLiteral("Disabled")}.contains(state))
        throw std::invalid_argument("Label State requires Default or Disabled");
    applyFigmaLabelAppearance(label, alignment, wrap == QStringLiteral("True"));
    label->setEnabled(state != QStringLiteral("Disabled"));
    label->setFocusPolicy(Qt::NoFocus);
    label->setProperty("figmaLabelState", state);
}
}
