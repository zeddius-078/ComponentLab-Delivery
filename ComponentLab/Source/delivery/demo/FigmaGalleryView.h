#pragma once
#include <QGraphicsView>

namespace native_gallery {
// Keep design-space geometry intact; only the viewport scales to fit the demo.
class FigmaGalleryView final : public QGraphicsView {
  public:
    explicit FigmaGalleryView(QWidget* canvas, QWidget* parent = nullptr);

  protected:
    void resizeEvent(QResizeEvent* event) override;

  private:
    qreal designWidth;
};
} // namespace native_gallery
