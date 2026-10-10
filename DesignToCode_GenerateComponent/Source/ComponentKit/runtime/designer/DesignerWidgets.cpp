#include "DesignerWidgets.h"
#include "ComponentTheme.h"
#include <QLineEdit>
#include <QListView>
#include <QVBoxLayout>
#include <stdexcept>

namespace designer_widgets {
namespace {
const ThemeHost &themeFor(QWidget *parent) {
    // uic 会经过滚动内容和布局容器创建控件；沿父链查找主题，避免复制 Token 对象。
    for (auto *widget = parent; widget; widget = widget->parentWidget()) {
        if (const auto *host = dynamic_cast<const ThemeHost *>(widget))
            return *host;
    }
    throw std::logic_error("Designer component requires a ThemeHost ancestor");
}
} // namespace

ThemeHost::ThemeHost(const QString &profile, QWidget *parent) : QWidget(parent) {
    // Button 的运行配置是进程级；阻止同一页面中混入另一组字段和 Checkbox 参数。
    if (component_theme::activeProfile() != profile)
        throw std::logic_error("Install the matching component theme before creating the page");
    catalog = std::make_shared<surface::Catalog>(profile);
    tokens = std::make_shared<field::Tokens>(profile);
}
CheckBox::CheckBox(QWidget *parent)
    : commonWidget::Z_CheckBox(themeFor(parent).surfaces(), {}, parent) {}
InputField::InputField(QWidget *parent)
    : commonWidget::Z_InputField(themeFor(parent).fields(), parent) {}
QString InputField::placeholderText() const { return lineEdit()->placeholderText(); }
void InputField::setPlaceholderText(const QString &text) { setPlaceholder(text); }
Select::Select(QWidget *parent) : commonWidget::Z_Select(themeFor(parent).fields(), parent) {
    // 原组件将焦点交给内部显示框，键盘展开与选择依赖该目标，适配层保持它不变。
}

SelectOption::SelectOption(QWidget *parent) : SelectOption(themeFor(parent).fields(), parent) {}
SelectOption::SelectOption(std::shared_ptr<const field::Tokens> value, QWidget *parent)
    : QWidget(parent), tokens(std::move(value)),
      source(std::make_unique<commonWidget::Z_Select>(tokens, this)) {
    // 此组合适配归平台维护，复用现有选项绘制、Token 和模型，不复制第二套视觉常量。
    source->hide();
    source->setMode(commonWidget::Z_Select::Mode::Single);
    view = source->optionView();
    const auto optionFont = source->popupWidget()->font();
    view->setParent(this);
    view->removeEventFilter(source.get());
    view->setFrameShape(QFrame::NoFrame);
    view->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    view->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    view->setFocusPolicy(Qt::NoFocus);
    view->setFont(optionFont);
    auto *box = new QVBoxLayout(this);
    const int inset = tokens->metric("popupPadding");
    box->setContentsMargins(inset, 0, inset, 0);
    box->setSpacing(0);
    box->addWidget(view);
    setControlSize(sizeName);
    setText({});
    view->show();
}
SelectOption::~SelectOption() = default;
void SelectOption::setText(const QString &text) {
    label = text;
    source->setOptions({{QStringLiteral("option"), label, {}, true}});
    setAccessibleName(label);
    updateGeometry();
}
void SelectOption::setControlSize(const QString &size) {
    // Tokens::size 拒绝不存在的等级，不静默选择另一尺寸。
    tokens->size(size);
    source->setControlSize(size);
    sizeName = size;
    view->setStyleSheet(source->styleSheet());
    updateGeometry();
}
QSize SelectOption::sizeHint() const {
    return QSize(view->sizeHint().width() + 2 * tokens->metric("popupPadding"),
                 tokens->metric("optionHeight"));
}
} // namespace designer_widgets
