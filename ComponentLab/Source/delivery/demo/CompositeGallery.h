#pragma once
#include <QString>
class QWidget;
namespace native_gallery {
// Passive source-reproduction galleries for the three composite navigation pages whose
// public SDK implementation is blocked (Navbar / Sidebar / Desktop Bottom Navigation).
bool hasCompositeGallery(const QString& pageId);
QWidget* createCompositeGallery(const QString& pageId, QWidget* parent);
} // namespace native_gallery
