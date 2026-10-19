// Button Contract implementation. Isolated delivery; not installed in CWS production.
#pragma once
#include <QPushButton>
#include <QColor>
#include <QFont>
#include <QIcon>
#include <QJsonObject>
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
class QPainter;
class QResizeEvent;

namespace commonWidget {
enum class ButtonIntent { Primary, Secondary, Operation, Danger };
enum class ButtonAppearance { Filled, Outlined, Dashed, Text, Link };
enum class ButtonType { Primary, Default, Dashed, Text, Link };
enum class ButtonSize { Small, Medium, Large, XS = Small, S = Small, M = Medium, L = Large };
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
struct Runtime {
    std::array<Metrics, 3> sizes;
    int cycleMs = 0;
    int frameMs = 0;
    qreal spinnerStartDegrees = 0;
    qreal spinnerSweepDegrees = 0;
    qreal spinnerTrackOpacity = 0;
    QJsonObject values;
    std::function<QPixmap(const QIcon&, int, const QColor&,
                          QIcon::Mode, qreal)> renderIcon;
};
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
    QString visualState() const;

public slots:
    void setIntent(ButtonIntent value);
    void setAppearance(ButtonAppearance value);
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

signals:
    void visualStateChanged(const QString& state);

protected:
    bool event(QEvent* event) override;
    bool eventFilter(QObject* watched, QEvent* event) override;
    void paintEvent(QPaintEvent* event) override;
    void resizeEvent(QResizeEvent* event) override;
    virtual QString stateStyleOverride(const QString& state) const;
    void refreshState();

private:
    struct Impl;
    std::unique_ptr<Impl> d;
    QString contentMode() const;
    void sync();
    void refreshVisual();
    void watchParents();
    void requireIconContent(const QIcon& leading, bool leadingVisible,
                            const QIcon& trailing, bool trailingVisible) const;
};
}
