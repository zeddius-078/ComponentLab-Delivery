#include "ReferencePage.h"
#include "Z_PushButton.h"
#include "ui_ReferencePage.h"
#include <QLineEdit>
#include <QSignalBlocker>

ReferencePage::ReferencePage(const QString &profile, QWidget *parent)
    : ThemeHost(profile, parent), ui(std::make_unique<Ui::ReferencePage>()) {
    ui->setupUi(this);
    // Qt 属性以外的组件初始配置使用有类型接口，结构与静态文字均保留在 .ui。
    ui->lotInput->setAllowClear(true);
    ui->submitButton->setType(commonWidget::ButtonType::Primary);
    ui->submitButton->setSize(commonWidget::ButtonSize::M);
    ui->searchButton->setType(commonWidget::ButtonType::Default);
    ui->searchButton->setSize(commonWidget::ButtonSize::M);
    ui->searchButton->setIconOnly(true);
    ui->chamberSelect->setMode(commonWidget::Z_Select::Mode::Single);

    // 页面只传递业务字段；调用方决定校验、提交和查询行为。
    connect(ui->submitButton, &QPushButton::clicked, this, [this] {
        const auto value = fields();
        emit submitRequested(value.lotNumber, value.chamberIds, value.qualityCheck);
    });
    connect(ui->searchButton, &QPushButton::clicked, this,
            [this] { emit searchRequested(ui->lotInput->value()); });
    connect(ui->lotInput, &commonWidget::Z_InputField::valueChanged, this,
            &ReferencePage::fieldsChanged);
    connect(ui->chamberSelect, &commonWidget::Z_Select::valuesChanged, this,
            &ReferencePage::fieldsChanged);
    connect(ui->qualityCheck, &QCheckBox::toggled, this, &ReferencePage::fieldsChanged);
}

ReferencePage::~ReferencePage() = default;

ReferencePage::Fields ReferencePage::fields() const {
    return {ui->lotInput->value(), ui->chamberSelect->values(), ui->qualityCheck->isChecked()};
}
void ReferencePage::setFields(const Fields &values) {
    // 整组输入作为一次更新交付，避免调用方收到尚未更新完的中间字段状态。
    const QSignalBlocker lot(ui->lotInput), chambers(ui->chamberSelect), quality(ui->qualityCheck);
    ui->lotInput->setValue(values.lotNumber);
    ui->chamberSelect->setValues(values.chamberIds);
    ui->qualityCheck->setChecked(values.qualityCheck);
    emit fieldsChanged();
}
void ReferencePage::setChambers(const QVector<commonWidget::Z_Select::Option> &options) {
    ui->chamberSelect->setOptions(options);
}
void ReferencePage::setLotError(const QString &message) {
    ui->lotInput->setStatus(message.isEmpty() ? QString() : QStringLiteral("error"));
    ui->lotInput->setHint(message);
    ui->lotInput->lineEdit()->setAccessibleDescription(message);
}
void ReferencePage::setBusy(bool busy) {
    ui->submitButton->setLoading(busy);
    ui->searchButton->setEnabled(!busy);
    ui->lotInput->setReadOnly(busy);
    ui->chamberSelect->setReadOnly(busy);
    ui->qualityCheck->setEnabled(!busy);
}
