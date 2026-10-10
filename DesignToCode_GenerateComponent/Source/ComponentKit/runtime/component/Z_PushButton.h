// Button Contract implementation. Isolated delivery; not installed in CWS production.
#pragma once
#include <QPushButton>
#include <QColor>
#include <QFont>
#include <QIcon>
#include <QMap>
#include <QPixmap>
#include <QString>
#include <QVariant>
#include <QVector>
#include <array>
#include <functional>
#include <memory>

class QApplication;
class QEvent;
class QPaintEvent;
class QResizeEvent;

namespace commonWidget {
enum class ButtonType { Primary, Default, Dashed, Text, Link };
enum class ButtonSize { XS, S, M, L };
enum class IconContainerStyle { Filled, Outlined, Bare };

namespace button_detail {
struct Metrics {
    int minimumHeight = 0;
    int paddingInline = 0;
    int paddingBlock = 0;
    int gap = 0;
    int borderWidth = 0;
    int iconCanvas = 0;
    int iconContainer = 0;
    int indicatorSize = 0;
    qreal indicatorStroke = 0;
    int iconOnlySize = 0;
};
struct LegacyRecipe {
    ButtonType type;
    bool danger;
    bool ghost;
    IconContainerStyle iconStyle;
};
struct Runtime {
    std::array<Metrics, 4> sizes;
    int cycleMs = 0;
    int frameMs = 0;
    int sweepDegrees = 0;
    QColor focusColor, dangerFocusColor, ghostFocusColor;
    QString qssTemplate;
    QString generatedQss;
    QVector<QMap<QString, QString>> qssRows;
    std::function<QPixmap(const QIcon&, int, const QColor&,
                          QIcon::Mode, qreal)> renderIcon;
    std::function<QIcon(int)> legacyOperationIcon;
    std::function<LegacyRecipe(const QString&)> legacyRecipe;
    std::function<ButtonSize(const QFont&)> legacyFontSize;
};
QString renderButtonQss(const QString& source,
                       const QVector<QMap<QString, QString>>& rows);
void installButtonRuntime(QApplication& app, const Runtime& runtime);
}

class Z_PushButton : public QPushButton {
    Q_OBJECT
    Q_PROPERTY(bool enabled READ isEnabled WRITE setEnabled)
    Q_PROPERTY(QString text READ text WRITE setText)
    Q_PROPERTY(QString visualState READ visualState)
    Q_PROPERTY(bool iconOnly READ isIconOnly WRITE setIconOnly)
    Q_PROPERTY(QString leadingIconSource READ leadingIconSource WRITE setLeadingIconSource)
    Q_PROPERTY(QString trailingIconSource READ trailingIconSource WRITE setTrailingIconSource)
public:
    explicit Z_PushButton(QWidget* parent = nullptr);
    ~Z_PushButton() override;
    QSize sizeHint() const override;
    QSize minimumSizeHint() const override;
    bool isIconOnly() const;
    QString leadingIconSource() const;
    QString trailingIconSource() const;
    QColor focusIndicatorColor() const;

public slots:
    void setType(ButtonType value);
    void setDanger(bool value);
    void setGhost(bool value);
    void setSize(ButtonSize value);
    void setLoading(bool value);
    void setEnabled(bool value);
    void setText(const QString& value);
    void setLeadingIcon(const QIcon& value);
    void setTrailingIcon(const QIcon& value);
    void setLeadingIconVisible(bool value);
    void setTrailingIconVisible(bool value);
    void setLeadingIconStyle(IconContainerStyle value);
    void setTrailingIconStyle(IconContainerStyle value);
    void setIconOnly(bool value);
    void setLeadingIconSource(const QString& source);
    void setTrailingIconSource(const QString& source);

public:
    // Compatibility only. New callers use the explicit setters above.
    void SetPrimaryButton();
    void SetSecondaryButton();
    void SetDangerousButton();
    void SetOperateIco(int icoIndex = 0);
    void SetIcoButton();
    void SetDarkButton(const QIcon& icon);
    void SetIcoRightButton(const QString& text, const QIcon& icon);
    void SetFont(const QFont& font);

protected:
    bool event(QEvent* event) override;
    bool eventFilter(QObject* watched, QEvent* event) override;
    void paintEvent(QPaintEvent* event) override;
    void resizeEvent(QResizeEvent* event) override;

private:
    enum class ButtonVariant { Solid, Outlined, Dashed, Text, Link };
    enum class ButtonColor { Primary, Neutral, Danger };
    struct Recipe { ButtonColor color; ButtonVariant variant; };
    struct Impl;
    std::unique_ptr<Impl> d;
    Recipe resolveRecipe() const;
    QString visualState() const;
    void sync();
    void refreshVisual();
    void watchParents();
    void applyLegacy(const QString& name);
    void requireIconContent(const QIcon& leading, bool leadingVisible,
                            const QIcon& trailing, bool trailingVisible) const;
};
}
