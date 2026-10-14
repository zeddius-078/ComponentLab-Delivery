// 布局测试将控件绘制与排布分开，使用轻量测量控件验证尺寸、对齐和可见性。
#include "LayoutConverter.h"
#include <QApplication>
#include <QFile>
#include <QJsonDocument>
#include <QTest>
#include <QLabel>
#include <QFormLayout>
#include <QGridLayout>

QJsonObject read(const QString& path) {
    QFile f(QString(D2C_ROOT) + "/" + path);
    // 缺失测试输入时立即失败，避免用空对象验证布局而得到误导性的结果。
    if (!f.open(QIODevice::ReadOnly))
        throw std::runtime_error("INPUT_MISSING");
    return QJsonDocument::fromJson(f.readAll()).object();
}

// 通过可控 sizeHint 区分 Hug 与 Fixed，避免组件自身最小尺寸干扰布局结论。
class MeasuredWidget : public QWidget {
public:
    MeasuredWidget(QSize hint, QWidget* parent) : QWidget(parent), hint(hint) {}

    QSize sizeHint() const override {
        return hint;
    }

private:
    QSize hint;
};

class LayoutTests : public QObject {
    Q_OBJECT
private:
    void verify(QJsonObject node) {
        // 保存的设计坐标作为独立参照，转换器只消费布局语义与尺寸字段。
        d2c::LayoutConverter converter(read("HtmlDsl/pipeline/mapping/layout-mapping.json"),
                                       read("HtmlDsl/pipeline/design-inputs/node-12-382-full/canonical.raw.json"));
        QWidget owner;
        owner.resize(node["width"].toInt(), node["height"].toInt());
        auto* layout = converter.convert(node, &owner, [](const QJsonObject& n, QWidget* p) {
            auto* w = new MeasuredWidget(
                QSize(qRound(n["width"].toDouble()), qRound(n["height"].toDouble())), p);
            w->setObjectName(n["guid"].toString());
            return w;
        });
        owner.show();
        QApplication::processEvents();
        layout->activate();
        const auto children = node["childNode"].toArray();
        QCOMPARE(layout->count(), children.size());
        // 按 Qt 排列顺序逐个对照反向 DSL 子项，验证顺序、局部坐标和尺寸均对应同一节点。
        for (int i = 0; i < children.size(); ++i) {
            const auto source = children[children.size() - 1 - i].toObject();
            auto* widget = layout->itemAt(i)->widget();
            QCOMPARE(widget->objectName(), source["guid"].toString());
            // 坐标允许 1px 舍入误差；宽高按设计值舍入后要求完全一致。
            QVERIFY2(qAbs(widget->x() - source["left"].toDouble()) <= 1,
                     qPrintable(widget->objectName() + " x"));
            QVERIFY2(qAbs(widget->y() - source["top"].toDouble()) <= 1,
                     qPrintable(widget->objectName() + " y"));
            QCOMPARE(widget->width(), qRound(source["width"].toDouble()));
            QCOMPARE(widget->height(), qRound(source["height"].toDouble()));
        }
        const auto report = qEnvironmentVariable("D2C_LAYOUT_REPORT",
            QString(D2C_ROOT) + "/../build/reports/layout");
        QFile file(report + "/" +
                   node["autoLayout"].toObject()["stackMode"].toString() + "-result.json");
        QVERIFY(file.open(QIODevice::WriteOnly));
        file.write(QJsonDocument(converter.trace()).toJson());
    }

private slots:

    // 正式生成必须区分字段缺失和明确的零，不能用默认零掩盖设计缺口。
    void strictLayoutMetrics() {
        auto rules = read("HtmlDsl/pipeline/mapping/layout-mapping.json");
        rules["requireExplicitMetrics"] = true;
        QJsonObject fields{{"stackMode", "VERTICAL"}, {"autoLayoutItemSpacing", 0},
                           {"autoLayoutPaddingLeft", 0}, {"autoLayoutPaddingTop", 0},
                           {"autoLayoutPaddingRight", 0}, {"autoLayoutPaddingBottom", 0}};
        for (const auto& key : {"autoLayoutItemSpacing", "autoLayoutPaddingLeft",
                                "autoLayoutPaddingTop", "autoLayoutPaddingRight",
                                "autoLayoutPaddingBottom"}) {
            auto missing = fields;
            missing.remove(key);
            QWidget owner;
            d2c::LayoutConverter converter(rules);
            bool rejected = false;
            try {
                converter.convert({{"autoLayout", missing}}, &owner,
                    [](const QJsonObject&, QWidget* p) { return new QWidget(p); });
            } catch (const std::runtime_error& e) {
                QCOMPARE(QString::fromUtf8(e.what()), QString("MISSING_LAYOUT_METRIC: ") + key);
                rejected = true;
            }
            QVERIFY2(rejected, key);
            QVERIFY(!owner.layout());
        }
        QWidget owner;
        d2c::LayoutConverter converter(rules);
        auto* layout = converter.convert({{"autoLayout", fields}}, &owner,
            [](const QJsonObject&, QWidget* p) { return new QWidget(p); });
        QCOMPARE(layout->spacing(), 0);
        QCOMPARE(layout->contentsMargins(), QMargins(0, 0, 0, 0));
    }

    // 验证两个方向的交叉轴 Fill 随窗口变化，且始终扣除父容器内边距。
    void crossAxisFillResizes() {
        // 分别检查横纵方向的交叉轴 Fill，确保宽度和高度两个分支都随父窗口变化。
        for (bool horizontal : {false, true}) {
            QJsonObject child{
                {"width", 20},
                {"height", 10},
                {"autoLayout", QJsonObject{{"stackChildCounterSizing", "RESIZE_TO_FIT"}}}};
            QJsonObject root{
                {"autoLayout", QJsonObject{{"stackMode", horizontal ? "HORIZONTAL" : "VERTICAL"},
                                           {"autoLayoutPaddingLeft", 7},
                                           {"autoLayoutPaddingRight", 13},
                                           {"autoLayoutPaddingTop", 11},
                                           {"autoLayoutPaddingBottom", 17}}},
                {"childNode", QJsonArray{child}}};
            QWidget owner;
            owner.resize(200, 100);
            d2c::LayoutConverter converter(read("HtmlDsl/pipeline/mapping/layout-mapping.json"));
            auto* layout = converter.convert(root, &owner, [](const QJsonObject&, QWidget* p) {
                return new QWidget(p);
            });
            owner.show();
            QApplication::processEvents();
            layout->activate();
            auto* widget = layout->itemAt(0)->widget();
            QCOMPARE(horizontal ? widget->height() : widget->width(), horizontal ? 72 : 180);
            owner.resize(300, 200);
            QApplication::processEvents();
            layout->activate();
            QCOMPARE(horizontal ? widget->height() : widget->width(), horizontal ? 172 : 280);
        }
    }

    // 使用真实输入组件的母版与覆盖字段检查内部交叉轴尺寸。
    void realInputFillOverrides() {
        const auto raw = read("HtmlDsl/pipeline/design-inputs/node-12-382-full/canonical.raw.json");
        QJsonObject instance, master;
        // 先在真实页面中定位输入框实例，后续覆盖字段都以这个实例为准。
        for (const auto& n : raw["pixTreeNodes"].toArray()[0].toObject()["childNode"].toArray())
            // 只提取目标输入框，避免将页面上其他实例的覆盖字段用于该测试。
            if (n.toObject()["guid"] == "12:385")
                instance = n.toObject();
        // 遍历组件集，再进入各母版，按实例引用定位尺寸计算的基础定义。
        for (const auto& family : raw["pixComponentTreeDslNodes"].toArray())
            // 检查当前组件集的每个母版，支持目标母版位于任意集合位置。
            for (const auto& n : family.toObject()["childNode"].toArray())
                // 用母版 GUID 精确匹配，避免名称相同但变体不同的定义混入。
                if (n.toObject()["guid"] == instance["mainComponent"])
                    master = n.toObject();
        QVERIFY(!master.isEmpty());
        // 此夹具只覆盖交叉轴尺寸，容器方向来自真实实例；完整展开和变量间距另行验证。
        // 逐条读取实例覆盖记录，提取本测试需要的根容器方向。
        for (const auto& value : instance["props"].toArray()) {
            const auto override = value.toObject();
            // 只把指向母版根的方向覆盖应用到根布局，子节点记录留给下面的合并。
            if (override["componentId"] == instance["mainComponent"])
                master["autoLayout"] = QJsonObject{{"stackMode", override["stackMode"]}};
        }
        QCOMPARE(master["autoLayout"].toObject()["stackMode"].toString(), QString("VERTICAL"));
        // 在内存副本中合并本测试所需的直接子节点覆盖，保留磁盘 canonical DSL。
        QJsonArray children;
        // 依次展开母版的直接子节点，在内存中形成带实例覆盖的布局夹具。
        for (const auto& value : master["childNode"].toArray()) {
            auto child = value.toObject();
            // 为当前子节点查找对应覆盖记录，保证不同节点的参数互不串用。
            for (const auto& overrideValue : instance["props"].toArray()) {
                const auto override = overrideValue.toObject();
                // 目标 GUID 不同就跳过，防止一个输入区域的参数覆盖另一个子区域。
                if (override["componentId"] != child["guid"])
                    continue;
                // 逐字段合并匹配到的记录；布局对象需保留未被覆盖的原有字段。
                for (auto it = override.begin(); it != override.end(); ++it) {
                    // 布局字段做成员级合并，其他字段直接覆盖，以模拟实例覆盖而保留母版默认值。
                    if (it.key() == "autoLayout") {
                        auto layout = child["autoLayout"].toObject();
                        const auto patch = it.value().toObject();
                        // 只替换补丁中出现的布局属性，使未声明的尺寸规则继续继承母版。
                        for (auto p = patch.begin(); p != patch.end(); ++p)
                            layout[p.key()] = p.value();
                        child["autoLayout"] = layout;
                    } else
                        child[it.key()] = it.value();
                }
            }
            children.append(child);
        }
        master["childNode"] = children;
        QWidget owner;
        owner.resize(320, 88);
        d2c::LayoutConverter converter(read("HtmlDsl/pipeline/mapping/layout-mapping.json"));
        auto* layout = converter.convert(master, &owner, [](const QJsonObject& n, QWidget* p) {
            return new MeasuredWidget(QSize(50, qRound(n["height"].toDouble())), p);
        });
        owner.show();
        QApplication::processEvents();
        layout->activate();
        QCOMPARE(layout->count(), 3);
        // 检查全部三个子节点均填满初始 320px 宽度，避免只测到单个控件。
        for (int i = 0; i < 3; ++i)
            QCOMPARE(layout->itemAt(i)->widget()->width(), 320);
        owner.resize(420, 88);
        QApplication::processEvents();
        layout->activate();
        // 窗口扩宽后再次检查全部子节点，证明 Fill 是持续布局约束而非初次设置的固定宽度。
        for (int i = 0; i < 3; ++i)
            QCOMPARE(layout->itemAt(i)->widget()->width(), 420);
    }

    // 故意提供不同的 sizeHint，确认 Fixed/Hug 最终按实例覆盖规则求值。
    void actualInstanceSizingOverrides() {
        const auto raw = read("HtmlDsl/pipeline/design-inputs/node-12-382-full/canonical.raw.json");
        const auto root = raw["pixTreeNodes"].toArray()[0].toObject();
        QWidget owner;
        owner.resize(720, 600);
        d2c::LayoutConverter converter(read("HtmlDsl/pipeline/mapping/layout-mapping.json"), raw);
        auto* layout = converter.convert(root, &owner, [](const QJsonObject& n, QWidget* parent) {
            auto* w = new MeasuredWidget(
                QSize(qRound(n["width"].toDouble()) + 7, qRound(n["height"].toDouble()) + 9),
                parent);
            w->setObjectName(n["guid"].toString());
            return w;
        });
        owner.show();
        QApplication::processEvents();
        layout->activate();
        const auto* checkbox = owner.findChild<QWidget*>("12:399");
        const auto* input = owner.findChild<QWidget*>("12:385");
        const auto* button = owner.findChild<QWidget*>("12:402");
        QVERIFY(checkbox && input && button);
        QCOMPARE(checkbox->width(), 127);
        QCOMPARE(checkbox->height(), 32);
        QCOMPARE(input->width(), 320);
        QCOMPARE(input->height(), 97);
        QCOMPARE(button->width(), 82);
        QCOMPARE(button->height(), 32);
    }

    void alignmentMatrix() {
        // 18 个受控组合覆盖方向及两轴对齐；这些数据是测试夹具，不属于 Pixso 采集证据。
        int count = 0;
        // 覆盖横向和纵向两种主轴，为后续两轴对齐组合建立方向条件。
        for (const auto& mode : {QString("HORIZONTAL"), QString("VERTICAL")})
            // 在每种方向下遍历主轴起始、居中和末尾对齐，检查整体占用区的偏移。
            for (const auto& primary :
                 {QString("flex-start"), QString("center"), QString("flex-end")})
                // 再遍历交叉轴三种对齐，组合为 18 个用例，检查不同高度或宽度控件的位置。
                for (const auto& counter :
                     {QString("flex-start"), QString("center"), QString("flex-end")}) {
                    const bool h = mode == "HORIZONTAL";
                    QJsonObject a{{"stackMode", mode},
                                  {"autoLayoutItemSpacing", 5},
                                  {"autoLayoutPaddingLeft", 7},
                                  {"autoLayoutPaddingTop", 11},
                                  {"autoLayoutPaddingRight", 13},
                                  {"autoLayoutPaddingBottom", 17},
                                  {"autoLayoutPrimaryAlign", primary},
                                  {"autoLayoutCounterAlign", counter}};
                    QJsonArray nodes{
                        QJsonObject{{"guid", "fixture:B"}, {"width", 30}, {"height", 20}},
                        QJsonObject{{"guid", "fixture:A"}, {"width", 20}, {"height", 10}}};
                    QWidget owner;
                    owner.resize(200, 100);
                    d2c::LayoutConverter converter(read("HtmlDsl/pipeline/mapping/layout-mapping.json"));
                    auto* layout = converter.convert({{"autoLayout", a}, {"childNode", nodes}},
                                                     &owner, [](const QJsonObject&, QWidget* p) {
                                                         return new QWidget(p);
                                                     });
                    owner.show();
                    QApplication::processEvents();
                    layout->activate();
                    const auto offset = [](const QString& alignment, double available,
                                           double occupied) {
                        return alignment == "center"     ? (available - occupied) / 2
                               : alignment == "flex-end" ? available - occupied
                                                         : 0;
                    };
                    const double origin = (h ? 7 : 11) + offset(primary, h ? 180 : 72, h ? 55 : 35);
                    // 逐项核对两个不同尺寸控件，防止对齐算法只对首项或等尺寸控件正确。
                    for (int i = 0; i < 2; ++i) {
                        auto* w = layout->itemAt(i)->widget();
                        const double expectedMain = origin + (i == 0 ? 0 : (h ? 25 : 15));
                        const double expectedCross =
                            (h ? 11 : 7) +
                            offset(counter, h ? 72 : 180, h ? w->height() : w->width());
                        QVERIFY2(qAbs((h ? w->x() : w->y()) - expectedMain) <= 1,
                                 qPrintable(mode + " " + primary + " " + counter + " main"));
                        QVERIFY2(qAbs((h ? w->y() : w->x()) - expectedCross) <= 1,
                                 qPrintable(mode + " " + primary + " " + counter + " cross"));
                    }
                    ++count;
                }
        QCOMPARE(count, 18);
    }

    // 隐藏项退出空间分配，后续可见控件从布局起点排列。
    void hiddenChildCollapses() {
        QWidget owner;
        owner.resize(200, 100);
        d2c::LayoutConverter converter(read("HtmlDsl/pipeline/mapping/layout-mapping.json"));
        const QJsonObject root{
            {"autoLayout", QJsonObject{{"stackMode", "HORIZONTAL"}, {"autoLayoutItemSpacing", 5}}},
            {"childNode",
             QJsonArray{
                 QJsonObject{{"guid", "B"}, {"width", 30}, {"height", 20}},
                 QJsonObject{{"guid", "A"}, {"width", 20}, {"height", 10}, {"visible", false}}}}};
        auto* layout = converter.convert(root, &owner, [](const QJsonObject&, QWidget* p) {
            return new QWidget(p);
        });
        owner.show();
        QApplication::processEvents();
        layout->activate();
        QVERIFY(layout->itemAt(0)->widget()->isHidden());
        QCOMPARE(layout->itemAt(1)->widget()->x(), 0);
        QVERIFY(layout->itemAt(1)->widget()->isVisible());
    }

    // 文本 Hug 宽度跟随 sizeHint，Fixed 高度保持设计约束。
    void hugUsesSizeHint() {
        QWidget owner;
        owner.resize(200, 100);
        d2c::LayoutConverter converter(read("HtmlDsl/pipeline/mapping/layout-mapping.json"));
        const QJsonObject child{
            {"guid", "Hug"},
            {"width", 1},
            {"height", 20},
            {"autoLayout",
             QJsonObject{{"stackMode", "HORIZONTAL"}, {"stackPrimarySizing", "RESIZE_TO_FIT"}}}};
        auto* layout = converter.convert({{"autoLayout", QJsonObject{{"stackMode", "HORIZONTAL"}}},
                                          {"childNode", QJsonArray{child}}},
                                         &owner, [](const QJsonObject&, QWidget* p) {
                                             return new QLabel("Hug content", p);
                                         });
        owner.show();
        QApplication::processEvents();
        layout->activate();
        auto* label = layout->itemAt(0)->widget();
        QCOMPARE(label->width(), label->sizeHint().width());
        QCOMPARE(label->height(), 20);
    }

    // Grid 的行列与跨度必须直接进入 QGridLayout，不能被转换成线性插入顺序。
    void gridPlacementAndStretch() {
        QWidget owner;
        owner.resize(260, 140);
        const QJsonObject root{
            {"autoLayout", QJsonObject{{"stackMode", "GRID"},
                                        {"autoLayoutItemSpacing", 6},
                                        {"autoLayoutPaddingLeft", 4},
                                        {"autoLayoutPaddingTop", 5},
                                        {"autoLayoutPaddingRight", 6},
                                        {"autoLayoutPaddingBottom", 7},
                                        {"gridColumnGap", 12},
                                        {"gridRowGap", 8},
                                        {"gridRowStretch", QJsonArray{1, 2}},
                                        {"gridColumnStretch", QJsonArray{3, 1}}}},
            {"childNode", QJsonArray{
                QJsonObject{{"guid", "grid:B"}, {"width", 40}, {"height", 20},
                             {"layoutItem", QJsonObject{{"gridRow", 1}, {"gridColumn", 0},
                                                          {"gridColumnSpan", 2}}}},
                QJsonObject{{"guid", "grid:A"}, {"width", 30}, {"height", 20},
                             {"layoutItem", QJsonObject{{"gridRow", 0}, {"gridColumn", 0}}}}}}};
        d2c::LayoutConverter converter(read("HtmlDsl/pipeline/mapping/layout-mapping.json"));
        auto* layout = converter.convert(root, &owner, [](const QJsonObject& node, QWidget* parent) {
            auto* widget = new MeasuredWidget(QSize(node["width"].toInt(), node["height"].toInt()), parent);
            widget->setObjectName(node["guid"].toString());
            return widget;
        });
        auto* grid = qobject_cast<QGridLayout*>(layout);
        QVERIFY(grid);
        QCOMPARE(grid->horizontalSpacing(), 12);
        QCOMPARE(grid->verticalSpacing(), 8);
        QCOMPARE(grid->rowStretch(0), 1);
        QCOMPARE(grid->rowStretch(1), 2);
        QCOMPARE(grid->columnStretch(0), 3);
        QCOMPARE(grid->columnStretch(1), 1);
        int row = -1, column = -1, rowSpan = -1, columnSpan = -1;
        grid->getItemPosition(0, &row, &column, &rowSpan, &columnSpan);
        QCOMPARE(row, 0);
        QCOMPARE(column, 0);
        QCOMPARE(rowSpan, 1);
        QCOMPARE(columnSpan, 1);
        grid->getItemPosition(1, &row, &column, &rowSpan, &columnSpan);
        QCOMPARE(row, 1);
        QCOMPARE(column, 0);
        QCOMPARE(rowSpan, 1);
        QCOMPARE(columnSpan, 2);
        QCOMPARE(converter.trace().first().toObject()["items"].toArray().size(), 2);
    }

    // Form 的 Label/Field/Spanning 是独立角色，不能复用 Grid 的任意列跨度语义。
    void formRolesAndPolicies() {
        QWidget owner;
        owner.resize(320, 160);
        const QJsonObject root{
            {"autoLayout", QJsonObject{{"stackMode", "FORM"},
                                        {"autoLayoutItemSpacing", 5},
                                        {"autoLayoutPaddingLeft", 3},
                                        {"autoLayoutPaddingTop", 4},
                                        {"autoLayoutPaddingRight", 5},
                                        {"autoLayoutPaddingBottom", 6},
                                        {"formFieldGrowthPolicy", "allNonFixedFieldsGrow"},
                                        {"formRowWrapPolicy", "dontWrapRows"}}},
            {"childNode", QJsonArray{
                QJsonObject{{"guid", "form:field"}, {"width", 180}, {"height", 28},
                             {"layoutItem", QJsonObject{{"formRow", 0}, {"formRole", "field"}}}},
                QJsonObject{{"guid", "form:label"}, {"width", 70}, {"height", 28},
                             {"layoutItem", QJsonObject{{"formRow", 0}, {"formRole", "label"}}}}}}};
        d2c::LayoutConverter converter(read("HtmlDsl/pipeline/mapping/layout-mapping.json"));
        auto* layout = converter.convert(root, &owner, [](const QJsonObject& node, QWidget* parent) {
            auto* widget = new MeasuredWidget(QSize(node["width"].toInt(), node["height"].toInt()), parent);
            widget->setObjectName(node["guid"].toString());
            return widget;
        });
        auto* form = qobject_cast<QFormLayout*>(layout);
        QVERIFY(form);
        QCOMPARE(form->fieldGrowthPolicy(), QFormLayout::AllNonFixedFieldsGrow);
        QCOMPARE(form->rowWrapPolicy(), QFormLayout::DontWrapRows);
        QCOMPARE(form->itemAt(0, QFormLayout::LabelRole)->widget()->objectName(), QString("form:label"));
        QCOMPARE(form->itemAt(0, QFormLayout::FieldRole)->widget()->objectName(), QString("form:field"));
        QCOMPARE(converter.trace().first().toObject()["items"].toArray().size(), 2);
    }

    // 负数、字符串尺寸、缺失固定尺寸、绝对定位和未知模式均应明确失败。
    void invalidFieldsAreRejected() {
        const auto rules = read("HtmlDsl/pipeline/mapping/layout-mapping.json");
        const QJsonObject validChild{{"guid", "invalid-fixture"}, {"width", 20}, {"height", 10}};
        const auto rejects = [&](QJsonObject root, QJsonObject mapping, const char* expected) {
            QWidget owner;
            d2c::LayoutConverter converter(mapping);
            try {
                converter.convert(root, &owner, [](const QJsonObject&, QWidget* p) {
                    return new QWidget(p);
                });
            } catch (const std::runtime_error& error) {
                QVERIFY2(QString::fromUtf8(error.what()).startsWith(expected), error.what());
                return;
            }
            QFAIL(expected);
        };
        const QJsonObject valid{{"autoLayout", QJsonObject{{"stackMode", "HORIZONTAL"}}},
                                {"childNode", QJsonArray{validChild}}};
        // 分别输入负数、字符串和超大数，确认非法间距由类型与范围校验明确拒绝。
        for (const auto& value : {QJsonValue(-1), QJsonValue("5"), QJsonValue(1e30)}) {
            auto n = valid;
            auto a = n["autoLayout"].toObject();
            a["autoLayoutItemSpacing"] = value;
            n["autoLayout"] = a;
            rejects(n, rules, "INVALID_LAYOUT_METRIC");
        }
        auto child = validChild;
        child["visible"] = "false";
        auto n = valid;
        n["childNode"] = QJsonArray{child};
        rejects(n, rules, "INVALID_VISIBILITY");
        child = validChild;
        child.remove("width");
        n["childNode"] = QJsonArray{child};
        rejects(n, rules, "MISSING_FIXED_SIZE");
        child = validChild;
        child["autoLayout"] = QJsonObject{{"autoLayoutItemAbsolutePos", true}};
        n["childNode"] = QJsonArray{child};
        rejects(n, rules, "UNMAPPED_ABSOLUTE_CHILD");
        child = validChild;
        child["autoLayout"] = QJsonObject{{"autoLayoutWidthResize", "0"}};
        n["childNode"] = QJsonArray{child};
        rejects(n, rules, "UNVERIFIED_RESIZE_MODE");
        auto bad = rules;
        bad["modes"] = QJsonObject{{"HORIZONTAL", "QAnything"}};
        rejects(valid, bad, "UNMAPPED_LAYOUT_MODE");
        const QJsonObject gridChildA{{"guid", "grid-a"}, {"width", 20}, {"height", 10},
                                     {"layoutItem", QJsonObject{{"gridRow", 0}, {"gridColumn", 0}}}};
        const QJsonObject gridChildB{{"guid", "grid-b"}, {"width", 20}, {"height", 10},
                                     {"layoutItem", QJsonObject{{"gridRow", 0}, {"gridColumn", 0}}}};
        rejects({{"autoLayout", QJsonObject{{"stackMode", "GRID"}}},
                 {"childNode", QJsonArray{gridChildA, gridChildB}}},
                rules, "GRID_CELL_OVERLAP");
        rejects({{"autoLayout", QJsonObject{{"stackMode", "GRID"}}},
                 {"childNode", QJsonArray{validChild}}},
                rules, "MISSING_GRID_PLACEMENT");
        const QJsonObject formChildA{{"guid", "form-a"}, {"width", 20}, {"height", 10},
                                     {"layoutItem", QJsonObject{{"formRow", 0}, {"formRole", "label"}}}};
        const QJsonObject formChildB{{"guid", "form-b"}, {"width", 20}, {"height", 10},
                                     {"layoutItem", QJsonObject{{"formRow", 0}, {"formRole", "label"}}}};
        rejects({{"autoLayout", QJsonObject{{"stackMode", "FORM"}}},
                 {"childNode", QJsonArray{formChildA, formChildB}}},
                rules, "DUPLICATE_FORM_ROLE");
    }

    // 分别使用真实纵向页面和真实横向母版验证几何，而非由转换器反推期望值。
    void vertical() {
        verify(read("HtmlDsl/pipeline/design-inputs/node-12-382-full/canonical.raw.json")["pixTreeNodes"]
                   .toArray()[0]
                   .toObject());
    }

    void horizontal() {
        auto raw = read("HtmlDsl/pipeline/design-inputs/node-12-382-full/canonical.raw.json");
        // 从真实组件集中定位横向样本，用独立的设计坐标验证转换结果。
        for (const auto& s : raw["pixComponentTreeDslNodes"].toArray())
            // 遍历各母版定义，以 GUID 查找横向布局参照，避免依赖数组位置。
            for (const auto& v : s.toObject()["childNode"].toArray())
                // 找到指定样本后执行几何检查并返回；未找到时由末尾断言报告夹具缺失。
                if (v.toObject()["guid"] == "3:6764") {
                    verify(v.toObject());
                    return;
                }
        QFAIL("Missing real horizontal sample");
    }
};
QTEST_MAIN(LayoutTests)
#include "LayoutTests.moc"
