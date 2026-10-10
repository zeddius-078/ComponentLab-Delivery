#pragma once
#include "Z_CheckBox.h"
#include <QPointer>
#include <QVector>

class CheckBoxGroupController : public QObject {
    Q_OBJECT
  public:
    CheckBoxGroupController(commonWidget::Z_CheckBox* summary,
                            const QVector<commonWidget::Z_CheckBox*>& items,
                            QObject* parent = nullptr);
    QStringList values() const;
    void synchronize();
  signals:
    void valuesChanged(const QStringList& values);

  protected:
    bool eventFilter(QObject*, QEvent*) override;

  private:
    QPointer<commonWidget::Z_CheckBox> summary;
    QVector<QPointer<commonWidget::Z_CheckBox>> items;
    bool updating = false;
};
