#include "QtMapping.h"
#include "MappingGenerated.h"
#include "MappingPage.h"
#include "ComponentTheme.h"
#include "Z_CheckBox.h"
#include "Z_InputField.h"
#include "Z_Select.h"
#include <QApplication>
#include <QBoxLayout>
#include <QDir>
#include <QJsonArray>
#include <QJsonDocument>
#include <QCryptographicHash>
#include <QLabel>
#include <QLineEdit>
#include <QPlainTextEdit>
#include <QProcess>
#include <QFile>
#include <QPushButton>
#include <QSignalSpy>
#include <QStyleFactory>
#include <QTemporaryDir>
#include <QTest>
#include <QXmlStreamReader>
using namespace design_mapping;
using namespace commonWidget;
namespace {
QJsonObject mutate(const QString& id, const std::function<void(QJsonObject&)>& edit) {
    auto contract = example();
    std::function<QJsonObject(QJsonObject)> walk = [&](QJsonObject node) {
        if (node["id"] == id)
            edit(node);
        if (node.contains("children")) {
            QJsonArray children;
            for (auto child : node["children"].toArray())
                children.append(walk(child.toObject()));
            node["children"] = children;
        }
        return node;
    };
    contract["page"] = walk(contract["page"].toObject());
    return contract;
}
} // namespace
class MappingTests : public QObject {
    Q_OBJECT
    QString profile = qEnvironmentVariable("DEMO_PROFILE", "light-comfortable");
  private slots:
    void externalPixsoButton() {
        const auto input = qEnvironmentVariable("PIXSO_BUTTON_CONTRACT");
        if (input.isEmpty())
            QSKIP("Set PIXSO_BUTTON_CONTRACT for the captured Pixso pilot");
        const auto contract = parse(read(input));
        QVERIFY2(validate(contract, profile).isEmpty(), "Pixso contract validation failed");
        std::unique_ptr<QWidget> page(preview(contract, profile));
        auto* button = page->findChild<Z_PushButton*>("pixsoProcessButton");
        QVERIFY(button);
        QCOMPARE(button->text(), QString::fromUtf8("执行工艺"));
        QVERIFY(button->isEnabled());
        QVERIFY(!button->isIconOnly());
        page->resize(360, 120);
        page->show();
        QTest::qWait(50);
        QSignalSpy clicked(button, &QPushButton::clicked);
        QTest::mouseClick(button, Qt::LeftButton);
        QCOMPARE(clicked.count(), 1);
        const auto capture = qEnvironmentVariable("PIXSO_BUTTON_CAPTURE");
        if (!capture.isEmpty())
            QVERIFY(page->grab().save(capture));
    }
    void canonicalFilesAndCompiledUi() {
        const auto contract = example();
        QVERIFY2(validate(contract, profile).isEmpty(),
                 qPrintable(validate(contract, profile).join('\n')));
        const auto files = generate(contract, profile);
        QCOMPARE(files.size(), 7);
        QXmlStreamReader xml(files["MappingGenerated.ui"]);
        while (!xml.atEnd())
            xml.readNext();
        QVERIFY(!xml.hasError());
        QVERIFY(files["MappingGenerated.cpp"].contains("design_mapping::mount"));
        QVERIFY(!files["MappingGenerated.qss"].contains("#6235"));
        QCOMPARE(generate(contract, profile), files);
        MappingGenerated compiled(profile);
        std::unique_ptr<QWidget> loaded(preview(contract, profile));
        for (auto* root : {static_cast<QWidget*>(&compiled), loaded.get()}) {
            QVERIFY(root->findChild<Z_PushButton*>("submitButton"));
            QVERIFY(root->findChild<Z_PushButton*>("searchButton")->isIconOnly());
            QVERIFY(root->findChild<Z_CheckBox*>("qualityCheck")->isChecked());
            QCOMPARE(root->findChild<Z_InputField*>("lotInput")->value(), QString("LOT-2026-0917"));
            QCOMPARE(root->findChild<Z_Select*>("chamberSelect")->values(), QStringList{"etch-01"});
            const field::Tokens values(profile);
            auto* layout = root->findChild<QBoxLayout*>("layout_MappingExample");
            QCOMPARE(layout->spacing(), values.metric("labelGap"));
            QCOMPARE(layout->contentsMargins().left(), values.metric("tagPadding"));
            QCOMPARE(root->findChild<Z_InputField*>("lotInput")->control()->controlSize(),
                     QString("md"));
        }
    }
    void actualInteractions() {
        std::unique_ptr<QWidget> page(preview(example(), profile));
        page->resize(600, 500);
        page->show();
        QTest::qWait(40);
        auto* b = page->findChild<Z_PushButton*>("submitButton");
        auto* icon = page->findChild<Z_PushButton*>("searchButton");
        const field::Tokens metrics(profile);
        QCOMPARE(icon->mapTo(page.get(), QPoint()).x() - b->mapTo(page.get(), QPoint()).x() -
                     b->width(),
                 metrics.metric("labelGap"));
        QSignalSpy clicked(b, &QPushButton::clicked);
        QTest::mouseClick(b, Qt::LeftButton);
        QCOMPARE(clicked.count(), 1);
        auto* c = page->findChild<Z_CheckBox*>("qualityCheck");
        QTest::mouseClick(c, Qt::LeftButton);
        QVERIFY(!c->isChecked());
        auto* input = page->findChild<Z_InputField*>("lotInput");
        input->lineEdit()->selectAll();
        QTest::keyClicks(input->lineEdit(), "LOT-002");
        QCOMPARE(input->value(), QString("LOT-002"));
        auto* select = page->findChild<Z_Select*>("chamberSelect");
        select->setValues({"etch-02"});
        QCOMPARE(select->values(), QStringList{"etch-02"});
    }
    void unsupportedInputsFailClosed() {
        const QList<QJsonObject> invalid{
            mutate("submitButton",
                   [](QJsonObject& n) {
                       auto p = n["properties"].toObject();
                       p["fill"] = "#ff0000";
                       n["properties"] = p;
                   }),
            mutate("submitButton",
                   [](QJsonObject& n) {
                       auto p = n["properties"].toObject();
                       p["disabled"] = "false";
                       n["properties"] = p;
                   }),
            mutate("submitButton",
                   [](QJsonObject& n) {
                       auto p = n["properties"].toObject();
                       p["leadingIcon"] = "https://example.org/icon.svg";
                       n["properties"] = p;
                   }),
            mutate("submitButton",
                   [](QJsonObject& n) { n["tokenRefs"] = QJsonArray{"arbitrary.color"}; }),
            mutate("submitButton", [](QJsonObject& n) { n["component"] = "Table"; }),
            mutate("qualityCheck", [](QJsonObject& n) { n["id"] = "submitButton"; }),
            mutate("qualityCheck",
                   [](QJsonObject& n) {
                       auto p = n["properties"].toObject();
                       p["controlSize"] = "md";
                       n["properties"] = p;
                   }),
            mutate("searchButton",
                   [](QJsonObject& n) {
                       auto p = n["properties"].toObject();
                       p["accessibleName"] = "";
                       n["properties"] = p;
                   }),
            mutate("MappingExample",
                   [](QJsonObject& n) {
                       auto p = n["layout"].toObject();
                       p["margin"] = 24;
                       n["layout"] = p;
                   }),
            mutate("chamberSelect", [](QJsonObject& n) {
                auto p = n["properties"].toObject();
                p["values"] = QJsonArray{"missing"};
                n["properties"] = p;
            })};
        QTemporaryDir directory;
        for (const auto& contract : invalid) {
            QVERIFY(!validate(contract, profile).isEmpty());
            QVERIFY_EXCEPTION_THROWN(exportFiles(contract, profile, directory.path()),
                                     std::invalid_argument);
        }
        QVERIFY(
            QDir(directory.path()).entryList(QDir::NoDotAndDotDot | QDir::AllEntries).isEmpty());
        QVERIFY_EXCEPTION_THROWN(parse("not JSON"), std::invalid_argument);
        QVERIFY_EXCEPTION_THROWN(parse("[]"), std::invalid_argument);
    }
    void stateAndContentModes() {
        auto contract = mutate("searchButton", [](QJsonObject& n) {
            auto p = n["properties"].toObject();
            p["loading"] = true;
            n["properties"] = p;
        });
        std::unique_ptr<QWidget> page(preview(contract, profile));
        auto* icon = page->findChild<Z_PushButton*>("searchButton");
        QVERIFY(icon->isIconOnly());
        QVERIFY(!icon->isEnabled());
        icon->setLoading(false);
        QVERIFY(icon->isEnabled());
        auto disabled = mutate("lotInput", [](QJsonObject& n) {
            auto p = n["properties"].toObject();
            p["disabled"] = true;
            n["properties"] = p;
        });
        std::unique_ptr<QWidget> disabledPage(preview(disabled, profile));
        QVERIFY(!disabledPage->findChild<Z_InputField*>("lotInput")->isEnabled());
        for (auto material : surface::Catalog(profile).buttonPresetIds()) {
            auto input = mutate("submitButton", [material](QJsonObject& n) {
                auto p = n["properties"].toObject();
                p["appearance"] = material;
                n["properties"] = p;
            });
            if (validate(input, profile).isEmpty()) {
                std::unique_ptr<QWidget> materialPage(preview(input, profile));
                QVERIFY(materialPage);
            }
        }
    }
    void exportProvenanceAndNonOverwrite() {
        QTemporaryDir directory;
        const auto path = exportFiles(example(), profile, directory.path());
        const auto report = parse(read(path + "/mapping-report.json"));
        QCOMPARE(report["source"].toObject()["kind"].toString(), QString("localFixture"));
        QCOMPARE(report["tokenPipelineModified"].toBool(), false);
        const auto files = generate(example(), profile);
        for (auto it = files.begin(); it != files.end(); ++it)
            QCOMPARE(read(path + '/' + it.key()), it.value());
        auto fake = example();
        auto source = fake["source"].toObject();
        source["kind"] = "pixsoNormalized";
        fake["source"] = source;
        QVERIFY(!validate(fake, profile).isEmpty());
    }
    void canonicalButtonEvidence() {
        // Hash-bound review is simulated only inside this test's temporary directory.
        QTemporaryDir temp;
        QVERIFY(temp.isValid());
        const QString sourceDir = QString::fromUtf8(TEST_SOURCE_DIR);
        const QString script = sourceDir + "/mapping/design/pixso_button.py";
        const QString index = sourceDir + "/../../DesignEvidence/packages/DesignTest-2-145/canonical/index.json";
        const QString rule = sourceDir + "/mapping/design/pixso-button/rule.json";
        const QString reviewFile = temp.path() + "/test-review.json";
        QProcess python;
        python.start(QString::fromUtf8(TEST_PYTHON), {"-B", script, index, rule, reviewFile, "--inspect"});
        QVERIFY(python.waitForFinished(30000));
        QVERIFY2(python.exitCode() == 0, python.readAllStandardError().constData());
        QFile review(reviewFile);
        QVERIFY(review.open(QIODevice::ReadOnly));
        const auto bytes = review.readAll();
        QVERIFY(bytes.size() < 200000);
        const auto hash = QString::fromLatin1(QCryptographicHash::hash(bytes, QCryptographicHash::Sha256).toHex());
        const QString contractFile = temp.path() + "/test-contract.json";
        python.start(QString::fromUtf8(TEST_PYTHON), {"-B", script, index, rule, contractFile, "--reviewed-sha256", hash});
        QVERIFY(python.waitForFinished(30000));
        QVERIFY2(python.exitCode() == 0, python.readAllStandardError().constData());
        QFile file(contractFile);
        QVERIFY(file.open(QIODevice::ReadOnly));
        auto contract = parse(file.readAll());
        QVERIFY2(validate(contract, profile).isEmpty(), qPrintable(validate(contract, profile).join('\n')));
        const auto originalPage = contract["page"].toObject();
        auto child = originalPage["children"].toArray()[0].toObject();
        const auto identity = child["design"].toObject();
        QVERIFY(!identity.contains("instanceNodeId"));
        QCOMPARE(identity["variantNodeId"].toString(), QString("3:5686"));
        QCOMPARE(identity["nativeComponentKey"].toString(), QString("5db2a212943f439833dd3b8d6f8957d687a6edb1"));
        std::unique_ptr<QWidget> widget(preview(contract, profile));
        auto button = widget->findChild<QPushButton*>("pixsoProcessButton");
        QVERIFY(button);
        QCOMPARE(button->text(), QString::fromUtf8("执行工艺"));
        auto changedIdentity = identity;
        changedIdentity["nativeComponentKey"] = QString(40, '0');
        child["design"] = changedIdentity;
        auto changedPage = originalPage;
        changedPage["children"] = QJsonArray{child};
        contract["page"] = changedPage;
        QVERIFY(!validate(contract, profile).isEmpty());
        contract["page"] = originalPage;
        auto source = contract["source"].toObject();
        source["canonicalIndexSha256"] = QString(64, '0');
        contract["source"] = source;
        QVERIFY(!validate(contract, profile).isEmpty());
        source["kind"] = "pixsoNormalized";
        contract["source"] = source;
        QVERIFY(!validate(contract, profile).isEmpty());
    }
    void designIdentityAndSizedReferences() {
        auto live = example();
        live["source"] = QJsonObject{{"kind", "figmaNormalized"},
                                     {"documentUrl", "https://www.figma.com/design/test-fixture"},
                                     {"nodeId", "synthetic-page"},
                                     {"rawSha256", QString(64, '0')},
                                     {"reviewed", true}};
        QVERIFY(!validate(live, profile).isEmpty());
        // Synthetic identities exercise validation only; they are never design evidence.
        std::function<QJsonObject(QJsonObject)> identify = [&](QJsonObject node) {
            if (node.contains("children")) {
                QJsonArray children;
                for (const auto& child : node["children"].toArray())
                    children.append(identify(child.toObject()));
                node["children"] = children;
            } else {
                node["design"] =
                    QJsonObject{{"instanceNodeId", "synthetic-" + node["id"].toString()},
                                {"componentKey", "synthetic-component"},
                                {"libraryVersion", "test-only"},
                                {"alias", node["component"]}};
            }
            return node;
        };
        live["page"] = identify(live["page"].toObject());
        auto attachSnapshot = [](QJsonObject source, const QJsonObject& snapshot) {
            const auto bytes = QJsonDocument(snapshot).toJson(QJsonDocument::Compact);
            const auto digest = QString::fromLatin1(QCryptographicHash::hash(bytes, QCryptographicHash::Sha256).toHex());
            source["rawSha256"] = digest;
            source["reviewedSha256"] = digest;
            source["snapshotBase64"] = QString::fromLatin1(bytes.toBase64());
            return source;
        };
        live["source"] = attachSnapshot(live["source"].toObject(),
            {{"provider", "figma"}, {"fileKey", "test-fixture"},
             {"root", QJsonObject{{"id", "synthetic-page"}}}});
        QVERIFY2(validate(live, profile).isEmpty(), qPrintable(validate(live, profile).join('\n')));
        for (const auto* key : {"snapshotBase64", "reviewedSha256", "rawSha256"}) {
            auto broken = live; auto src = broken["source"].toObject();
            src[key] = "changed"; broken["source"] = src;
            QVERIFY(!validate(broken, profile).isEmpty());
        }
        auto figmaSource = live["source"].toObject();
        figmaSource["kind"] = "figmaNormalized";
        figmaSource["documentUrl"] = "https://www.figma.com/design/synthetic-test";
        figmaSource = attachSnapshot(figmaSource,
            {{"provider", "figma"}, {"fileKey", "synthetic-test"},
             {"root", QJsonObject{{"id", "synthetic-page"}}}});
        auto figmaContract = live;
        figmaContract["source"] = figmaSource;
        QVERIFY2(validate(figmaContract, profile).isEmpty(), qPrintable(validate(figmaContract, profile).join('\n')));
        for (const auto* invalidUrl : {"https://www.figma.com.evil.example/design/test",
                                      "https://user@www.figma.com/design/test",
                                      "http://www.figma.com/design/test",
                                      "https://pixso.cn/design/test"}) {
            figmaSource["documentUrl"] = invalidUrl;
            figmaContract["source"] = figmaSource;
            QVERIFY(!validate(figmaContract, profile).isEmpty());
        }
        std::unique_ptr<QWidget> page(preview(live, profile));
        QCOMPARE(page->findChild<Z_InputField*>("lotInput")->property("designNodeId").toString(),
                 QString("synthetic-lotInput"));
        auto mismatched = mutate("submitButton", [](QJsonObject& n) {
            auto p = n["properties"].toObject();
            p["controlSize"] = "lg";
            n["properties"] = p;
        });
        QVERIFY(!validate(mismatched, profile).isEmpty());
        auto matched = mutate("submitButton", [](QJsonObject& n) {
            auto p = n["properties"].toObject();
            p["controlSize"] = "lg";
            n["properties"] = p;
            n["tokenRefs"] = QJsonArray{"component.basic.button.size.lg.height"};
        });
        QVERIFY(validate(matched, profile).isEmpty());
        auto smallest = mutate("submitButton", [](QJsonObject& n) {
            auto p = n["properties"].toObject();
            p["controlSize"] = "xs";
            n["properties"] = p;
            n["tokenRefs"] = QJsonArray{"semantic.dimension.control.height-xs",
                                        "base.stoken.typography.font-size.label.default"};
        });
        QVERIFY(validate(smallest, profile).isEmpty());
        auto quoted = mutate("submitButton", [](QJsonObject& n) {
            auto p = n["properties"].toObject();
            p["text"] = "quotes \", unicode, )QTM0\" & <tag>";
            n["properties"] = p;
        });
        const auto code = generate(quoted, profile)["MappingGenerated.cpp"];
        QVERIFY(code.contains("QByteArray(R\"QTM"));
        QVERIFY(!code.contains("fromBase64"));
        QVERIFY(code.contains("quotes"));
        auto literal = mutate("lotInput", [](QJsonObject& n) {
            auto p = n["properties"].toObject();
            p["label"] = "<b>Literal label</b>";
            n["properties"] = p;
        });
        std::unique_ptr<QWidget> literalPage(preview(literal, profile));
        auto* field = literalPage->findChild<Z_InputField*>("lotInput");
        bool found = false;
        for (auto* label : field->findChildren<QLabel*>()) {
            if (label->text() == "<b>Literal label</b>") {
                found = true;
                QCOMPARE(label->textFormat(), Qt::PlainText);
            }
        }
        QVERIFY(found);
    }
    void staleContractCannotExport() {
        QWidget flow, filePage;
        new QVBoxLayout(&flow);
        new QVBoxLayout(&filePage);
        MappingPage controller(profile, &flow, &filePage, &flow);
        auto* button = flow.findChild<QPushButton*>("mappingExport");
        QVERIFY(button->isEnabled());
        auto* editor = filePage.findChild<QPlainTextEdit*>("mappingContractEditor");
        editor->setPlainText("{}");
        QVERIFY(!button->isEnabled());
        QVERIFY(!controller.applyContract());
        QVERIFY(!button->isEnabled());
        QVERIFY(flow.findChild<Z_InputField*>("lotInput"));
    }
};
int main(int argc, char** argv) {
    QApplication app(argc, argv);
    app.setStyle(QStyleFactory::create("Fusion"));
    component_theme::install(app, qEnvironmentVariable("DEMO_PROFILE", "light-comfortable"));
    MappingTests tests;
    return QTest::qExec(&tests, argc, argv);
}
#include "MappingTests.moc"
