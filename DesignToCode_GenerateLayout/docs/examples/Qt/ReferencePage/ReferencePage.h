#pragma once
#include "DesignerWidgets.h"
#include <memory>

namespace Ui {
class ReferencePage;
}

// 批次参数表单。调用方提供数据、接收动作；页面保持组件组合与字段显示职责。
class ReferencePage final : public designer_widgets::ThemeHost {
    Q_OBJECT
public:
    struct Fields {
        QString lotNumber;
        QStringList chamberIds;
        bool qualityCheck = false;
    };
    explicit ReferencePage(const QString &profile, QWidget *parent = nullptr);
    ~ReferencePage() override;
    Fields fields() const;
    void setFields(const Fields &values);
    void setChambers(const QVector<commonWidget::Z_Select::Option> &options);
    void setLotError(const QString &message);
    void setBusy(bool busy);
signals:
    void submitRequested(const QString &lotNumber, const QStringList &chamberIds,
                         bool qualityCheck);
    void searchRequested(const QString &lotNumber);
    void fieldsChanged();

private:
    std::unique_ptr<Ui::ReferencePage> ui;
};
