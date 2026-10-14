// 构建期命令行工具：完整 DSL/混合输入 + 映射规则 → .ui、组件清单、页面 QSS、设计树。
// 临时 QWidget 用于规则求值和布局装配，交付运行器加载静态 XML。
#include "SceneBuilder.h"
#include "DesignTree.h"
#include "FormalResources.h"
#include "FormalPublication.h"
#include "ComponentInitializer.h"
#include "ComponentTheme.h"
#include <QApplication>
#include <QCommandLineParser>
#include <QFontDatabase>
#include <QXmlStreamWriter>
#include <QSaveFile>
#include <QDir>
#include <QDirIterator>
#include <QLabel>
#include <QAbstractButton>
#include <QBoxLayout>
#include <QGridLayout>
#include <QFormLayout>
#include <QCryptographicHash>
#include <QTextStream>
#include <QFileInfo>
#include <QSet>
#include <QTemporaryDir>
#include <QRegularExpression>
#include <QProcess>
#include <QLibraryInfo>
#ifdef Q_OS_WIN
#include <qt_windows.h>
#endif

namespace {
// 哈希关联实际文件字节；单文件使用 QSaveFile，正式四件套另在暂存目录整体发布。
QString hash(const QString& file) {
    QFile f(file);
    // 来源文件必须可读，无法取得字节时拒绝生成伪来源哈希。
    if (!f.open(QIODevice::ReadOnly))
        throw std::runtime_error("HASH_INPUT_MISSING");
    return QString::fromLatin1(
        QCryptographicHash::hash(f.readAll(), QCryptographicHash::Sha256).toHex());
}

void save(const QString& file, const QByteArray& bytes) {
    QSaveFile f(file);
    // 打开、写入长度和原子提交全部成功才视为输出完成，防止保留截断文件。
    if (!f.open(QIODevice::WriteOnly) || f.write(bytes) != bytes.size() || !f.commit())
        throw std::runtime_error("OUTPUT_WRITE_FAILED");
}

bool redirectedDirectory(const QString& path) {
#ifdef Q_OS_WIN
    // Qt5 的 isSymLink/canonicalFilePath 未可靠识别 NTFS junction；显式拒绝重解析点。
    const DWORD attributes = GetFileAttributesW(reinterpret_cast<LPCWSTR>(path.utf16()));
    if (attributes != INVALID_FILE_ATTRIBUTES && (attributes & FILE_ATTRIBUTE_REPARSE_POINT))
        return true;
#endif
    return QFileInfo(path).isSymLink();
}

// 创建工具内部目录前逐层检查真实位置，包含 build/delivery 自身的重定向。
QString internalDirectory(const QDir& project, const QString& relative) {
    QDir cursor(QFileInfo(project.absolutePath()).canonicalFilePath());
    for (const auto& part : relative.split('/')) {
        if (!QRegularExpression("^[A-Za-z0-9][A-Za-z0-9_-]*$").match(part).hasMatch())
            throw std::runtime_error("INVALID_INTERNAL_PATH");
        const auto expected = cursor.filePath(part);
        QFileInfo info(expected);
        if (redirectedDirectory(expected)) throw std::runtime_error("FORMAL_OUTPUT_REDIRECTED");
        if (info.exists() && (!info.isDir() || info.isSymLink() ||
            info.canonicalFilePath().compare(expected, Qt::CaseInsensitive) != 0))
            throw std::runtime_error("FORMAL_OUTPUT_REDIRECTED");
        if (!info.exists() && !cursor.mkdir(part)) throw std::runtime_error("OUTPUT_DIRECTORY_FAILED");
        cursor = QDir(expected);
    }
    return cursor.absolutePath();
}

// 正式模式只发布到工具自己的 build/delivery；规范路径同时阻断 .. 和目录联接逃逸。
QString formalDestination(const QString& requested) {
    QDir root(QCoreApplication::applicationDirPath());
    while (!root.exists("Source/HtmlDsl/pipeline")) {
        if (!root.cdUp()) throw std::runtime_error("TOOL_ROOT_NOT_FOUND");
    }
    const auto base = internalDirectory(root, "build/delivery");
    const auto canonicalBase = QFileInfo(base).canonicalFilePath();
    const auto destination = QDir::cleanPath(QFileInfo(requested).absoluteFilePath());
    const auto relative = QDir(canonicalBase).relativeFilePath(destination);
    if (relative == "." || relative.startsWith("../") || QDir::isAbsolutePath(relative))
        throw std::runtime_error("FORMAL_OUTPUT_OUTSIDE_DELIVERY");
    QDir cursor(canonicalBase);
    for (const auto& part : relative.split('/')) {
        if (!QRegularExpression("^[A-Za-z0-9][A-Za-z0-9_-]*$").match(part).hasMatch())
            throw std::runtime_error("INVALID_DELIVERY_PATH_SEGMENT");
        const QFileInfo entry(cursor.filePath(part));
        if (redirectedDirectory(entry.absoluteFilePath()))
            throw std::runtime_error("FORMAL_OUTPUT_REDIRECTED");
        if (entry.exists() && (entry.isSymLink() || !entry.isDir() ||
            !entry.canonicalFilePath().startsWith(canonicalBase + "/", Qt::CaseInsensitive)))
            throw std::runtime_error("FORMAL_OUTPUT_REDIRECTED");
        cursor = QDir(cursor.filePath(part));
    }
    if (QFileInfo::exists(destination)) throw std::runtime_error("FORMAL_OUTPUT_ALREADY_EXISTS");
    return destination;
}

class Emitter {
public:
    // 将组件调用和布局记录按稳定名称索引，写 XML 时据此区分组件与普通容器。
    Emitter(v2::SceneBuilder& scene, const QString& profile, const QString& namingHash,
            bool formal = false, const QJsonObject& componentRules = {})
        : scene(scene), profile(profile), namingHash(namingHash), formal(formal), resources(componentRules) {
        const auto trace = scene.trace();
        // 按稳定 objectName 索引通用组件，写 XML 时将它们视为原生叶节点。
        for (const auto& c : trace["components"].toArray())
            specs.insert(scene.widget(c.toObject()["nodeId"].toString())->objectName(), c.toObject());
        // 选项适配记录需要补齐实际注册类名，供运行时工厂精确创建。
        for (const auto& c : trace["adapters"].toArray()) {
            auto s = c.toObject();
            s["qtClass"] = formal ? "designer_widgets::SelectOption" : "v2::SelectOptionAdapter";
            specs.insert(scene.widget(s["nodeId"].toString())->objectName(), s);
        }
        // 以设计 ID 索引布局轨迹，发射时复用已解析的方向与对齐语义。
        for (const auto& l : trace["layouts"].toArray())
            layouts[l.toObject()["nodeId"].toString()] = l.toObject();
        if (formal) {
            for (auto it = specs.begin(); it != specs.end(); ++it) {
                auto calls = it.value()["calls"].toArray();
                for (int i = 0; i < calls.size(); ++i) {
                    auto call = calls[i].toObject();
                    if (call["api"].toString().endsWith("IconSource")) {
                        call["value"] = resources.use(call["value"].toString());
                        calls[i] = call;
                    }
                }
                it.value()["calls"] = calls;
            }
        }
    }

    void emitPage(QWidget* page, const QString& prefix, const QString& sourceHash,
                  const QJsonObject& design) {
        // customwidgets 声明供 Designer/QUiLoader 识别类型，实际创建由注册表完成。
        QByteArray bytes;
        QXmlStreamWriter writer(&bytes);
        xml = &writer;
        xml->setAutoFormatting(true);
        xml->writeStartDocument();
        xml->writeStartElement("ui");
        xml->writeAttribute("version", "4.0");
        xml->writeTextElement("class", "Generated_" + page->objectName());
        widget(page, true);
        xml->writeStartElement("customwidgets");
        // 排序集合输出，使同一输入重复生成时字节一致。
        auto sortedTypes = types.values();
        std::sort(sortedTypes.begin(), sortedTypes.end());
        // 按排序后的类型列表写 promoted 声明，消除集合迭代顺序对输出字节的影响。
        for (const auto& type : sortedTypes) {
            xml->writeStartElement("customwidget");
            xml->writeTextElement("class", type);
            const auto base = type == "commonWidget::Z_PushButton" ? "QPushButton"
                              : type == "commonWidget::Z_CheckBox" ? "QCheckBox"
                                                                   : "QWidget";
            xml->writeTextElement("extends", base);
            xml->writeTextElement("header", type.startsWith("designer_widgets::") ? "DesignerWidgets.h" : type.startsWith("v2::")
                                                ? "WidgetRegistry.h"
                                                : type.section("::", -1) + ".h");
            xml->writeEndElement();
        }
        xml->writeEndElement();
        xml->writeStartElement("resources");
        if (formal && resources.hasPageResources()) {
            auto className = page->objectName(); className[0] = className[0].toUpper();
            xml->writeEmptyElement("include");
            xml->writeAttribute("location", "../view/res/" + className + ".qrc");
        }
        xml->writeEndElement();
        xml->writeEmptyElement("connections");
        xml->writeEndElement();
        xml->writeEndDocument();
        if (formal) {
            emitFormal(page, prefix, bytes, sourceHash);
            return;
        }
        save(prefix + ".ui", bytes);
        save(prefix + ".qss", qss.toUtf8());
        QJsonArray components;
        // 按有序索引输出参数，保证重复生成的组件清单顺序稳定。
        for (const auto& spec : specs)
            components.append(spec);
        const auto designBytes = QJsonDocument(design).toJson();
        save(prefix + ".design-tree.json", designBytes);
        // 清单关联设计 ID、objectName、API 参数、输入哈希及设计树哈希。
        const QJsonObject manifest{
            {"schemaVersion", 2},
            {"profile", profile},
            {"rootId", page->property("designNodeId").toString()},
            {"nodes", nodes},
            {"layoutNodeIds", layoutNodeIds},
            {"components", components},
            {"sourceSha256", sourceHash},
            {"engineeringNamesSha256", namingHash},
            {"designTreeSha256",
             QString::fromLatin1(
                 QCryptographicHash::hash(designBytes, QCryptographicHash::Sha256).toHex())},
            {"structureSource", "sibling .ui XML"},
            {"generator", "C++ UiCompiler; build-time conversion only"}};
        save(prefix + ".components.json", QJsonDocument(manifest).toJson());
    }

private:
    v2::SceneBuilder& scene;
    QString profile, qss, namingHash;
    QXmlStreamWriter* xml = nullptr;
    QMap<QString, QJsonObject> specs, layouts;
    QJsonObject nodes;
    QJsonArray layoutNodeIds;
    QSet<QString> types;
    bool formal;
    QMap<QString, QString> publicObjects;
    QString layoutSetup;
    QJsonArray structuralDecisions;
    FormalResources resources;

    // 使用 C++ 字符串字面量承载已解析值，不把运行参数外置到设计追溯清单。
    QString literal(const QString& value) const {
        auto text = value;
        text.replace('\\', "\\\\").replace('"', "\\\"");
        text.replace('\n', "\\n").replace('\r', "\\r").replace('\t', "\\t");
        return "QStringLiteral(\"" + text + "\")";
    }

    QString cppValue(const QJsonValue& value) const {
        if (value.isString()) return literal(value.toString());
        if (value.isBool()) return value.toBool() ? "true" : "false";
        if (value.isDouble()) return QString::number(value.toDouble(), 'g', 17);
        throw std::runtime_error("UNSUPPORTED_FORMAL_PARAMETER_TYPE");
    }

    void emitFormal(QWidget* page, const QString& directory, const QByteArray& ui,
                    const QString& sourceHash) {
        auto className = page->objectName();
        className[0] = className[0].toUpper();
        QString header = "#pragma once\n#include <DesignerWidgets.h>\n#include <Z_PushButton.h>\n"
                         "#include <QLabel>\n#include <QHBoxLayout>\n#include <QVBoxLayout>\n"
                         "#include <QGridLayout>\n#include <QFormLayout>\n"
                         "#include <memory>\nnamespace Ui { class Generated_" + page->objectName() + "; }\n"
                         "// 页面结构由 uic 编译；业务通过公开对象、信号槽或继承接入。\nclass " + className +
                         " : public designer_widgets::ThemeHost {\npublic:\n    explicit " + className +
                         "(QWidget* parent = nullptr);\n    ~" + className + "() override;\n";
        QString code = "#include \"" + className + ".h\"\n#include \"ui_" + className +
                       ".h\"\n#include <ComponentFactory.h>\n#include <QFile>\n#include <QResource>\n#include <stdexcept>\n\n" +
                       className + "::" + className + "(QWidget* parent)\n    : ThemeHost(" + literal(profile) +
                       ", parent), ui_(std::make_unique<Ui::Generated_" + page->objectName() + ">()) {\n"
                       "    ui_->setupUi(this);\n    commonWidget::ComponentFactory factory(" + literal(profile) +
                       ");\n    QString error;\n";
        if (resources.hasPageResources())
            code += "    Q_INIT_RESOURCE(" + className + ");\n";
        code += v2::component_initializer::emitInitialization(
            specs,
            [this](const QString& value) { return literal(value); },
            [this](const QJsonValue& value) { return cppValue(value); });
        // 组件属性完成后加载页面级 QSS；组件内部样式继续由 SDK 主题和组件自身管理。
        code += "    {\n"
                "        QFile pageStyle(QStringLiteral(\":/d2c/" + className +
                "/qss/" + className + ".qss\"));\n"
                "        if (!pageStyle.open(QIODevice::ReadOnly))\n"
                "            throw std::runtime_error(\"PAGE_QSS_RESOURCE_MISSING\");\n"
                "        setStyleSheet(QString::fromUtf8(pageStyle.readAll()));\n"
                "    }\n";
        code += layoutSetup + "}\n" + className + "::~" + className + "() = default;\n";
        for (auto it = publicObjects.cbegin(); it != publicObjects.cend(); ++it) {
            // 根页面也有公开访问；其余只暴露最终页面对象，不穿透 SDK 内部结构。
            const auto accessor = "get" + it.key().left(1).toUpper() + it.key().mid(1);
            header += "    " + it.value() + "* " + accessor + "() const;\n";
            code += it.value() + "* " + className + "::" + accessor + "() const { return " +
                    (it.key() == page->objectName() ? "const_cast<" + className + "*>(this)" : "ui_->" + it.key()) + "; }\n";
        }
        header += "private:\n    std::unique_ptr<Ui::Generated_" + page->objectName() + "> ui_;\n};\n";
        // 调用方只允许传入隔离暂存目录；四件套成功后由 main 一次发布整个目录。
        for (const auto& sub : {"form", "include", "src", "view/res/qss"})
            if (!QDir(directory).mkpath(sub)) throw std::runtime_error("OUTPUT_DIRECTORY_FAILED");
        save(directory + "/form/" + className + ".ui", ui);
        save(directory + "/include/" + className + ".h", header.toUtf8());
        save(directory + "/src/" + className + ".cpp", code.toUtf8());
        resources.write(directory, className, qss.toUtf8());
        // uic 输出只用于验证；页面 QRC 随页面包交付，由业务 CMake 编译并链接。
        QProcess uic;
        uic.start(QLibraryInfo::location(QLibraryInfo::BinariesPath) + "/uic.exe",
                  QStringList{directory + "/form/" + className + ".ui"});
        if (!uic.waitForFinished(30000)) {
            uic.kill(); uic.waitForFinished();
            throw std::runtime_error("UIC_VALIDATION_TIMEOUT");
        }
        if (uic.exitStatus() != QProcess::NormalExit || uic.exitCode() != 0)
            throw std::runtime_error(("UIC_VALIDATION_FAILED: " + uic.readAllStandardError()).toStdString());
        QJsonArray diagnostics;
        // 数值来自解析后的明确设计属性，但当前接口未提供可证明的 Token 引用链。
        // 保留生成能力，同时逐节点登记；不把字面值伪报为已完成 Token 接入。
        for (auto it = layouts.cbegin(); it != layouts.cend(); ++it)
            diagnostics.append(QJsonObject{{"code", "RAW_LAYOUT_VALUE"}, {"nodeId", it.key()},
                {"spacing", it.value()["spacing"]}, {"padding", it.value()["padding"]},
                {"upstreamTokenBinding", "not verified by current parser"}});
        formalRecord = QJsonObject{{"schemaVersion", 1}, {"profile", profile},
                                  {"sourceSha256", sourceHash}, {"engineeringNamesSha256", namingHash},
                                  {"nodes", nodes}, {"trace", scene.trace()}, {"diagnostics", diagnostics},
                                  {"resources", resources.trace()}, {"structuralDecisions", structuralDecisions}};
    }

public:
    QJsonObject formalRecord;
private:

    void property(const QString& name, const QString& type, const QString& value,
                  bool dynamic = false) {
        // 动态属性用 stdset=0，保留 designNodeId 等元数据而不要求 C++ setter。
        xml->writeStartElement("property");
        xml->writeAttribute("name", name);
        // 动态元数据没有标准 setter，声明 stdset=0 让 QUiLoader 通过属性保存。
        if (dynamic)
            xml->writeAttribute("stdset", "0");
        xml->writeTextElement(type, value);
        xml->writeEndElement();
    }

    void size(const QString& name, QSize s) {
        xml->writeStartElement("property");
        xml->writeAttribute("name", name);
        xml->writeStartElement("size");
        xml->writeTextElement("width", QString::number(s.width()));
        xml->writeTextElement("height", QString::number(s.height()));
        xml->writeEndElement();
        xml->writeEndElement();
    }

    QString policy(QSizePolicy::Policy p) {
        // 只序列化已实现的 Qt 尺寸策略，未知枚举报错以免写出不可读 XML。
        switch (p) {
        case QSizePolicy::Fixed:
            return "Fixed";
        case QSizePolicy::Minimum:
            return "Minimum";
        case QSizePolicy::Maximum:
            return "Maximum";
        case QSizePolicy::Preferred:
            return "Preferred";
        case QSizePolicy::Expanding:
            return "Expanding";
        default:
            throw std::runtime_error("UNSUPPORTED_UI_SIZE_POLICY");
        }
    }

    QString alignment(Qt::Alignment value) {
        const QMap<Qt::AlignmentFlag, QString> flags{
            {Qt::AlignLeft, "Qt::AlignLeft"},       {Qt::AlignRight, "Qt::AlignRight"},
            {Qt::AlignHCenter, "Qt::AlignHCenter"}, {Qt::AlignTop, "Qt::AlignTop"},
            {Qt::AlignBottom, "Qt::AlignBottom"},   {Qt::AlignVCenter, "Qt::AlignVCenter"}};
        QStringList result;
        // 逐项提取已设置的对齐位，用 XML 可识别的枚举名称组合输出。
        for (auto it = flags.cbegin(); it != flags.cend(); ++it)
            // 只输出实际置位的对齐枚举，避免把未启用的方向写入 XML。
            if (value.testFlag(it.key()))
                result << it.value();
        return result.join('|');
    }

    void spacer(const QString& name, bool horizontal) {
        // 用可扩展占位重现主轴 start/center/end；交叉轴对齐仍写在各 item 上。
        xml->writeStartElement("item");
        xml->writeStartElement("spacer");
        xml->writeAttribute("name", name);
        property("orientation", "enum", horizontal ? "Qt::Horizontal" : "Qt::Vertical");
        property("sizeType", "enum", "QSizePolicy::Expanding");
        size("sizeHint", QSize(0, 0));
        xml->writeEndElement();
        xml->writeEndElement();
    }

    void emitLayout(QWidget* owner, bool flattened = false) {
        const auto id = owner->property("designNodeId").toString();
        const auto name = owner->objectName();
        auto* layout = owner->layout();
        auto* box = qobject_cast<QBoxLayout*>(layout);
        auto* grid = qobject_cast<QGridLayout*>(layout);
        auto* form = qobject_cast<QFormLayout*>(layout);
        if (!layout || (!box && !grid && !form))
            throw std::runtime_error("FRAME_LAYOUT_MISSING");
        const auto rule = layouts.value(id);
        const auto qtLayout = rule["qtLayout"].toString();
        const bool horizontal = qtLayout == "QHBoxLayout";
        const bool linear = box != nullptr;
        xml->writeStartElement("layout");
        xml->writeAttribute("class", qtLayout);
        const auto layoutName = formal ? name + "Layout" : "layout_" + name;
        if (formal) {
            if (publicObjects.contains(layoutName)) throw std::runtime_error("DUPLICATE_ENGINEERING_NAME");
            publicObjects[layoutName] = qtLayout;
            // QBoxLayout 的容器对齐由运行时布局对象恢复；Grid/Form 的单元格位置写入 item 属性。
            if (box) {
                const auto flags = alignment(box->alignment());
                if (!flags.isEmpty())
                    layoutSetup += "    ui_->" + layoutName + "->setAlignment(" + flags + ");\n";
            }
        }
        xml->writeAttribute("name", layoutName);
        if (formal ? flattened : owner->property("structuralFrame").toBool()) {
            nodes[id] = layoutName;
            layoutNodeIds.append(id);
            if (!formal) {
                property("designNodeId", "string", id, true);
                property("designName", "string", owner->property("designName").toString(), true);
            }
        }
        const auto m = layout->contentsMargins();
        property("leftMargin", "number", QString::number(m.left()));
        property("topMargin", "number", QString::number(m.top()));
        property("rightMargin", "number", QString::number(m.right()));
        property("bottomMargin", "number", QString::number(m.bottom()));
        if (box)
            property("spacing", "number", QString::number(box->spacing()));
        else if (grid) {
            property("horizontalSpacing", "number", QString::number(grid->horizontalSpacing()));
            property("verticalSpacing", "number", QString::number(grid->verticalSpacing()));
            for (int row = 0; row < grid->rowCount(); ++row)
                if (grid->rowStretch(row))
                    layoutSetup += "    ui_->" + layoutName + "->setRowStretch(" +
                                   QString::number(row) + ", " + QString::number(grid->rowStretch(row)) + ");\n";
            for (int column = 0; column < grid->columnCount(); ++column)
                if (grid->columnStretch(column))
                    layoutSetup += "    ui_->" + layoutName + "->setColumnStretch(" +
                                   QString::number(column) + ", " + QString::number(grid->columnStretch(column)) + ");\n";
        } else {
            property("horizontalSpacing", "number", QString::number(form->horizontalSpacing()));
            property("verticalSpacing", "number", QString::number(form->verticalSpacing()));
            property("fieldGrowthPolicy", "enum", QString("QFormLayout::%1").arg(
                form->fieldGrowthPolicy() == QFormLayout::FieldsStayAtSizeHint ? "FieldsStayAtSizeHint" :
                form->fieldGrowthPolicy() == QFormLayout::ExpandingFieldsGrow ? "ExpandingFieldsGrow" :
                "AllNonFixedFieldsGrow"));
            property("rowWrapPolicy", "enum", QString("QFormLayout::%1").arg(
                form->rowWrapPolicy() == QFormLayout::DontWrapRows ? "DontWrapRows" :
                form->rowWrapPolicy() == QFormLayout::WrapAllRows ? "WrapAllRows" : "WrapLongRows"));
            const auto labelFlags = alignment(form->labelAlignment());
            const auto formFlags = alignment(form->formAlignment());
            if (!labelFlags.isEmpty()) property("labelAlignment", "set", labelFlags);
            if (!formFlags.isEmpty()) property("formAlignment", "set", formFlags);
        }
        const auto primary = rule["primary"].toString();
        if (box && !formal && !owner->property("structuralFrame").toBool() &&
            (primary == "end" || primary == "center"))
            spacer("before_" + name, horizontal);
        for (int i = 0; i < layout->count(); ++i) {
            auto* item = layout->itemAt(i);
            if (!item->widget())
                throw std::runtime_error("UNEXPECTED_SOURCE_SPACER");
            auto* child = item->widget();
            bool flatten = child->property("structuralFrame").toBool();
            if (formal && flatten) {
                // Qt5 的 addLayout 没有 alignment 参数。若子布局自身不能表达父级对齐，
                // 保留原设计容器作为真实对齐边界；不生成替代包装层或 Spacer。
                const auto own = child->layout()->alignment();
                flatten = (own & item->alignment()) == item->alignment();
                // 父级混有 Fill 子项时，纯子布局会取得整个交叉轴分配框，失去此 Hug
                // 设计容器的独立宽/高上限。保留原容器的尺寸职责，不以空白层补位置。
                const auto crossMask = horizontal ? Qt::AlignVertical_Mask : Qt::AlignHorizontal_Mask;
                if (box && !(box->alignment() & crossMask) && (item->alignment() & crossMask))
                    flatten = false;
                structuralDecisions.append(QJsonObject{{"nodeId", child->property("designNodeId").toString()},
                    {"result", flatten ? "layout" : "original-container"},
                    {"reason", flatten ? "alignment and Hug bounds retained" : "independent alignment or Hug size boundary"}});
            }
            xml->writeStartElement("item");
            const auto a = alignment(item->alignment());
            if (!a.isEmpty() && !flatten)
                xml->writeAttribute("alignment", a);
            if (grid) {
                int row = 0, column = 0, rowSpan = 1, columnSpan = 1;
                grid->getItemPosition(i, &row, &column, &rowSpan, &columnSpan);
                xml->writeAttribute("row", QString::number(row));
                xml->writeAttribute("column", QString::number(column));
                if (rowSpan > 1) xml->writeAttribute("rowspan", QString::number(rowSpan));
                if (columnSpan > 1) xml->writeAttribute("colspan", QString::number(columnSpan));
            } else if (form) {
                int row = 0;
                QFormLayout::ItemRole role = QFormLayout::SpanningRole;
                form->getItemPosition(i, &row, &role);
                xml->writeAttribute("row", QString::number(row));
                xml->writeAttribute("column", QString::number(
                    role == QFormLayout::LabelRole ? 0 : role == QFormLayout::FieldRole ? 1 : 0));
                if (role == QFormLayout::SpanningRole)
                    xml->writeAttribute("colspan", "2");
            }
            if (formal && flatten) {
                emitLayout(child, true);
            } else if (!linear && flatten) {
                // Grid/Form 的嵌套结构直接保留为布局节点，避免用盒式锚定层改变行列语义。
                emitLayout(child);
            } else if (!formal && flatten) {
                const bool anchorHorizontal = !horizontal;
                const auto counter = rule["counter"].toString();
                xml->writeStartElement("layout");
                xml->writeAttribute("class", anchorHorizontal ? "QHBoxLayout" : "QVBoxLayout");
                xml->writeAttribute("name", "anchor_" + child->objectName());
                for (const auto& margin : {"leftMargin", "topMargin", "rightMargin", "bottomMargin",
                                           "spacing"})
                    property(margin, "number", "0");
                if (counter == "end" || counter == "center")
                    spacer("before_" + child->objectName(), anchorHorizontal);
                xml->writeStartElement("item");
                emitLayout(child);
                xml->writeEndElement();
                if (counter == "start" || counter == "center")
                    spacer("after_" + child->objectName(), anchorHorizontal);
                xml->writeEndElement();
            } else
                widget(child);
            xml->writeEndElement();
        }
        if (box && !formal && !owner->property("structuralFrame").toBool() &&
            (primary == "start" || primary == "center"))
            spacer("after_" + name, horizontal);
        xml->writeEndElement();
    }

    void widget(QWidget* w, bool root = false) {
        // 递归只写页面结构。已映射组件作为叶节点，内部图标/绘制层由组件实现维护。
        const auto id = w->property("designNodeId").toString();
        const auto name = w->objectName();
        nodes[id] = name;
        const bool component = specs.contains(name);
        QString type = component                  ? specs[name]["qtClass"].toString()
                       : qobject_cast<QLabel*>(w) ? "QLabel"
                                                  : "QWidget";
        if (formal) {
            const QMap<QString, QString> wrappers{{"commonWidget::Z_CheckBox", "designer_widgets::CheckBox"},
                {"commonWidget::Z_InputField", "designer_widgets::InputField"},
                {"commonWidget::Z_Select", "designer_widgets::Select"}};
            type = wrappers.value(type, type);
            if (publicObjects.contains(name)) throw std::runtime_error("DUPLICATE_ENGINEERING_NAME");
            publicObjects[name] = type;
        }
        // 原生组件需要 customwidgets 声明；普通 QWidget/QLabel 使用 Qt 内置类型。
        if (component)
            types.insert(type);
        xml->writeStartElement("widget");
        xml->writeAttribute("class", type);
        xml->writeAttribute("name", name);
        if (!formal) {
            property("designNodeId", "string", id, true);
            property("designName", "string", w->property("designName").toString(), true);
            property("designType", "string", w->property("designType").toString(), true);
        }
        if (formal && type == "designer_widgets::SelectOption") {
            property("text", "string", specs[name]["label"].toString());
            property("controlSize", "string", specs[name]["controlSize"].toString());
        }
        // 只为根窗口写初始 geometry，子节点位置由布局计算，避免与约束冲突。
        if (root) {
            xml->writeStartElement("property");
            xml->writeAttribute("name", "geometry");
            xml->writeStartElement("rect");
            xml->writeTextElement("x", "0");
            xml->writeTextElement("y", "0");
            xml->writeTextElement("width", QString::number(w->width()));
            xml->writeTextElement("height", QString::number(w->height()));
            xml->writeEndElement();
            xml->writeEndElement();
        }
        size("minimumSize", w->minimumSize());
        size("maximumSize", w->maximumSize());
        xml->writeStartElement("property");
        xml->writeAttribute("name", "sizePolicy");
        xml->writeStartElement("sizepolicy");
        xml->writeAttribute("hsizetype", policy(w->sizePolicy().horizontalPolicy()));
        xml->writeAttribute("vsizetype", policy(w->sizePolicy().verticalPolicy()));
        xml->writeTextElement("horstretch", "0");
        xml->writeTextElement("verstretch", "0");
        xml->writeEndElement();
        xml->writeEndElement();
        // 显式保存禁用初态，让 XML 预览与组件参数保持一致。
        if (!w->isEnabled())
            property("enabled", "bool", "false");
        // 普通文字保存文本与作用域 QSS；按钮只保存 text，其内部材质由组件处理。
        if (auto* text = qobject_cast<QLabel*>(w)) {
            // 标题等普通文字的 QSS 按 objectName 限定，避免覆盖整页组件样式。
            property("text", "string", text->text());
            property("textFormat", "enum", "Qt::PlainText");
            auto css = text->styleSheet();
            css.replace("QLabel {", "QLabel#" + name + " {");
            qss += css + '\n';
        } else if (auto* button = qobject_cast<QAbstractButton*>(w))
            property("text", "string", button->text());
        // 仅展开页面容器，避免把原生组件内部图标和绘制层重复生成到 XML。
        if (!component) {
            // 只输出已启用背景填充的容器，保留其他容器的透明行为。
            if (w->autoFillBackground())
                qss += "QWidget#" + name + " { background-color: " +
                       w->palette().color(QPalette::Window).name(QColor::HexArgb) + "; }\n";
            if (w->layout())
                emitLayout(w);
        }
        xml->writeEndElement();
    }
};
}

int main(int argc, char** argv) {
    // 仍需 QApplication 和字体以求出真实 sizeHint；脚本通常设置 offscreen 运行。
    QApplication app(argc, argv);
    QCommandLineParser cli;
    cli.addHelpOption();
    cli.addOption({"strict-layout", "Require explicit layout gap and margins"});
    cli.addOption({"formal", "Generate isolated uic page package (light-compact only)"});
    // 登记生成必需的输入和输出参数，使 CLI 以固定契约接收路径。
    for (const auto& name : {"raw", "mapping", "extensions", "font", "output"})
        cli.addOption({name, name, "path"});
    cli.process(app);
    try {
        // 逐项检查参数存在后再读取设计，防止空路径误用当前目录。
        for (const auto& name : {"raw", "mapping", "extensions", "font", "output"})
            // 逐个拒绝缺失的必要参数，确保生成器有完整的设计输入、规则、字体和输出位置。
            if (!cli.isSet(name))
                throw std::runtime_error("MISSING_GENERATOR_ARGUMENT");
        // 字体影响 sizeHint 和静态尺寸，加载失败时不能用系统替代字体继续生成。
        if (QFontDatabase::addApplicationFont(cli.value("font")) < 0)
            throw std::runtime_error("FONT_LOAD_FAILED");
        const auto components = v2::readJson(cli.value("mapping") + "/component-mapping.json");
        const bool formal = cli.isSet("formal");
        if (formal && components["profile"] != "light-compact")
            throw std::runtime_error("FORMAL_REQUIRES_LIGHT_COMPACT_MAPPING");
        const auto destination = formal ? formalDestination(cli.value("output")) : QString{};
        component_theme::install(app, components["profile"].toString());
        const auto raw = v2::readJson(cli.value("raw"));
        const auto sourceHash = hash(cli.value("raw"));
        // 先从设计输入投影节点树，再创建临时 Qt 对象，保持两种树的来源明确。
        const auto design = v2::designTree(raw, sourceHash, components["profile"].toString());
        auto layoutRules = v2::readJson(cli.value("mapping") + "/layout-mapping.json");
        if (cli.isSet("strict-layout") || formal)
            layoutRules["requireExplicitMetrics"] = true;
        v2::SceneBuilder scene(raw, components, layoutRules,
                               v2::readJson(cli.value("extensions")),
                               v2::readJson(cli.value("mapping") + "/engineering-names.json"));
        std::unique_ptr<QWidget> page(scene.build());
        page->ensurePolished();
        page->layout()->activate();
        Emitter emitter(scene, components["profile"].toString(),
                        hash(cli.value("mapping") + "/engineering-names.json"), formal, components);
        if (formal) {
            const auto parent = QFileInfo(destination).absolutePath();
            if (!QDir().mkpath(parent)) throw std::runtime_error("OUTPUT_DIRECTORY_FAILED");
            QTemporaryDir staging(parent + "/.staging-XXXXXX");
            if (!staging.isValid()) throw std::runtime_error("OUTPUT_STAGING_FAILED");
            emitter.emitPage(page.get(), staging.path(), sourceHash, design);
            emitter.formalRecord["variableValidation"] = design["variableValidation"];
            emitter.formalRecord["inputs"] = QJsonObject{
                {"componentMappingSha256", hash(cli.value("mapping") + "/component-mapping.json")},
                {"layoutMappingSha256", hash(cli.value("mapping") + "/layout-mapping.json")},
                {"extensionsSha256", hash(cli.value("extensions"))},
                {"fontSha256", hash(cli.value("font"))}};
            // 清单只写包外。检查器必须核对现有字节，不能只信任某次运行的 PASS。
            QJsonObject outputHashes;
            QDirIterator files(staging.path(), QDir::Files, QDirIterator::Subdirectories);
            while (files.hasNext()) {
                const auto file = files.next();
                outputHashes[QDir(staging.path()).relativeFilePath(file)] = hash(file);
            }
            emitter.formalRecord["outputSha256"] = outputHashes;
            // 先保存包外记录，发布失败时标记未发布；成功后整目录可供业务自行导入。
            QDir project(QCoreApplication::applicationDirPath());
            while (!project.exists("Source/HtmlDsl/pipeline")) project.cdUp();
            const auto relative = QDir(project.filePath("build/delivery")).relativeFilePath(destination);
            const auto recordDir = internalDirectory(project, "build/reports/formal-delivery/" + relative);
            v2::publishFormalPackage(staging.path(), destination,
                recordDir + "/generation-record.json", emitter.formalRecord, save);
            staging.setAutoRemove(false);
            QTextStream(stdout) << destination << Qt::endl;
        } else {
            QDir().mkpath(QFileInfo(cli.value("output")).absolutePath());
            emitter.emitPage(page.get(), cli.value("output"), sourceHash, design);
            QTextStream(stdout) << cli.value("output") << ".ui" << Qt::endl;
        }
        return 0;
    } catch (const std::exception& e) {
        QTextStream(stderr) << e.what() << Qt::endl;
        return 1;
    }
}
