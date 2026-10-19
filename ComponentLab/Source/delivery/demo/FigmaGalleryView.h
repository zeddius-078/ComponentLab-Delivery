#pragma once
#include <QGraphicsView>

class QGraphicsProxyWidget;
class QHideEvent;
class QShowEvent;

namespace native_gallery {
// Keep design-space geometry intact; only the viewport scales to fit the demo.
class FigmaGalleryView final : public QGraphicsView {
  public:
    explicit FigmaGalleryView(QWidget* canvas, QWidget* parent = nullptr);

  protected:
    void hideEvent(QHideEvent* event) override;
    void resizeEvent(QResizeEvent* event) override;
    void showEvent(QShowEvent* event) override;

  private:
    qreal designWidth;
    QGraphicsProxyWidget* canvasProxy = nullptr;
};
} // namespace native_gallery
