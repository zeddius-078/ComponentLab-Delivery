#pragma once

#include <QList>
#include <QPixmap>
#include <QSize>
#include <QString>
#include <QStringList>

namespace commonWidget {

enum class VisualAssetKind {
    Image,
    Model2D,
};

struct VisualAssetStateSource {
    QString state;
    QString qrcPath;
};

// Public non-QWidget contract for content images and 2D schematic assets.
// Stable IDs remain independent from concrete QRC paths and file names.
struct VisualAssetDescriptor {
    QString id;
    QString displayName;
    VisualAssetKind kind = VisualAssetKind::Image;
    QString format;
    QString source;
    QString license;
    QSize intrinsicSize;
    bool transparent = false;
    bool scalable = false;
    QStringList supportedFits;
    QStringList supportedStates;
    QList<VisualAssetStateSource> stateSources;
};

class VisualAssetCatalog final {
public:
    static QList<VisualAssetDescriptor> descriptors();
    static QList<VisualAssetDescriptor> imageDescriptors();
    static QList<VisualAssetDescriptor> modelDescriptors();
    static bool isSemanticId(const QString& value);
    static bool contains(const QString& id);
    static const VisualAssetDescriptor& descriptor(const QString& id);

    // Resolves the default payload state when state is empty. Known Empty/Error
    // states without a payload fail with NO_ASSET_PAYLOAD; unknown IDs/states
    // fail with UNMAPPED_RESOURCE.
    static QString resolve(const QString& id, const QString& state = QString());
    static QString sha256(const QString& id, const QString& state = QString());

    // Renders the SDK-owned SVG with the requested profile's semantic colors.
    // Fit accepts Contain or Cover and preserves the intrinsic aspect ratio.
    static QPixmap render(const QString& id, const QString& state,
                          const QSize& logicalSize, const QString& fit,
                          const QString& profile, qreal devicePixelRatio = 1.0);
};

}
