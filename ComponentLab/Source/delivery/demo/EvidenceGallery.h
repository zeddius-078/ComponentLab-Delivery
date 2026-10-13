#pragma once

#include <QString>
#include <memory>

class QWidget;

namespace commonWidget {
class ComponentFactory;
struct DesignComponentPage;
}

namespace native_gallery {

// Evidence-backed Figma galleries. Each current component Variant is placed at its captured
// source geometry; an SDK runtime class is instantiated when the public catalog exposes one,
// with a passive evidence specimen retained only for sets without a runtime identity.
bool hasEvidenceGallery(const QString& pageId);
QWidget* createEvidenceGallery(
    const commonWidget::DesignComponentPage& page,
    const std::shared_ptr<commonWidget::ComponentFactory>& factory,
    QWidget* parent);

} // namespace native_gallery
