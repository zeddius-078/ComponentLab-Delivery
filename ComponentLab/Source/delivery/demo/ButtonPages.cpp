#include "DemoWindow.h"
#include "ui_ComponentLab.h"
#include "IconSource.h"
#include "Z_CheckBox.h"
#include "Z_PushButton.h"
#include <QApplication>
#include <QButtonGroup>
#include <QComboBox>
#include <QFileDialog>
#include <QFormLayout>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QEvent>
#include <QLabel>
#include <QLineEdit>
#include <QPlainTextEdit>
#include <QRadioButton>
#include <QScopedValueRollback>
#include <QScrollArea>
#include <QSignalBlocker>
#include <QSplitter>
#include <QTreeWidget>
#include <QTabWidget>

using namespace commonWidget;
namespace {
struct ButtonControls {
    QComboBox *type, *size, *material, *state, *iconSide;
    QLineEdit *text, *name, *leading, *trailing;
    QRadioButton *textMode, *iconMode;
    Z_CheckBox *leadingVisible, *trailingVisible, *danger, *ghost, *loading, *disabled,
        *parentDisabled;
    bool applying = false;
    bool wasIconMode = false, textLeading = true, textTrailing = true;
};
QComboBox* choices(QFormLayout* form, const QString& label, const QString& name,
                   const QStringList& values) {
    auto* c = new QComboBox;
    c->setObjectName(name);
    c->addItems(values);
    form->addRow(label, c);
    return c;
}
QTreeWidget* makeTree() {
    auto* tree = new QTreeWidget;
    tree->setHeaderLabels({QString::fromUtf8("节点 / 属性"), QString::fromUtf8("类型 / 值")});
    tree->setColumnWidth(0, 170);
    return tree;
}
} // namespace
void DemoWindow::buttonPage() {
    auto* split = new QSplitter;
    ui->buttonLayout->addWidget(split);
    auto* settings = new QWidget;
    auto* form = new QFormLayout(settings);
    form->setRowWrapPolicy(QFormLayout::WrapLongRows);
    form->setVerticalSpacing(12);
    auto* scroll = new QScrollArea;
    scroll->setFrameShape(QFrame::NoFrame);
    scroll->setWidgetResizable(true);
    scroll->setWidget(settings);
    scroll->setMinimumWidth(250);
    auto c = std::make_shared<ButtonControls>();
    c->type = choices(form, "Type", "buttonType", {"Primary", "Default", "Dashed", "Text", "Link"});
    const auto check = [this, form](const QString& name, const QString& label, bool value = false) {
        auto* w = new Z_CheckBox(catalog, label);
        w->setObjectName(name);
        w->setChecked(value);
        form->addRow(w);
        return w;
    };
    c->danger = check("buttonDanger", "Danger");
    c->material = choices(form, QString::fromUtf8("V2 外观"), "buttonAppearance", {});
    for (const auto& id : QStringList{"filled", "outlined", "dashed", "text", "link"})
        c->material->addItem(id, id);
    c->material->setCurrentIndex(1);
    c->ghost = check("buttonGhost", "Ghost");
    c->size = choices(form, "Size", "buttonSize", {"XS", "S", "M", "L"});
    c->size->setCurrentIndex(2);
    auto* modes = new QWidget;
    auto* modesLayout = new QHBoxLayout(modes);
    modesLayout->setContentsMargins(0, 0, 0, 0);
    c->textMode = new QRadioButton("Text");
    c->textMode->setObjectName("buttonTextMode");
    c->iconMode = new QRadioButton("IconOnly");
    c->iconMode->setObjectName("buttonIconMode");
    c->textMode->setChecked(true);
    modesLayout->addWidget(c->textMode);
    modesLayout->addWidget(c->iconMode);
    form->addRow(QString::fromUtf8("内容模式"), modes);
    c->iconSide =
        choices(form, QString::fromUtf8("独立图标来源"), "buttonIconSide", {"Leading", "Trailing"});
    c->text = new QLineEdit("Button");
    c->text->setObjectName("buttonText");
    c->name = new QLineEdit(c->text->text());
    c->name->setObjectName("buttonAccessibleName");
    form->addRow(QString::fromUtf8("文本"), c->text);
    form->addRow("Accessible name", c->name);
    const auto source = [form](const QString& prefix, const QString& initial) {
        auto* row = new QWidget;
        auto* layout = new QHBoxLayout(row);
        layout->setContentsMargins(0, 0, 0, 0);
        auto* input = new QLineEdit(initial);
        input->setObjectName(prefix + "Source");
        input->setToolTip("QRC / SVG / PNG");
        auto* browse = new Z_PushButton;
        browse->setType(ButtonType::Text);
        browse->setText(QString::fromUtf8("选择文件"));
        browse->setSize(ButtonSize::XS);
        layout->addWidget(input, 1);
        layout->addWidget(browse);
        form->addRow(prefix == "leading" ? "LeadingIcon" : "TrailingIcon", row);
        QObject::connect(browse, &QPushButton::clicked, input, [input] {
            const auto path = QFileDialog::getOpenFileName(input, QString::fromUtf8("选择图标"), {},
                                                           "Icons (*.svg *.png)");
            if (!path.isEmpty()) {
                input->setText(path);
                emit input->editingFinished();
            }
        });
        return input;
    };
    c->leadingVisible = check("leadingVisible", "Leading visible", true);
    c->leading = source("leading", ":/icons/arrow-left.svg");
    c->trailingVisible = check("trailingVisible", "Trailing visible", true);
    c->trailing = source("trailing", ":/icons/arrow-right.svg");
    c->disabled = check("buttonDisabled", "Disabled");
    c->parentDisabled = check("buttonParentDisabled", "Parent disabled");
    c->loading = check("buttonLoading", "Loading");
    c->state = choices(form, QString::fromUtf8("状态预览"), "buttonState",
                       {"Live", "Default", "Hover", "Pressed", "Focus"});
    auto* applyButton = new Z_PushButton;
    applyButton->setText(QString::fromUtf8("应用资源"));
    applyButton->setObjectName("buttonApply");
    form->addRow(applyButton);
    auto* error = new QLabel;
    error->setObjectName("buttonValidation");
    error->setWordWrap(true);
    form->addRow(error);
    auto* area = new QWidget;
    auto* layout = new QVBoxLayout(area);
    auto* status = new QLabel;
    status->setObjectName("buttonConfigurationStatus");
    status->setWordWrap(true);
    layout->addWidget(status);
    auto* host = new QWidget;
    host->setObjectName("previewHost");
    host->setMinimumHeight(180);
    auto* previewLayout = new QVBoxLayout(host);
    previewLayout->setContentsMargins(32, 32, 32, 32);
    buttonPreview = new Z_PushButton(host);
    buttonPreview->setObjectName("BUTTON_ROOT");
    previewLayout->addWidget(buttonPreview, 0, Qt::AlignCenter);
    layout->addWidget(host);
    auto* events = new QPlainTextEdit;
    events->setReadOnly(true);
    events->setMaximumBlockCount(60);
    events->setMaximumHeight(160);
    events->setObjectName("buttonEvents");
    layout->addWidget(events);
    layout->addStretch();
    const auto apply = [this, c, host, error, status, applyButton] {
        if (c->applying)
            return;
        QScopedValueRollback<bool> guard(c->applying, true);
        auto* b = buttonPreview;
        QStringList changes;
        const auto setCheck = [](Z_CheckBox* w, bool value) {
            QSignalBlocker block(w);
            w->setChecked(value);
        };
        const auto appearanceId = c->material->currentData().toString();
        const bool iconOnly = c->iconMode->isChecked();
        if (iconOnly != c->wasIconMode) {
            if (iconOnly) {
                c->textLeading = c->leadingVisible->isChecked();
                c->textTrailing = c->trailingVisible->isChecked();
                if (!c->textLeading && c->textTrailing) {
                    QSignalBlocker block(c->iconSide);
                    c->iconSide->setCurrentIndex(1);
                }
            } else {
                setCheck(c->leadingVisible, c->textLeading);
                setCheck(c->trailingVisible, c->textTrailing);
            }
            c->wasIconMode = iconOnly;
        }
        if (iconOnly) {
            setCheck(c->leadingVisible, c->iconSide->currentIndex() == 0);
            setCheck(c->trailingVisible, c->iconSide->currentIndex() == 1);
        }
        const bool left = c->leadingVisible->isChecked(), right = c->trailingVisible->isChecked();
        const bool loading = c->loading->isChecked();
        const bool policyDisabled = c->disabled->isChecked() || c->parentDisabled->isChecked();
        if ((loading || policyDisabled) && c->state->currentIndex() != 0) {
            QSignalBlocker block(c->state);
            c->state->setCurrentIndex(0);
            changes << QString::fromUtf8("已退出状态预览：当前由 Loading / Disabled 控制");
        }
        bool valid = false;
        try {
            // Validate the whole requested configuration before mutating the preview.
            const auto leading = left ? IconSource::load(c->leading->text()) : QIcon(),
                       trailing = right ? IconSource::load(c->trailing->text()) : QIcon();
            if ((left && leading.isNull()) || (right && trailing.isNull()))
                throw std::invalid_argument("Selected icon slot requires a valid resource");
            if (iconOnly && leading.isNull() && trailing.isNull())
                throw std::invalid_argument("IconOnly requires one visible icon");
            if (!iconOnly && c->text->text().trimmed().isEmpty())
                throw std::invalid_argument("Text mode requires non-empty text");
            if (iconOnly && c->name->text().trimmed().isEmpty())
                throw std::invalid_argument("Icon-only content needs accessibleName");
            if (b->property("loading").toBool() && !loading)
                b->setLoading(false);
            if (!b->property("loading").toBool()) {
                if (b->isIconOnly())
                    b->setIconOnly(false);
                b->setType(ButtonType(c->type->currentIndex()));
                const QVector<ButtonSize> sizes{ButtonSize::XS, ButtonSize::S,
                                                ButtonSize::M, ButtonSize::L};
                b->setSize(sizes.at(c->size->currentIndex()));
                if (c->danger->isChecked())
                    b->setDanger(true);
                const QMap<QString, ButtonAppearance> appearances{
                    {"filled", ButtonAppearance::Filled},
                    {"outlined", ButtonAppearance::Outlined},
                    {"dashed", ButtonAppearance::Dashed},
                    {"text", ButtonAppearance::Text},
                    {"link", ButtonAppearance::Link}};
                b->setAppearance(appearances.value(appearanceId));
                if (c->ghost->isChecked())
                    b->setGhost(true);
                b->setText(c->text->text());
                b->setAccessibleName(c->name->text());
                b->setToolTip(c->name->text());
                // Keep both authored source slots on the runtime instance
                // even when a slot is currently hidden.  Visibility is an
                // independent content axis; dropping the hidden source made
                // later icon-mode round trips lose the Figma asset identity.
                b->setLeadingIconSource(c->leading->text());
                b->setTrailingIconSource(c->trailing->text());
                b->setLeadingIconVisible(left);
                b->setTrailingIconVisible(right);
                b->setLeadingIconStyle(IconContainerStyle::Bare);
                b->setTrailingIconStyle(IconContainerStyle::Bare);
                b->setIconOnly(iconOnly);
            }
            host->setEnabled(!c->parentDisabled->isChecked());
            b->setEnabled(!c->disabled->isChecked());
            b->setLoading(loading);
            error->clear();
            status->setText(changes.join('\n'));
            valid = true;
        } catch (const std::exception& e) {
            b->setLoading(false);
            b->setEnabled(false);
            const auto message =
                QString::fromUtf8("配置无效，预览已禁用：") + QString::fromUtf8(e.what());
            error->setText(message);
            status->setText(message);
        }
        b->setProperty("configurationValid", valid);
        for (auto* w : QList<QWidget*>{c->type, c->danger, c->material, c->size, c->textMode,
                                       c->iconMode, c->name})
            w->setEnabled(!loading);
        c->ghost->setEnabled(!loading);
        c->ghost->setToolTip(QString::fromUtf8("Ghost 使用 V2 text 外观"));
        c->iconSide->setEnabled(iconOnly && !loading);
        c->text->setEnabled(!iconOnly && !loading);
        c->leadingVisible->setEnabled(!iconOnly && !loading);
        c->trailingVisible->setEnabled(!iconOnly && !loading);
        c->leading->parentWidget()->setEnabled(left && !loading);
        c->trailing->parentWidget()->setEnabled(right && !loading);
        c->state->setEnabled(valid && !loading && !policyDisabled);
        c->loading->setEnabled(loading || (valid && !policyDisabled));
        applyButton->setEnabled(valid && !loading);
        {
            b->setDown(false);
            b->clearFocus();
            b->setAttribute(Qt::WA_UnderMouse, false);
            QEvent leave(QEvent::Leave);
            QApplication::sendEvent(b, &leave);
            if (valid && !loading && !policyDisabled &&
                (c->state->currentIndex() == 2 || c->state->currentIndex() == 3)) {
                b->setAttribute(Qt::WA_UnderMouse, true);
                b->setDown(c->state->currentIndex() == 3);
                QEvent enter(QEvent::Enter);
                QApplication::sendEvent(b, &enter);
            }
            if (valid && !loading && !policyDisabled && c->state->currentIndex() == 4)
                b->setFocus(Qt::TabFocusReason);
        }
        const QStringList previewStates{"","default","hover","active","focus-visible"};
        b->setProperty("figmaPreviewState",valid && !loading && !policyDisabled?
                       previewStates.value(c->state->currentIndex()):QString());
    };
    for (auto* w : {c->type, c->size, c->material, c->state, c->iconSide})
        connect(w, qOverload<int>(&QComboBox::currentIndexChanged), this, [apply] { apply(); });
    for (auto* w : {c->leadingVisible, c->trailingVisible, c->danger, c->ghost, c->loading,
                    c->disabled, c->parentDisabled})
        connect(w, &QCheckBox::toggled, this, [apply] { apply(); });
    for (auto* w : {c->textMode, c->iconMode})
        connect(w, &QRadioButton::toggled, this, [apply](bool on) {
            if (on)
                apply();
        });
    for (auto* w : {c->leading, c->trailing, c->text, c->name}) {
        connect(w, &QLineEdit::editingFinished, this, [apply] { apply(); });
        connect(w, &QLineEdit::textEdited, this, [apply] { apply(); });
    }
    connect(applyButton, &QPushButton::clicked, this, [apply] { apply(); });
    connect(buttonPreview, &QPushButton::clicked, events,
            [events] { events->appendPlainText("clicked"); });
    buttonTree = makeTree();
    split->addWidget(scroll);
    split->addWidget(area);
    split->addWidget(buttonTree);
    split->setSizes({330, 500, 330});
    apply();
}
void DemoWindow::buttonStatesPage() {
    auto* scroll = new QScrollArea;
    scroll->setWidgetResizable(true);
    scroll->setFrameShape(QFrame::NoFrame);
    ui->buttonStatesLayout->addWidget(scroll);
    auto* body = new QWidget;
    scroll->setWidget(body);
    auto* outer = new QVBoxLayout(body);
    auto* appearance = new QComboBox;
    appearance->setObjectName("buttonStateAppearance");
    for (const auto& id : QStringList{"filled", "outlined", "dashed", "text", "link"})
        appearance->addItem(id, id);
    outer->addWidget(appearance, 0, Qt::AlignLeft);
    auto* grid = new QGridLayout;
    grid->setHorizontalSpacing(28);
    grid->setVerticalSpacing(18);
    outer->addLayout(grid);
    const QStringList types{"Primary", "Default", "Dashed", "Text", "Link"},
        sizes{"XS", "S", "M", "L"};
    QVector<Z_PushButton*> buttons;
    for (int col = 0; col < 4; ++col)
        grid->addWidget(new QLabel(sizes[col]), 0, col + 1);
    for (int row = 0; row < 5; ++row) {
        grid->addWidget(new QLabel(types[row]), row + 1, 0);
        for (int col = 0; col < 4; ++col) {
            auto* b = new Z_PushButton;
            b->setText(QString::fromUtf8("执行任务"));
            b->setType(ButtonType(row));
            const QVector<ButtonSize> buttonSizes{ButtonSize::XS, ButtonSize::S,
                                                  ButtonSize::M, ButtonSize::L};
            b->setSize(buttonSizes.at(col));
            buttons << b;
            grid->addWidget(b, row + 1, col + 1, Qt::AlignLeft);
        }
    }
    auto* states = new QGridLayout;
    states->setVerticalSpacing(16);
    states->setHorizontalSpacing(28);
    outer->addLayout(states);
    const QStringList labels{"Default",  "Disabled",         "Loading",       "Danger",
                             "Ghost",    "Leading",          "Trailing",      "Dual",
                             "IconOnly", "IconOnly Loading", "Danger + Dual", "Parent disabled"};
    for (int i = 0; i < labels.size(); ++i) {
        const int row = (i / 4) * 2, col = i % 4;
        states->addWidget(new QLabel(labels[i]), row, col);
        auto* host = new QWidget;
        auto* layout = new QHBoxLayout(host);
        auto* b = new Z_PushButton(host);
        b->setText(QString::fromUtf8("执行任务"));
        b->setType(ButtonType::Primary);
        b->setAccessibleName(labels[i]);
        b->setObjectName("buttonStateExample" + QString::number(i));
        if (i == 1)
            b->setEnabled(false);
        b->setProperty("sampleDisabled", i == 1);
        if (i == 3)
            b->setDanger(true);
        if (i == 4) {
            b->setGhost(true);
            host->setObjectName("ghostSampleHost");
        }
        if (i == 5 || i == 7 || i >= 8)
            b->setLeadingIconSource(":/icons/arrow-left.svg");
        if (i == 6 || i == 7 || i == 10)
            b->setTrailingIconSource(":/icons/arrow-right.svg");
        if (i == 8 || i == 9)
            b->setIconOnly(true);
        if (i == 10) {
            b->setDanger(true);
        }
        if (i == 2 || i == 9)
            b->setLoading(true);
        if (i == 11)
            host->setEnabled(false);
        buttons << b;
        layout->addWidget(b, 0, Qt::AlignLeft);
        states->addWidget(host, row + 1, col);
    }
    connect(appearance, qOverload<int>(&QComboBox::currentIndexChanged), this,
            [appearance, buttons] {
                const QMap<QString, ButtonAppearance> values{
                    {"filled", ButtonAppearance::Filled},
                    {"outlined", ButtonAppearance::Outlined},
                    {"dashed", ButtonAppearance::Dashed},
                    {"text", ButtonAppearance::Text},
                    {"link", ButtonAppearance::Link}};
                const auto id = appearance->currentData().toString();
                for (auto* b : buttons) {
                    b->setAppearance(values.value(id));
                    b->setEnabled(!b->property("sampleDisabled").toBool());
                }
            });
    outer->addStretch();
}
