#include "WidgetRegistry.h"
#include "ComponentFactory.h"
#include "Z_Select.h"

#include <QListView>
#include <QVBoxLayout>
#include <stdexcept>

namespace v2 {
namespace {

// HTML + DSL 路线只保留布局侧特有的单选项适配。标准组件由 SDK 工厂统一创建。
class SelectOptionAdapter final : public QWidget {
public:
    SelectOptionAdapter(const QJsonObject& config, QWidget* parent) : QWidget(parent) {
        const auto tokens = std::make_shared<field::Tokens>(config["profile"].toString());
        source = new commonWidget::Z_Select(tokens, this);
        source->hide();
        source->setMode(commonWidget::Z_Select::Mode::Single);
        source->setControlSize(config["controlSize"].toString());
        source->setOptions({{config["nodeId"].toString(), config["label"].toString(), {}, true}});
        auto* view = source->optionView();
        const auto font = source->popupWidget()->font();
        view->setParent(this);
        view->removeEventFilter(source);
        view->setStyleSheet(source->styleSheet());
        view->setFont(font);
        view->setFrameShape(QFrame::NoFrame);
        view->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        view->setFocusPolicy(Qt::NoFocus);
        auto* box = new QVBoxLayout(this);
        const int inset = tokens->metric("popupPadding");
        box->setContentsMargins(inset, 0, inset, 0);
        box->setSpacing(0);
        box->addWidget(view);
        view->show();
        setAccessibleName(config["label"].toString());
        setProperty("mappedQtApi", "Z_Select::optionView");
        setProperty("designState", "disabled");
    }

    ~SelectOptionAdapter() override { delete source; }

private:
    commonWidget::Z_Select* source;
};

} // namespace

QWidget* createSelectOption(const QJsonObject& config, QWidget* parent) {
    if (config["state"] != "disabled")
        throw std::runtime_error("UNSUPPORTED_OPTION_STATE");
    return new SelectOptionAdapter(config, parent);
}

WidgetRegistry::WidgetRegistry(const QString& profile)
    : componentFactory(std::make_unique<commonWidget::ComponentFactory>(profile)) {
    registerType("v2::SelectOptionAdapter", createSelectOption);
}

WidgetRegistry::~WidgetRegistry() = default;

bool WidgetRegistry::contains(const QString& name) const {
    return factories.contains(name) || commonWidget::ComponentFactory::isRegistered(name);
}

void WidgetRegistry::registerType(const QString& name, Factory factory) {
    if (name.isEmpty() || !factory || factories.contains(name) ||
        commonWidget::ComponentFactory::isRegistered(name))
        throw std::runtime_error("INVALID_OR_DUPLICATE_WIDGET_REGISTRATION");
    factories.insert(name, std::move(factory));
}

QWidget* WidgetRegistry::create(const QString& name, const QJsonObject& config,
                                QWidget* parent) const {
    QWidget* result = nullptr;
    if (commonWidget::ComponentFactory::isRegistered(name)) {
        result = componentFactory->create(name, parent);
        componentFactory->applyCalls(result, config["calls"].toArray());
    } else if (factories.contains(name)) {
        result = factories[name](config, parent);
    } else {
        throw std::runtime_error(("UNREGISTERED_UI_CLASS: " + name).toStdString());
    }
    if (!result)
        throw std::runtime_error("NULL_WIDGET_FACTORY");
    return result;
}

} // namespace v2
