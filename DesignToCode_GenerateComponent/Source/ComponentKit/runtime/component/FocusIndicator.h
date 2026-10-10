#pragma once
#include <QColor>
#include <QObject>

class QWidget;

namespace commonWidget {
// One input-modality policy for QSS, self-painted controls and composed fields.
class FocusIndicator final : public QObject {
    Q_OBJECT
  public:
    enum class Mode { Off, Keyboard, Always };
    Q_ENUM(Mode)
    static FocusIndicator& instance();
    static Mode parseMode(const QString& value);
    static QString modeName(Mode value);
    void configure(const QString& defaultMode, qreal opacity);
    void setMode(Mode value);
    Mode mode() const {
        return currentMode;
    }
    qreal opacity() const {
        return alpha;
    }
    QColor color(QColor source) const;
    bool visibleFor(const QWidget* widget) const;
    void watch(QWidget* widget);

  signals:
    void changed();

  protected:
    bool eventFilter(QObject*, QEvent*) override;

  private:
    explicit FocusIndicator(QObject* parent);
    Mode currentMode = Mode::Keyboard;
    bool keyboard = false;
    qreal alpha = 1;
};
} // namespace commonWidget
