// 使用生产发布事务和真实临时文件系统；仅替换记录写入，模拟确定的磁盘写入失败。
#include "FormalPublication.h"
#include <QFile>
#include <QSaveFile>
#include <QTemporaryDir>
#include <QTest>

namespace {
void save(const QString& path, const QByteArray& bytes) {
    QSaveFile file(path);
    if (!file.open(QIODevice::WriteOnly) || file.write(bytes) != bytes.size() || !file.commit())
        throw std::runtime_error("OUTPUT_WRITE_FAILED");
}
QJsonObject readRecord(const QString& path) {
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) return {};
    return QJsonDocument::fromJson(file.readAll()).object();
}
}
class FormalPublicationTests : public QObject {
    Q_OBJECT
private slots:
    void successfulPublish() {
        QTemporaryDir root;
        QVERIFY(root.isValid());
        QTemporaryDir staging(root.path() + "/staging-XXXXXX");
        save(staging.path() + "/Page.ui", "complete package");
        QJsonObject record;
        const auto target = root.path() + "/page";
        const auto log = root.path() + "/record.json";
        v2::publishFormalPackage(staging.path(), target, log, record, save);
        staging.setAutoRemove(false);
        QVERIFY(QFile::exists(target + "/Page.ui"));
        QVERIFY(readRecord(log)["published"].toBool());
    }
    void firstRecordFailurePublishesNothing() {
        QTemporaryDir root;
        const auto target = root.path() + "/page";
        const auto log = root.path() + "/record.json";
        QString originalStaging;
        {
            QTemporaryDir staging(root.path() + "/staging-XXXXXX");
            originalStaging = staging.path();
            save(staging.path() + "/Page.ui", "complete package");
            QJsonObject record;
            auto fail = [](const QString&, const QByteArray&) { throw std::runtime_error("INJECTED_WRITE_FAILURE"); };
            QVERIFY_EXCEPTION_THROWN(v2::publishFormalPackage(staging.path(), target, log, record, fail), std::runtime_error);
            QVERIFY(!QFile::exists(target));
            QVERIFY(!QFile::exists(log));
        }
        QVERIFY(!QFile::exists(originalStaging));
    }
    void finalRecordFailureRetractsPackage() {
        QTemporaryDir root;
        const auto target = root.path() + "/page";
        const auto log = root.path() + "/record.json";
        QString originalStaging;
        int writes = 0;
        {
            QTemporaryDir staging(root.path() + "/staging-XXXXXX");
            originalStaging = staging.path();
            save(staging.path() + "/Page.ui", "complete package");
            QJsonObject record;
            const auto writer = [&](const QString& path, const QByteArray& bytes) {
                if (++writes == 2) {
                    // 失败发生在整包已经重命名之后，而不是生成早期。
                    if (!QFile::exists(target + "/Page.ui")) throw std::logic_error("PACKAGE_NOT_PUBLISHED");
                    throw std::runtime_error("INJECTED_FINAL_WRITE_FAILURE");
                }
                save(path, bytes);
            };
            QVERIFY_EXCEPTION_THROWN(v2::publishFormalPackage(staging.path(), target, log, record, writer), std::runtime_error);
            QCOMPARE(writes, 2);
            QVERIFY(!QFile::exists(target));
            QVERIFY(!readRecord(log)["published"].toBool(true));
            QVERIFY(!record["published"].toBool(true));
            QVERIFY(QFile::exists(staging.path() + "/Page.ui"));
        }
        QVERIFY(!QFile::exists(originalStaging));
    }
    void destinationConflictPreservesExistingFiles() {
        QTemporaryDir root;
        QTemporaryDir staging(root.path() + "/staging-XXXXXX");
        const auto target = root.path() + "/page";
        const auto log = root.path() + "/record.json";
        QVERIFY(QDir().mkdir(target));
        save(target + "/owned.txt", "business-owned");
        save(staging.path() + "/Page.ui", "new package");
        QJsonObject record;
        QVERIFY_EXCEPTION_THROWN(v2::publishFormalPackage(staging.path(), target, log, record, save), std::runtime_error);
        QFile existing(target + "/owned.txt");
        QVERIFY(existing.open(QIODevice::ReadOnly));
        QCOMPARE(existing.readAll(), QByteArray("business-owned"));
        QVERIFY(!QFile::exists(target + "/Page.ui"));
        QVERIFY(!readRecord(log)["published"].toBool(true));
    }
};
QTEST_GUILESS_MAIN(FormalPublicationTests)
#include "FormalPublicationTests.moc"
