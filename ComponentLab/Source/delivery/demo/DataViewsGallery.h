#pragma once
#include <QString>
class QWidget;
namespace native_gallery {
// Passive source-reproduction galleries for native data-view pages whose identity-only
// Z_* wrappers have no ::item rules yet; the interactive widgets live in the appearance tab.
bool hasDataViewsGallery(const QString& pageId);
QWidget* createDataViewsGallery(const QString& pageId, QWidget* parent);
} // namespace native_gallery
