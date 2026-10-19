#pragma once
#include "FieldChrome.h"
#include <QWidget>
#include <QStringList>
#include <QVector>
#include <memory>
class QListView;
class QLineEdit;

namespace commonWidget {
class Z_Select : public QWidget {
    Q_OBJECT
    Q_PROPERTY(QStringList values READ values WRITE setValues NOTIFY valuesChanged)
    Q_PROPERTY(bool open READ isOpen WRITE setOpen NOTIFY openChanged)
  public:
    enum class Mode { Single, Multiple, Tags };
    Q_ENUM(Mode)
    struct Option {
        QString value, label, group;
        bool disabled = false;
    };
    explicit Z_Select(std::shared_ptr<const field::Tokens>, QWidget* parent = nullptr);
    ~Z_Select() override;
    QStringList values() const;
    QStringList labels() const;
    bool isOpen() const;
    field::Chrome* control() const;
    QListView* optionView() const;
    QLineEdit* searchEdit() const;
    QWidget* popupWidget() const;
    QSize sizeHint() const override;
    quint64 beginOptionsRequest();
    bool completeOptionsRequest(quint64 id, const QVector<Option>&);
  public slots:
    void setOptions(const QVector<Option>&);
    void setValues(const QStringList&);
    void setMode(Mode);
    void setControlled(bool);
    void setControlSize(const QString&);
    void setAppearance(const QString&);
    void setStatus(const QString&);
    void setLabel(const QString&);
    void setHint(const QString&);
    void setPlaceholder(const QString&);
    void setPrefix(const QString&);
    void setPrefixIconSource(const QString&);
    void setSuffixIconSource(const QString&);
    void setAllowClear(bool);
    void setSearchEnabled(bool);
    void setReadOnly(bool);
    void setLoading(bool);
    void setMaxCount(int);
    void setMaxTagCount(int);
    void setOpen(bool);
    void setQuery(const QString&);
    void setTokenSeparators(const QString&);
  signals:
    void valuesChanged(const QStringList&);
    void valuesChangeRequested(const QStringList&);
    void searchRequested(const QString&);
    void openChanged(bool);
    void limitReached(int);

  protected:
    bool eventFilter(QObject*, QEvent*) override;
    void resizeEvent(QResizeEvent*) override;
    void changeEvent(QEvent*) override;

  private:
    struct Impl;
    std::unique_ptr<Impl> d;
    void sync();
    void syncTags();
    void propose(const QStringList&);
    void activateCurrent();
    void moveCurrent(int direction, bool end = false);
    void positionPopup();
    void syncOpenCondition();
};
} // namespace commonWidget
