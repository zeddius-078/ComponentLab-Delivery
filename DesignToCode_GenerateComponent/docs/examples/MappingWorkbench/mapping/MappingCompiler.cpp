#include "QtMapping.h"
#include <QApplication>
#include <QDir>
#include <QFile>
#include <QSaveFile>
#include <cstdio>
#include <stdexcept>
int main(int argc, char** argv) {
    QApplication app(argc, argv);
    try {
        if (argc != 3)
            throw std::invalid_argument("mapping_compiler INPUT OUTPUT_DIRECTORY");
        const auto files = design_mapping::generate(
            design_mapping::parse(design_mapping::read(QString::fromLocal8Bit(argv[1]))),
            "standard-light");
        const QString directory = QString::fromLocal8Bit(argv[2]);
        if (!QDir().mkpath(directory))
            throw std::runtime_error("Cannot create generated directory");
        for (auto it = files.begin(); it != files.end(); ++it) {
            const auto path = QDir(directory).filePath(it.key());
            if (QFile::exists(path) && design_mapping::read(path) == it.value())
                continue;
            QSaveFile file(path);
            if (!file.open(QIODevice::WriteOnly) || file.write(it.value()) != it.value().size() ||
                !file.commit())
                throw std::runtime_error("Cannot write generated mapping file");
        }
        return 0;
    } catch (const std::exception& error) {
        std::fprintf(stderr, "Mapping: %s\n", error.what());
        return 1;
    }
}
