#include PAGE_HEADER
#include <MyDesignSystem/ComponentTheme.h>
#include <MyDesignSystem/Z_DesignComponents.h>
#include <MyDesignSystem/Z_PushButton.h>
#include <MyDesignSystem/native/Z_TableView.h>
#include <QApplication>
#include <QAbstractButton>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPixmap>
#include <QSvgRenderer>
#include <QPainter>
#include <iostream>
#include <stdexcept>
#include "generated_page_snapshot.h"

// Read-only runtime evidence. Production generation must not depend on private
// child object names or mutate implementation anatomy through findChildren().
static QJsonObject contentSnapshot(QWidget& widget) {
    QJsonArray labels, buttons;
    for (auto label : widget.findChildren<QLabel*>())
        labels.append(QJsonObject{{"objectName", label->objectName()}, {"text", label->text()}});
    for (auto button : widget.findChildren<QAbstractButton*>())
        buttons.append(QJsonObject{{"objectName", button->objectName()},
            {"text", button->text()}, {"hasIcon", !button->icon().isNull()},
            {"checked", button->isChecked()}});
    return QJsonObject{{"labels", labels}, {"buttons", buttons}};
}

// Probe only: exercise QWidget's public resize contract without overriding SDK
// min/max constraints, token styles, or child geometry. Not a source renderer.
static QJsonObject geometrySnapshot(QWidget& widget, const QSize& requested) {
    auto size = [](const QSize& value) {
        return QJsonObject{{"width", value.width()}, {"height", value.height()}};
    };
    widget.ensurePolished();
    widget.resize(requested);
    widget.show();
    QApplication::processEvents();
    if (widget.layout()) widget.layout()->activate();
    QJsonObject result{{"requested", size(requested)}, {"actual", size(widget.size())},
        {"sizeHint", size(widget.sizeHint())}, {"minimumSize", size(widget.minimumSize())},
        {"maximumSize", size(widget.maximumSize())},
        {"minimumSizeHint", size(widget.minimumSizeHint())},
        {"requestHonored", widget.size() == requested},
        {"scope", "SDK resize constraints only; no visual equivalence claim"}};
    widget.hide();
    return result;
}

int main(int argc, char** argv) {
    QApplication app(argc, argv);
    try {
        if (QString::fromLatin1(qVersion()) != QStringLiteral("5.15.2"))
            throw std::runtime_error("QT5152_REQUIRED");
        component_theme::install(app, QStringLiteral("standard-light"));
        PAGE_TYPE page;
        page.setAttribute(Qt::WA_DontShowOnScreen, true);
        if (argc == 4 && QString::fromLocal8Bit(argv[2]) == QStringLiteral("--page-only")) {
            const auto result = generatedPageSnapshot(page, QString::fromLocal8Bit(argv[3]));
            if (!page.grab().save(QString::fromLocal8Bit(argv[1])))
                throw std::runtime_error("SCREENSHOT_SAVE_FAILED");
            std::cout << QJsonDocument(result).toJson(QJsonDocument::Compact).constData() << std::endl;
            return 0;
        }
        page.resize(640, 480);
        page.show();
        app.processEvents();
        if (page.styleSheet().contains(QStringLiteral("rgba(17,34,51,255)"))) {
            const auto surface = page.grab().toImage();
            if (surface.pixelColor(0, 0) != QColor(17, 34, 51))
                throw std::runtime_error("PAGE_SURFACE_COLOR_FAILED");
        }
        if (page.property("d2cOverflow").toString() == QStringLiteral("hidden")) {
            QWidget protruding(&page);
            protruding.setGeometry(page.width() - 10, 10, 30, 20);
            protruding.show();
            app.processEvents();
            if (protruding.visibleRegion().boundingRect() != QRect(0, 0, 10, 20))
                throw std::runtime_error("PARENT_RECTANGLE_CLIP_FAILED");
        }
        if (auto middle = page.findChild<QWidget*>(QStringLiteral("middle"))) {
            auto owner = middle->parentWidget();
            if (!owner || !owner->layout())
                throw std::runtime_error("PERCENTAGE_PROBE_LAYOUT_MISSING");
            const auto margins = owner->layout()->contentsMargins();
            if (middle->width() != 320 ||
                middle->height() != owner->height() - margins.top() - margins.bottom())
                throw std::runtime_error("PERCENTAGE_CROSS_AXIS_GEOMETRY_FAILED");
            if (auto growth = page.findChild<QWidget*>(QStringLiteral("growth"))) {
                const int available = owner->width() - margins.left() - margins.right()
                    - middle->width() - owner->layout()->spacing();
                if (growth->width() != available)
                    throw std::runtime_error("SINGLE_GROW_GEOMETRY_FAILED");
                if (page.maximumWidth() > 640) {
                    page.resize(800, 480);
                    app.processEvents();
                    const int resizedAvailable = owner->width() - margins.left() - margins.right()
                        - middle->width() - owner->layout()->spacing();
                    if (page.width() != 800 || growth->width() != resizedAvailable)
                        throw std::runtime_error("ROOT_PERCENTAGE_RESIZE_FAILED");
                    if (resizedAvailable != available + 160)
                        throw std::runtime_error("PERCENTAGE_CHAIN_DID_NOT_RESIZE");
                    page.resize(640, 480);
                    app.processEvents();
                }
            }
        }
        if (auto generatedButton = page.findChild<commonWidget::Z_PushButton*>(QStringLiteral("button"))) {
            if (generatedButton->text() != QStringLiteral("Constructor probe"))
                throw std::runtime_error("GENERATED_BUTTON_INITIALIZATION_FAILED");
        }
        if (auto generatedInput = page.findChild<designer_widgets::InputField*>(QStringLiteral("input"))) {
            if (generatedInput->value() != QStringLiteral("probe value"))
                throw std::runtime_error("GENERATED_INPUT_INITIALIZATION_FAILED");
        }
        bool tableModelChecked = false;
        if (auto generatedTable = page.findChild<commonWidget::Z_TableView*>(QStringLiteral("table"))) {
            if (auto model = generatedTable->model()) {
                if (model->rowCount() != 3 || model->columnCount() != 3
                    || model->headerData(0, Qt::Horizontal).toString() != QStringLiteral("Name")
                    || model->data(model->index(0, 0)).toString() != QStringLiteral("Alpha")
                    || model->data(model->index(2, 2)).toString() != QStringLiteral("32")
                    || model->flags(model->index(0, 0)).testFlag(Qt::ItemIsEditable))
                    throw std::runtime_error("GENERATED_TABLE_SAMPLE_READBACK_FAILED");
                tableModelChecked = true;
            }
        }

        // Public API smoke checks, NOT proof of design-to-component equivalence.
        commonWidget::Z_PushButton button(&page);
        button.setIntent(commonWidget::ButtonIntent::Primary);
        button.setAppearance(commonWidget::ButtonAppearance::Filled);
        button.setSize(commonWidget::ButtonSize::Small);
        button.setText(QStringLiteral("API probe"));
        button.setIconOnly(false);
        button.setEnabled(true);
        button.setLoading(true);
        if (button.visualState() != QStringLiteral("loading"))
            throw std::runtime_error("BUTTON_LOADING_STATE_FAILED");
        button.setLoading(false);
        button.setEnabled(false);
        if (button.isEnabled() || button.visualState() != QStringLiteral("disabled"))
            throw std::runtime_error("BUTTON_DISABLED_STATE_FAILED");
        button.setEnabled(true);
        designer_widgets::InputField input(&page);
        input.setPlaceholderText(QStringLiteral("Input probe"));
        input.setValue(QStringLiteral("value"));
        designer_widgets::InputField sourceInput(&page);
        sourceInput.setValue(QStringLiteral("Entered value"));
        sourceInput.setControlSize(QStringLiteral("lg"));
        sourceInput.setValue(QString());
        sourceInput.setEnabled(true);
        sourceInput.setReadOnly(false);
        sourceInput.setPlaceholderText(QStringLiteral("Search workspace"));
        if (!sourceInput.value().isEmpty() || !sourceInput.lineEdit()
            || !sourceInput.lineEdit()->text().isEmpty()
            || sourceInput.lineEdit()->placeholderText() != QStringLiteral("Search workspace")
            || sourceInput.lineEdit()->isReadOnly() || !sourceInput.isEnabled())
            throw std::runtime_error("INPUT_DEFAULT_PLACEHOLDER_CONTRACT_FAILED");
        commonWidget::Z_Navbar navbar(&page);
        bool navbarSlotChecked = false;
        if (auto generatedNavbar = page.findChild<commonWidget::Z_Navbar*>(QStringLiteral("navbar"))) {
            auto search = generatedNavbar->searchInput();
            if (!search || !search->lineEdit())
                throw std::runtime_error("NAVBAR_PUBLIC_SEARCH_UNAVAILABLE");
            if (search->lineEdit()->placeholderText() == QStringLiteral("Search workspace"))
                navbarSlotChecked = true;
        }
        QJsonObject defaultNavbar = contentSnapshot(navbar);
        defaultNavbar.insert("profileName", navbar.profileName());
        defaultNavbar.insert("searchText", navbar.searchText());
        bool layoutAxisRejected = false;
        try {
            navbar.setVariantProperty(QStringLiteral("Layout"), QStringLiteral("AppHeader"));
        } catch (const std::exception& error) {
            if (QString::fromUtf8(error.what()) !=
                    QStringLiteral("Navbar supports only the Density variant axis"))
                throw;
            layoutAxisRejected = true;
        }
        if (!layoutAxisRejected)
            throw std::runtime_error("NAVBAR_LAYOUT_CONTRACT_CHANGED_REVIEW_MAPPING");
        defaultNavbar.insert("layoutAxisSupported", false);
        navbar.setDensity(QStringLiteral("Comfortable"));
        defaultNavbar.insert("sourceSizeProbe", geometrySnapshot(navbar, QSize(1200, 63)));
        navbar.setProfileName(QStringLiteral("API probe"));
        commonWidget::Z_Sidebar sidebar(&page);
        QJsonObject defaultSidebar = contentSnapshot(sidebar);
        defaultSidebar.insert("routeIds", QJsonArray::fromStringList(sidebar.routeIds()));
        defaultSidebar.insert("routeLabels", QJsonArray::fromStringList(sidebar.routeLabels()));
        defaultSidebar.insert("activeRoute", sidebar.activeRoute());
        sidebar.setDensity(QStringLiteral("Comfortable"));
        sidebar.setCollapsed(false);
        defaultSidebar.insert("sourceSizeProbe", geometrySnapshot(sidebar, QSize(280, 520)));
        QJsonObject sidebarContract;
        for (const QString& value : {QStringLiteral("Closed"), QStringLiteral("Open")}) {
            bool rejected = false;
            try {
                sidebar.setVariantProperty(QStringLiteral("Flyout"), value);
                sidebarContract.insert(value, QJsonObject{
                    {"accepted", true},
                    {"variantProperties", QJsonObject::fromVariantMap(sidebar.variantProperties())}});
            } catch (const std::exception& error) {
                if (QString::fromUtf8(error.what()) !=
                        QStringLiteral("Sidebar supports only Density and Collapsed variant axes"))
                    throw;
                rejected = true;
                sidebarContract.insert(value, QJsonObject{
                    {"accepted", false}, {"error", QString::fromUtf8(error.what())}});
            }
            if (!rejected)
                throw std::runtime_error("SIDEBAR_FLYOUT_CONTRACT_CHANGED_REVIEW_MAPPING");
        }
        defaultSidebar.insert("flyoutAxisProbe", sidebarContract);
        if (sidebar.density() != QStringLiteral("Comfortable") || sidebar.isCollapsed())
            throw std::runtime_error("SIDEBAR_INITIAL_STATE_READBACK_FAILED");
        sidebar.setRouteIds({QStringLiteral("probe")});
        sidebar.setRouteLabels({QStringLiteral("Probe")});
        sidebar.setActiveRoute(QStringLiteral("probe"));
        commonWidget::Z_DesktopBottomNavigation navigation(&page);
        QJsonObject defaultNavigation = contentSnapshot(navigation);
        defaultNavigation.insert("primaryItems", QJsonArray::fromStringList(navigation.primaryItems()));
        defaultNavigation.insert("secondaryItems", QJsonArray::fromStringList(navigation.secondaryItems()));
        defaultNavigation.insert("currentIndex", navigation.currentIndex());
        defaultNavigation.insert("secondaryIndex", navigation.secondaryIndex());
        navigation.setPrimaryItems({QStringLiteral("Probe")});
        navigation.setSecondaryItems({QStringLiteral("Secondary")});
        commonWidget::Z_TableView table(&page);
        table.setDensity(QStringLiteral("compact"));
        if (button.text() != QStringLiteral("API probe")
            || input.value() != QStringLiteral("value")
            || navbar.profileName() != QStringLiteral("API probe")
            || sidebar.activeRoute() != QStringLiteral("probe"))
            throw std::runtime_error("PUBLIC_API_READBACK_FAILED");
        if (argc > 1 && !page.grab().save(QString::fromLocal8Bit(argv[1])))
            throw std::runtime_error("SCREENSHOT_SAVE_FAILED");
        QJsonObject result{
            {"status", "PASS"},
            {"scope", "generated page construction and SDK public API smoke only"},
            {"qtVersion", qVersion()},
            {"profile", component_theme::activeProfile()},
            {"pageStylesheetLoaded", !page.styleSheet().isEmpty()},
            {"tableSampleModelReadback", tableModelChecked},
            {"navbarSearchPlaceholderReadback", navbarSlotChecked},
            {"inputDefaultProbe", QJsonObject{{"value", sourceInput.value()},
                {"placeholder", sourceInput.placeholderText()},
                {"scope", "Large/Default text initialization only; icons and visual parity unverified"}}},
            {"sdkDefaultContent", QJsonObject{{"navbar", defaultNavbar},
                {"sidebar", defaultSidebar}, {"bottomNavigation", defaultNavigation}}},
            {"visualParity", "NOT_RUN"}
        };
        if (argc > 3) {
            QSvgRenderer renderer(QString::fromLocal8Bit(argv[2]));
            if (!renderer.isValid())
                throw std::runtime_error("VECTOR_SVG_LOAD_FAILED");
            QImage image(256, 256, QImage::Format_ARGB32_Premultiplied);
            image.fill(Qt::white);
            QPainter painter(&image);
            renderer.render(&painter);
            painter.end();
            int painted = 0;
            for (int y = 0; y < image.height(); ++y)
                for (int x = 0; x < image.width(); ++x)
                    painted += image.pixelColor(x, y) != QColor(Qt::white);
            if (!painted || !image.save(QString::fromLocal8Bit(argv[3])))
                throw std::runtime_error("VECTOR_SVG_RENDER_FAILED");
            result.insert("vectorProbe", QJsonObject{{"paintedPixels", painted},
                {"scope", "Qt SVG load and nonempty rendering only; slot placement not validated"}});
        }
        std::cout << QJsonDocument(result).toJson(QJsonDocument::Compact).constData() << std::endl;
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << std::endl;
        return 1;
    }
}
