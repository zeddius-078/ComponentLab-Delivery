#include "DemoWindow.h"
#include "ui_ComponentLab.h"
#include "Z_InputField.h"
#include "Z_Select.h"
#include "Z_OtpInput.h"
#include "Z_CheckBox.h"
#include <QComboBox>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPlainTextEdit>
#include <QScrollArea>
#include <QSpinBox>
#include <QSplitter>
#include <QTimer>
#include <QTreeWidget>
using commonWidget::Z_CheckBox;
using commonWidget::Z_InputField;
using commonWidget::Z_Select;

namespace {
QComboBox* combo(QFormLayout* form, const QString& label, const QStringList& values) {
    auto* w = new QComboBox;
    w->addItems(values);
    w->setSizeAdjustPolicy(QComboBox::AdjustToMinimumContentsLengthWithIcon);
    w->setMinimumContentsLength(8);
    form->addRow(label, w);
    return w;
}
Z_CheckBox* check(std::shared_ptr<const surface::Catalog> catalog, QFormLayout* form,
                  const QString& text, bool value = false) {
    auto* w = new Z_CheckBox(catalog, text);
    w->setChecked(value);
    form->addRow(w);
    return w;
}
QLabel* readout(QVBoxLayout* layout) {
    auto* w = new QLabel;
    w->setTextFormat(Qt::PlainText);
    w->setWordWrap(true);
    w->setTextInteractionFlags(Qt::TextSelectableByMouse);
    layout->addWidget(w);
    return w;
}
struct Page {
    QWidget* root;
    QFormLayout* controls;
    QVBoxLayout* examples;
    QTreeWidget* tree;
    QWidget* exampleRoot;
};
Page page() {
    auto* root = new QWidget;
    auto* outer = new QVBoxLayout(root);
    auto* split = new QSplitter;
    outer->addWidget(split);
    auto* settings = new QWidget;
    auto* form = new QFormLayout(settings);
    form->setVerticalSpacing(12);
    form->setRowWrapPolicy(QFormLayout::WrapLongRows);
    auto* settingsScroll = new QScrollArea;
    settingsScroll->setWidgetResizable(true);
    settingsScroll->setWidget(settings);
    settingsScroll->setFrameShape(QFrame::NoFrame);
    settingsScroll->setMinimumWidth(220);
    auto* examples = new QWidget;
    auto* layout = new QVBoxLayout(examples);
    layout->setSpacing(24);
    layout->setContentsMargins(24, 24, 24, 24);
    auto* examplesScroll = new QScrollArea;
    examplesScroll->setWidgetResizable(true);
    examplesScroll->setWidget(examples);
    examplesScroll->setFrameShape(QFrame::NoFrame);
    auto* tree = new QTreeWidget;
    tree->setHeaderLabels({QString::fromUtf8("节点 / 属性"), QString::fromUtf8("类型 / 值")});
    tree->setColumnWidth(0, 160);
    split->addWidget(settingsScroll);
    split->addWidget(examplesScroll);
    split->addWidget(tree);
    split->setSizes({250, 560, 330});
    return {root, form, layout, tree, examples};
}
template <class T>
void commonControls(Page& page, T* preview, std::shared_ptr<const surface::Catalog> catalog,
                    const char* appearanceName) {
    auto* size = combo(page.controls, QString::fromUtf8("尺寸"), {"sm", "md", "lg"});
    size->setCurrentText("md");
    auto* variant = combo(page.controls, QString::fromUtf8("形态"),
                          {"outlined", "filled", "borderless", "underlined"});
    auto* appearance = new QComboBox;
    appearance->setObjectName(appearanceName);
    for (const auto& id : catalog->presetIds())
        appearance->addItem(catalog->label(id), id);
    page.controls->addRow(QString::fromUtf8("材质"), appearance);
    auto* status =
        combo(page.controls, QString::fromUtf8("校验状态"), {"none", "error", "warning"});
    auto *readOnly = check(catalog, page.controls, "readOnly"),
         *disabled = check(catalog, page.controls, "disabled");
    QObject::connect(size, &QComboBox::currentTextChanged, preview, &T::setControlSize);
    QObject::connect(variant, &QComboBox::currentTextChanged, preview, &T::setVariant);
    QObject::connect(
        appearance, qOverload<int>(&QComboBox::currentIndexChanged), preview,
        [preview, appearance] { preview->setAppearance(appearance->currentData().toString()); });
    QObject::connect(status, &QComboBox::currentTextChanged, preview, [preview](const QString& s) {
        preview->setStatus(s == "none" ? QString() : s);
    });
    QObject::connect(readOnly, &QCheckBox::toggled, preview, &T::setReadOnly);
    QObject::connect(disabled, &QCheckBox::toggled, preview,
                     [preview](bool on) { preview->setEnabled(!on); });
}
QVector<Z_Select::Option> options() {
    return {
        {"etch-01", QString::fromUtf8("刻蚀腔体 ETCH-01"), QString::fromUtf8("刻蚀设备"), false},
        {"etch-02", QString::fromUtf8("刻蚀腔体 ETCH-02"), QString::fromUtf8("刻蚀设备"), false},
        {"etch-03", QString::fromUtf8("刻蚀腔体 ETCH-03 · 维护中"), QString::fromUtf8("刻蚀设备"),
         true},
        {"dep-01", QString::fromUtf8("沉积腔体 DEP-01"), QString::fromUtf8("沉积设备"), false},
        {"dep-02", QString::fromUtf8("沉积腔体 DEP-02"), QString::fromUtf8("沉积设备"), false}};
}
} // namespace
void DemoWindow::formPages() {
    auto tokens = std::make_shared<field::Tokens>(profile);
    auto input = page();
    inputTree = input.tree;
    auto* field = new Z_InputField(tokens);
    inputPreview = field;
    auto* kind = combo(input.controls, QString::fromUtf8("类型"),
                       {"Text", "Password", "Search", "TextArea"});
    connect(kind, qOverload<int>(&QComboBox::currentIndexChanged), field,
            [field](int index) { field->setKind(Z_InputField::Kind(index)); });
    commonControls(input, field, catalog, "inputAppearance");
    auto *clear = check(catalog, input.controls, "allowClear", true),
         *count = check(catalog, input.controls, "showCount", true);
    auto *prefix = check(catalog, input.controls, "prefix / suffix"),
         *loading = check(catalog, input.controls, "loading");
    auto *controlled = check(catalog, input.controls, "controlled"),
         *accept = check(catalog, input.controls, QString::fromUtf8("提交值变更请求"), true);
    auto* max = new QSpinBox;
    max->setRange(-1, 1000);
    max->setSpecialValueText(QString::fromUtf8("无限制"));
    max->setValue(24);
    input.controls->addRow("maxLength", max);
    field->setLabel(QString::fromUtf8("工艺配方名称"));
    field->setPlaceholder(QString::fromUtf8("输入配方名称"));
    field->setHint(QString::fromUtf8("所属批次：LOT-2026-0915"));
    field->setValue("ETCH_CF4_Standard");
    field->setAllowClear(true);
    field->setShowCount(true);
    field->setMaximumLength(24);
    input.examples->addWidget(field);
    auto* inputResult = readout(input.examples);
    auto describeInput = [field, inputResult] {
        inputResult->setText(QString("value = %1").arg(field->value()));
    };
    connect(clear, &QCheckBox::toggled, field, &Z_InputField::setAllowClear);
    connect(count, &QCheckBox::toggled, field, &Z_InputField::setShowCount);
    connect(prefix, &QCheckBox::toggled, field, [field](bool on) {
        field->setPrefix(on ? "Recipe /" : "");
        field->setSuffix(on ? ".json" : "");
    });
    connect(loading, &QCheckBox::toggled, field, &Z_InputField::setLoading);
    connect(controlled, &QCheckBox::toggled, field, &Z_InputField::setControlled);
    connect(max, qOverload<int>(&QSpinBox::valueChanged), field, &Z_InputField::setMaximumLength);
    connect(field, &Z_InputField::valueChangeRequested, this,
            [field, accept, inputResult](const QString& value) {
                if (field->isControlled() && accept->isChecked())
                    field->setValue(value);
                inputResult->setText(
                    QString("request = %1\ncommitted = %2").arg(value, field->value()));
            });
    connect(field, &Z_InputField::valueChanged, this, [describeInput] { describeInput(); });
    connect(field, &Z_InputField::searchRequested, this,
            [inputResult](const QString& text, const QString& source) {
                inputResult->setText(QString("search(%1) = %2").arg(source, text));
            });
    input.examples->addStretch();
    describeInput();
    ui->inputAppearanceLayout->addWidget(input.root);
    auto inputStates = page();
    inputStates.exampleRoot->setObjectName("INPUT_STATE_GROUP");
    extraInspections.append(qMakePair(inputStates.tree, inputStates.exampleRoot));
    auto* inputMaterial = combo(inputStates.controls, QString::fromUtf8("材质"), {});
    inputMaterial->setObjectName("inputStateAppearance");
    for (const auto& id : catalog->presetIds())
        inputMaterial->addItem(catalog->label(id), id);
    auto* groupEnabled =
        check(catalog, inputStates.controls, QString::fromUtf8("启用工艺复核"), true);
    auto* notes = new Z_InputField(tokens);
    notes->setKind(Z_InputField::Kind::TextArea);
    notes->setLabel(QString::fromUtf8("工艺备注"));
    notes->setPlaceholder(QString::fromUtf8("添加批次备注"));
    notes->setShowCount(true);
    notes->setCountLimit(80);
    notes->setAutoSize(true, 2, 5);
    notes->setValue(QString::fromUtf8("首片检测完成。\n等待工程师确认腔体状态。"));
    inputStates.examples->addWidget(notes);
    auto* password = new Z_InputField(tokens);
    password->setKind(Z_InputField::Kind::Password);
    password->setLabel(QString::fromUtf8("复核密码"));
    password->setPlaceholder(QString::fromUtf8("输入复核密码"));
    inputStates.examples->addWidget(password);
    auto* otpLabel = new QLabel(QString::fromUtf8("设备校验码"));
    inputStates.examples->addWidget(otpLabel);
    auto* otp = new commonWidget::Z_OtpInput(tokens);
    inputStates.examples->addWidget(otp);
    auto* otpResult = readout(inputStates.examples);
    connect(otp, &commonWidget::Z_OtpInput::completed, otpResult,
            [otpResult](const QString& value) { otpResult->setText("OTP = " + value); });
    QVector<Z_InputField*> inputSamples{notes, password};
    const QStringList inputLabels{"Error", "Warning", "Read only", "Disabled", "Search", "Loading"};
    for (int i = 0; i < inputLabels.size(); ++i) {
        auto* sample = new Z_InputField(tokens);
        sample->setObjectName("inputStateExample" + QString::number(i));
        sample->setLabel(inputLabels[i]);
        sample->setValue("ETCH_CF4_Standard");
        sample->setAllowClear(true);
        if (i == 0 || i == 1)
            sample->setStatus(i == 0 ? "error" : "warning");
        if (i == 2)
            sample->setReadOnly(true);
        if (i == 3)
            sample->setEnabled(false);
        if (i >= 4)
            sample->setKind(Z_InputField::Kind::Search);
        if (i == 5)
            sample->setLoading(true);
        inputStates.examples->addWidget(sample);
        inputSamples.append(sample);
    }
    connect(groupEnabled, &QCheckBox::toggled, inputStates.exampleRoot, &QWidget::setEnabled);
    connect(inputMaterial, qOverload<int>(&QComboBox::currentIndexChanged), this,
            [inputMaterial, inputSamples] {
                for (auto* sample : inputSamples)
                    sample->setAppearance(inputMaterial->currentData().toString());
            });
    inputStates.examples->addStretch();
    ui->inputStatesLayout->addWidget(inputStates.root);

    auto select = page();
    selectTree = select.tree;
    auto* selector = new Z_Select(tokens);
    selectPreview = selector;
    auto* mode = combo(select.controls, QString::fromUtf8("模式"), {"Single", "Multiple", "Tags"});
    connect(mode, qOverload<int>(&QComboBox::currentIndexChanged), selector,
            [selector](int index) { selector->setMode(Z_Select::Mode(index)); });
    commonControls(select, selector, catalog, "selectAppearance");
    auto *searchable = check(catalog, select.controls, "searchable", true),
         *selectClear = check(catalog, select.controls, "allowClear", true);
    auto *selectControlled = check(catalog, select.controls, "controlled"),
         *selectAccept = check(catalog, select.controls, QString::fromUtf8("提交值变更请求"), true);
    auto* countLimit = new QSpinBox;
    countLimit->setRange(-1, 10000);
    countLimit->setValue(3);
    countLimit->setSpecialValueText(QString::fromUtf8("无限制"));
    select.controls->addRow("maxCount", countLimit);
    auto* tags = new QSpinBox;
    tags->setRange(0, 10);
    tags->setValue(2);
    select.controls->addRow("maxTagCount", tags);
    auto* source = combo(select.controls, QString::fromUtf8("数据源"),
                         {QString::fromUtf8("分组设备"), QString::fromUtf8("10,000 条本地数据"),
                          QString::fromUtf8("本地异步模拟"), QString::fromUtf8("空数据")});
    selector->setLabel(QString::fromUtf8("目标腔体"));
    selector->setHint(QString::fromUtf8("维护中的腔体不可选择"));
    selector->setPlaceholder(QString::fromUtf8("选择设备"));
    selector->setOptions(options());
    selector->setValues({"etch-01"});
    selector->setMaxCount(3);
    select.examples->addWidget(selector);
    auto* selectResult = readout(select.examples);
    auto describeSelect = [selector, selectResult] {
        selectResult->setText(
            QString("value = [%1]\nlabel = [%2]")
                .arg(selector->values().join(", "), selector->labels().join(", ")));
    };
    connect(searchable, &QCheckBox::toggled, selector, &Z_Select::setSearchEnabled);
    connect(selectClear, &QCheckBox::toggled, selector, &Z_Select::setAllowClear);
    connect(selectControlled, &QCheckBox::toggled, selector, &Z_Select::setControlled);
    connect(countLimit, qOverload<int>(&QSpinBox::valueChanged), selector, &Z_Select::setMaxCount);
    connect(tags, qOverload<int>(&QSpinBox::valueChanged), selector, &Z_Select::setMaxTagCount);
    connect(selector, &Z_Select::valuesChanged, this, [describeSelect] { describeSelect(); });
    connect(selector, &Z_Select::valuesChangeRequested, this,
            [selector, selectControlled, selectAccept, selectResult](const QStringList& values) {
                if (selectControlled->isChecked() && selectAccept->isChecked())
                    selector->setValues(values);
                selectResult->setText(QString("request = [%1]\ncommitted = [%2]")
                                          .arg(values.join(", "), selector->values().join(", ")));
            });
    connect(selector, &Z_Select::limitReached, this,
            [selectResult](int max) { selectResult->setText(QString("maxCount = %1").arg(max)); });
    connect(
        source, qOverload<int>(&QComboBox::currentIndexChanged), selector, [selector](int index) {
            selector->beginOptionsRequest();
            selector->setLoading(false);
            if (index == 1) {
                QVector<Z_Select::Option> data;
                data.reserve(10000);
                for (int i = 0; i < 10000; ++i)
                    data.append(
                        {QString::number(i), QString("Process item %1").arg(i), {}, i % 17 == 0});
                selector->setOptions(data);
            } else
                selector->setOptions(index == 3 ? QVector<Z_Select::Option>() : options());
        });
    connect(selector, &Z_Select::searchRequested, this,
            [selector, source, selectResult](const QString& query) {
                if (source->currentIndex() != 2)
                    return;
                const auto id = selector->beginOptionsRequest();
                QTimer::singleShot(
                    query.size() % 2 ? 600 : 180, selector, [selector, id, query, selectResult] {
                        QVector<Z_Select::Option> found;
                        for (const auto& item : options())
                            if (item.label.contains(query, Qt::CaseInsensitive) ||
                                item.value.contains(query, Qt::CaseInsensitive))
                                found.append(item);
                        bool applied = selector->completeOptionsRequest(id, found);
                        selectResult->setText(
                            QString("local request #%1\n%2")
                                .arg(id)
                                .arg(applied ? "applied" : "stale response ignored"));
                    });
            });
    select.examples->addStretch();
    describeSelect();
    ui->selectAppearanceLayout->addWidget(select.root);
    auto selectStates = page();
    selectStates.exampleRoot->setObjectName("SELECT_STATE_GROUP");
    extraInspections.append(qMakePair(selectStates.tree, selectStates.exampleRoot));
    auto* selectMaterial = combo(selectStates.controls, QString::fromUtf8("材质"), {});
    selectMaterial->setObjectName("selectStateAppearance");
    for (const auto& id : catalog->presetIds())
        selectMaterial->addItem(catalog->label(id), id);
    auto* selectGroupEnabled =
        check(catalog, selectStates.controls, QString::fromUtf8("启用设备选择"), true);
    auto* multi = new Z_Select(tokens);
    multi->setMode(Z_Select::Mode::Multiple);
    multi->setOptions(options());
    multi->setValues({"etch-01", "etch-02", "dep-01"});
    multi->setLabel(QString::fromUtf8("参与对比的设备"));
    selectStates.examples->addWidget(multi);
    auto* tagExample = new Z_Select(tokens);
    tagExample->setMode(Z_Select::Mode::Tags);
    tagExample->setLabel(QString::fromUtf8("批次标签"));
    tagExample->setPlaceholder(QString::fromUtf8("添加标签"));
    tagExample->setValues({"Pilot", "QC"});
    selectStates.examples->addWidget(tagExample);
    auto* groupResult = readout(selectStates.examples);
    connect(multi, &Z_Select::valuesChanged, groupResult, [multi, groupResult] {
        groupResult->setText("values = [" + multi->values().join(", ") + "]");
    });
    QVector<Z_Select*> selectSamples{multi, tagExample};
    const QStringList selectLabels{"Error", "Warning", "Read only", "Disabled", "Empty", "Loading"};
    for (int i = 0; i < selectLabels.size(); ++i) {
        auto* sample = new Z_Select(tokens);
        sample->setObjectName("selectStateExample" + QString::number(i));
        sample->setLabel(selectLabels[i]);
        sample->setOptions(i == 4 ? QVector<Z_Select::Option>() : options());
        if (i != 4)
            sample->setValues({"etch-01"});
        if (i == 0 || i == 1)
            sample->setStatus(i == 0 ? "error" : "warning");
        if (i == 2)
            sample->setReadOnly(true);
        if (i == 3)
            sample->setEnabled(false);
        if (i == 5)
            sample->setLoading(true);
        selectStates.examples->addWidget(sample);
        selectSamples.append(sample);
    }
    connect(selectGroupEnabled, &QCheckBox::toggled, selectStates.exampleRoot,
            &QWidget::setEnabled);
    connect(selectMaterial, qOverload<int>(&QComboBox::currentIndexChanged), this,
            [selectMaterial, selectSamples] {
                for (auto* sample : selectSamples)
                    sample->setAppearance(selectMaterial->currentData().toString());
            });
    selectStates.examples->addStretch();
    ui->selectStatesLayout->addWidget(selectStates.root);
}
