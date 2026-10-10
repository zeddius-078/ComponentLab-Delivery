#include "IconSource.h"
#include <QBuffer>
#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QIconEngine>
#include <QImageReader>
#include <QPainter>
#include <QRegularExpression>
#include <QSvgRenderer>
#include <QUrl>
#include <QXmlStreamReader>
#include <stdexcept>
#include <utility>

namespace commonWidget {
namespace {
void require(bool valid, const QString& message) {
    if (!valid) throw std::invalid_argument(message.toStdString());
}

// Snapshot bytes so a later file move/delete cannot break an already loaded icon.
// QtSvg and Qt's PNG reader own decoding; this engine only supplies stable canvases.
class SourceEngine final : public QIconEngine {
public:
    SourceEngine(QByteArray svg, QImage png, QSize canvas)
        : svgBytes(std::move(svg)), raster(std::move(png)), sourceSize(canvas) {}
    QIconEngine* clone() const override { return new SourceEngine(*this); }
    void paint(QPainter* painter, const QRect& rect, QIcon::Mode, QIcon::State) override {
        QSize target = sourceSize;
        target.scale(rect.size(), Qt::KeepAspectRatio);
        const QRect bounds(rect.x() + (rect.width()-target.width())/2,
                           rect.y() + (rect.height()-target.height())/2,
                           target.width(), target.height());
        painter->save();
        if (!svgBytes.isEmpty()) {
            QSvgRenderer renderer(svgBytes);
            renderer.render(painter, bounds);
        } else {
            painter->setRenderHint(QPainter::SmoothPixmapTransform);
            painter->drawImage(bounds, raster);
        }
        painter->restore();
    }
    QPixmap pixmap(const QSize& size, QIcon::Mode mode, QIcon::State state) override {
        QPixmap result(size); result.fill(Qt::transparent);
        QPainter painter(&result);
        paint(&painter, result.rect(), mode, state);
        return result;
    }
private:
    QByteArray svgBytes;
    QImage raster;
    QSize sourceSize;
};
}

QString IconSource::resolve(const QString& source) {
    QString path = source.trimmed();
    if (path.isEmpty()) return {};
    if (path.startsWith("qrc:/")) {
        const QUrl url(path);
        require(url.isValid() && url.host().isEmpty() && !url.hasQuery() && !url.hasFragment(),
                "Invalid QRC path: " + source);
        path = ':' + url.path(QUrl::FullyDecoded);
    }
    if (path.startsWith(":/")) return QDir::cleanPath(path);
    require(!path.contains("://") && !path.startsWith("file:"),
            "Use a QRC path or a local filesystem path: " + source);
    if (QDir::isRelativePath(path)) path = QDir(QCoreApplication::applicationDirPath()).filePath(path);
    return QDir::cleanPath(QFileInfo(path).absoluteFilePath());
}

QIcon IconSource::load(const QString& source) {
    const QString path = resolve(source);
    if (path.isEmpty()) return {};
    const QString suffix = QFileInfo(path).suffix().toLower();
    require(suffix == "svg" || suffix == "png", "Icon format must be SVG or PNG: " + path);
    QFile file(path);
    require(file.open(QIODevice::ReadOnly), "Cannot open icon: " + path);
    require(file.size() > 0 && file.size() <= 8*1024*1024, "Icon file must be 1 byte..8 MiB: " + path);
    QByteArray bytes = file.readAll();
    QSize canvas;
    QImage png,probe;
    if (suffix == "svg") {
        QXmlStreamReader xml(bytes);
        const auto checkReference=[&path](const QString& text) {
            static const QRegularExpression url("url\\(\\s*['\"]?([^)'\"]+)",QRegularExpression::CaseInsensitiveOption);
            auto matches=url.globalMatch(text);
            while(matches.hasNext()) require(matches.next().captured(1).trimmed().startsWith('#'),
                "SVG external resources are not supported: " + path);
            require(!text.contains("@import",Qt::CaseInsensitive),"SVG external CSS is not supported: " + path);
        };
        while(!xml.atEnd()) {
            xml.readNext();
            require(!xml.isDTD(),"SVG DTD is not supported: " + path);
            if(xml.isStartElement()) {
                require(xml.name()!=QStringLiteral("text"),"Convert SVG text to outlines: " + path);
                for(const auto& attribute:xml.attributes()) {
                    const QString value=attribute.value().toString();
                    if(attribute.name()==QStringLiteral("href"))
                        require(value.trimmed().startsWith('#'),"SVG must use self-contained geometry: " + path);
                    checkReference(value);
                }
            } else if(xml.isCharacters()) checkReference(xml.text().toString());
        }
        require(!xml.hasError(),"Invalid SVG XML: " + path);
        QSvgRenderer renderer(bytes);
        require(renderer.isValid() && !renderer.animated(), "Icon must be a valid static SVG: " + path);
        canvas = renderer.defaultSize();
        probe=QImage(64,64,QImage::Format_ARGB32_Premultiplied); probe.fill(Qt::transparent);
        QPainter painter(&probe);
        renderer.setAspectRatioMode(Qt::IgnoreAspectRatio);
        renderer.render(&painter,QRectF(0,0,64,64));
    } else {
        QBuffer buffer(&bytes); buffer.open(QIODevice::ReadOnly);
        QImageReader reader(&buffer, "PNG");
        canvas = reader.size();
        require(canvas.isValid() && canvas.width() <= 4096 && canvas.height() <= 4096,
                "PNG canvas must be 1..4096 px per axis: " + path);
        png = reader.read();
        require(!png.isNull(), "Cannot decode PNG: " + path);
        probe=png;
    }
    require(canvas.width() > 0 && canvas.height() > 0 && canvas.width() <= 4096 && canvas.height() <= 4096,
            "Invalid icon canvas: " + path);
    QIcon icon(new SourceEngine(suffix == "svg" ? bytes : QByteArray(), png, canvas));
    bool ink = false, transparent = false;
    for (int y=0; y<probe.height(); ++y) for (int x=0; x<probe.width(); ++x) {
        const int alpha = probe.pixelColor(x,y).alpha();
        ink = ink || alpha > 0;
        transparent = transparent || alpha == 0;
    }
    require(ink, "Icon has no visible glyph: " + path);
    require(transparent, "Token-tinted icon needs a transparent background: " + path);
    return icon;
}
}
