#pragma once
#include "Surface.h"
#include "SolidInnerShadow.h"
#include "Z_PushButton.h"
#include <memory>

// Optional appearance policy around the unchanged R2 Button behavior core.
class ButtonAppearanceAdapter : public QObject {
    Q_OBJECT
    Q_PROPERTY(QString appearance READ appearance WRITE setAppearance)
  public:
    ButtonAppearanceAdapter(commonWidget::Z_PushButton* button,
                            std::shared_ptr<const surface::Catalog> catalog);
    QString appearance() const {
        return preset;
    }
    void setAppearance(const QString& id);
    void refresh();

  protected:
    bool eventFilter(QObject*, QEvent*) override;

  private:
    commonWidget::Z_PushButton* button;
    std::shared_ptr<const surface::Catalog> catalog;
    surface::InsetLayer* inset;
    surface::solid_inner_shadow::Layer* solid;
    surface::OuterShadowEffect* outer;
    QString preset = "solid";
    bool queued = false;
};
