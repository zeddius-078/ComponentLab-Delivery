#pragma once
#include "Surface.h"
#include <QMainWindow>
#include <QVector>
#include <memory>
class QTreeWidget;
namespace Ui {
class ComponentLab;
}
namespace commonWidget {
class Z_CheckBox;
class Z_PushButton;
} // namespace commonWidget
class DemoWindow : public QMainWindow {
    Q_OBJECT
  public:
    explicit DemoWindow(const QString& profile);
    ~DemoWindow() override;
    void capture(const QString& path, const QString& pageId = {});
    void setAppearancePreview(const QString& id);

  private:
    std::unique_ptr<Ui::ComponentLab> ui;
    std::shared_ptr<const surface::Catalog> catalog;
    commonWidget::Z_CheckBox* preview = nullptr;
    commonWidget::Z_PushButton* buttonPreview = nullptr;
    QTreeWidget* checkboxTree = nullptr;
    QTreeWidget* buttonTree = nullptr;
    QTreeWidget* inputTree = nullptr;
    QTreeWidget* selectTree = nullptr;
    QWidget* inputPreview = nullptr;
    QWidget* selectPreview = nullptr;
    QString profile;
    QVector<QPair<QTreeWidget*, QWidget*>> extraInspections;
    void inspectPage();
    void matrixPage();
    void groupPage();
    void buttonPage();
    void buttonStatesPage();
    void formPages();
    void nativePages();
    void updateTrees();
};
