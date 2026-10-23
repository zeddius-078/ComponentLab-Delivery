#pragma once

#include <QMainWindow>

class QTreeWidget;
class QWidget;

// ComponentLab owns this presentation shell. It only consumes the published
// Qt-native SDK; component visuals remain owned by the SDK QSS/QRC.
class BaselineWindow final : public QMainWindow {
  public:
    BaselineWindow();

    bool capture(const QString& directory);
    int componentSectionCount() const;
    int componentPageCount() const;
    bool hasObjectInspector() const;

  private:
    void selectPage(int pageIndex);
    void populateObjectInspector(QWidget* target);

    QTreeWidget* navigation_ = nullptr;
    QTreeWidget* objectInspector_ = nullptr;
    QWidget* pages_ = nullptr;
};

// Limited to Gallery canvas, descriptions and inspector; it has no SDK
// component-appearance selector.
QString baselineGalleryCanvasStyleSheet();
