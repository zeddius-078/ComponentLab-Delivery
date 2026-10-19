#pragma once
#include <QWidget>
class QLineEdit;
class QListWidget;
class QTabWidget;
class QTableWidget;

namespace component_lab_v2 {

// Data-driven V2 component map page: every ComponentLab component gets the two
// required tabs — 外观 (appearance bindings: Figma variable -> canonical token
// -> resolved value) and 状态表 (variant axes and supported states) — rendered
// from the published V2 artifacts:
//   figma-component-map-v2.json  (variable-group <-> component full mapping)
//   component-registry.json      (state matrix per component)
//   tokens_qt.json               (canonical resolved values)
// No per-component hand-written gallery code is needed; the page stays
// complete as long as the V2 source keeps producing these artifacts.
class V2ComponentMapPage : public QWidget {
    Q_OBJECT
  public:
    explicit V2ComponentMapPage(QWidget* parent = nullptr);

  private:
    QLineEdit* filter = nullptr;
    QListWidget* componentList = nullptr;
    QTabWidget* detailTabs = nullptr;
    QTableWidget* appearanceTable = nullptr;
    QTableWidget* stateTable = nullptr;
    void selectComponent(int row);
};

} // namespace component_lab_v2
