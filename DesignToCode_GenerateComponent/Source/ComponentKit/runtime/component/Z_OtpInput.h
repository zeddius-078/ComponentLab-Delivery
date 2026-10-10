#pragma once
#include "FieldChrome.h"
#include <QStringList>
#include <QVector>
#include <QWidget>
class QLineEdit;
namespace commonWidget {
// Composition example: fixed digit count; the same field tokens style every cell.
class Z_OtpInput : public QWidget {
    Q_OBJECT
    Q_PROPERTY(QString value READ value WRITE setValue NOTIFY valueChanged)
  public:
    explicit Z_OtpInput(std::shared_ptr<const field::Tokens>, int length = 6,
                        QWidget* parent = nullptr);
    ~Z_OtpInput() override;
    QString value() const;
    QList<QLineEdit*> editors() const;
  public slots:
    void setValue(const QString&);
    void setMasked(bool);
    void setReadOnly(bool);
    void setControlSize(const QString&);
    void setStatus(const QString&);
    void setVariant(const QString&);
    void setAppearance(const QString&);
  signals:
    void valueChanged(const QString&);
    void completed(const QString&);

  protected:
    bool eventFilter(QObject*, QEvent*) override;

  private:
    std::shared_ptr<const field::Tokens> tokens;
    QList<QLineEdit*> cells;
    QVector<field::Chrome*> frames;
    QStringList values;
    void edit(int index, const QString&);
    void commit(const QStringList&);
};
} // namespace commonWidget
