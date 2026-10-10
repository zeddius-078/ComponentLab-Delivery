#pragma once
#include "Z_CheckBox.h"
#include "Z_InputField.h"
#include "Z_Select.h"
#include <QWidget>
class QListView;

namespace designer_widgets {
// 页面根持有显式主题依赖；uic 创建子控件时，薄适配类沿父链取得同一份依赖。
class ThemeHost : public QWidget {
public:
    explicit ThemeHost(const QString &profile, QWidget *parent = nullptr);
    std::shared_ptr<const surface::Catalog> surfaces() const { return catalog; }
    std::shared_ptr<const field::Tokens> fields() const { return tokens; }

private:
    std::shared_ptr<const surface::Catalog> catalog;
    std::shared_ptr<const field::Tokens> tokens;
};

// 这些类只补充 QWidget* parent 构造入口，全部行为和绘制继续来自已有组件。
class CheckBox : public commonWidget::Z_CheckBox {
public:
    explicit CheckBox(QWidget *parent = nullptr);
};
class InputField : public commonWidget::Z_InputField {
    Q_OBJECT
    Q_PROPERTY(QString placeholderText READ placeholderText WRITE setPlaceholderText)
public:
    explicit InputField(QWidget *parent = nullptr);
    QString placeholderText() const;
    void setPlaceholderText(const QString &text);
};
class Select : public commonWidget::Z_Select {
public:
    explicit Select(QWidget *parent = nullptr);
};

// 已发布的独立禁用选项展示。页面只设置文字与尺寸，不拆取 Select 内部列表。
// 本轮仅支持只读禁用项，不承诺搜索、多选或完整选择器行为。
class SelectOption : public QWidget {
    Q_OBJECT
    Q_PROPERTY(QString text READ text WRITE setText)
    Q_PROPERTY(QString controlSize READ controlSize WRITE setControlSize)
public:
    explicit SelectOption(QWidget *parent = nullptr);
    explicit SelectOption(std::shared_ptr<const field::Tokens> tokens, QWidget *parent);
    ~SelectOption() override;
    QString text() const { return label; }
    QString controlSize() const { return sizeName; }
    QSize sizeHint() const override;
public slots:
    void setText(const QString &text);
    void setControlSize(const QString &size);
private:
    std::shared_ptr<const field::Tokens> tokens;
    std::unique_ptr<commonWidget::Z_Select> source;
    QListView *view = nullptr;
    QString label, sizeName = "md";
};
} // namespace designer_widgets
