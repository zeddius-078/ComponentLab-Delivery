#pragma once
#include "FieldChrome.h"
#include <QWidget>
#include <memory>
class QLineEdit;
class QPlainTextEdit;

namespace commonWidget {
class Z_InputField : public QWidget {
    Q_OBJECT
    Q_PROPERTY(QString value READ value WRITE setValue NOTIFY valueChanged)
    Q_PROPERTY(bool controlled READ isControlled WRITE setControlled)
  public:
    enum class Kind { Text, Password, Search, TextArea };
    Q_ENUM(Kind)
    explicit Z_InputField(std::shared_ptr<const field::Tokens>, QWidget* parent = nullptr);
    ~Z_InputField() override;
    QString value() const;
    bool isControlled() const;
    QLineEdit* lineEdit() const;
    QPlainTextEdit* textArea() const;
    field::Chrome* control() const;
    QSize sizeHint() const override;
    static int characterCount(const QString&);
  public slots:
    void setValue(const QString&);
    void setKind(Kind);
    void setControlSize(const QString&);
    void setVariant(const QString&);
    void setAppearance(const QString&);
    void setStatus(const QString&);
    void setLabel(const QString&);
    void setHint(const QString&);
    void setPlaceholder(const QString&);
    void setPrefix(const QString&);
    void setSuffix(const QString&);
    void setPrefixIconSource(const QString&);
    void setSuffixIconSource(const QString&);
    void setAllowClear(bool);
    void setShowCount(bool);
    void setMaximumLength(int);
    void setCountLimit(int);
    void setReadOnly(bool);
    void setControlled(bool);
    void setLoading(bool);
    void setAutoSize(bool enabled, int minRows = 0, int maxRows = 0);
  signals:
    void valueChanged(const QString&);
    void valueChangeRequested(const QString&);
    void submitted(const QString&);
    void searchRequested(const QString&, const QString& source);
    void cleared();

  protected:
    void changeEvent(QEvent*) override;
    bool eventFilter(QObject*, QEvent*) override;

  private:
    struct Impl;
    std::unique_ptr<Impl> d;
    void propose(const QString&);
    void sync();
};
} // namespace commonWidget
