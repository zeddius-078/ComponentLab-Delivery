#include "DemoWindow.h"
#include "DemoRuntime.h"
#include "ButtonAppearanceAdapter.h"
#include "Z_InputField.h"
#include "Z_Select.h"
#include "Z_CheckBox.h"
#include "SolidInnerShadow.h"
#include "FocusIndicator.h"
#include "Z_OtpInput.h"
#include <QApplication>
#include <QCheckBox>
#include <QComboBox>
#include <QDir>
#include <QDialog>
#include <QLabel>
#include <QLineEdit>
#include <QListView>
#include <QListWidget>
#include <QPainter>
#include <QRadioButton>
#include <QSet>
#include <QPushButton>
#include <QSignalSpy>
#include <QStackedWidget>
#include <QStyleFactory>
#include <QStyledItemDelegate>
#include <QTabWidget>
#include <QTableWidget>
#include <QTextBrowser>
#include <QTreeWidget>
#include <QTest>
#include <QVBoxLayout>

using namespace commonWidget;
class CompositionTests : public QObject {
    Q_OBJECT
    QString profile, evidence;
    std::shared_ptr<const field::Tokens> tokens;
    std::unique_ptr<DemoWindow> window;
    template <class T> T* part(const char* name) {
        return window->findChild<T*>(name);
    }
    void edit(const char* name, const QString& value) {
        auto* editor = part<QLineEdit>(name);
        editor->setText(value);
        QMetaObject::invokeMethod(editor, "editingFinished");
    }
  private slots:
    void initTestCase() {
        profile = qEnvironmentVariable("DEMO_PROFILE", "light-comfortable");
        evidence = qEnvironmentVariable("DEMO_EVIDENCE");
        if (!evidence.isEmpty())
            QVERIFY(QDir().mkpath(evidence));
        tokens = std::make_shared<field::Tokens>(profile);
        window = std::make_unique<DemoWindow>(profile);
        window->show();
        QTest::qWait(60);
    }
    void navigationContract() {
        auto* tabs = part<QTabWidget>("tabs");
        QVERIFY(tabs);
        QCOMPARE(tabs->count(), 1);
        QCOMPARE(tabs->tabText(0), QString("Z-DesignSystem"));
        const QStringList sections{"buttonSections", "checkboxSections", "inputSections", "selectSections"};
        for (const auto& name : sections) {
            auto* sub = part<QTabWidget>(name.toLatin1().constData());
            QVERIFY(sub);
            QCOMPARE(sub->count(), 2);
            QCOMPARE(sub->tabText(0), QString::fromUtf8("外观"));
            QCOMPARE(sub->tabText(1), QString::fromUtf8("分组与状态"));
            for (int j = 0; j < 2; ++j)
                QVERIFY(!sub->widget(j)->findChildren<QWidget*>().isEmpty());
        }
        auto* nativeSections = tabs->widget(0)->findChild<QTabWidget*>("nativeSections");
        QVERIFY(nativeSections);
        QCOMPARE(nativeSections->count(), 2);
        QCOMPARE(nativeSections->tabText(0), QString::fromUtf8("组件图库"));
        QCOMPARE(nativeSections->tabText(1), QString::fromUtf8("组件映射"));
        auto* gallery = part<QWidget>("nativeGalleryPage");
        auto* categories = part<QTreeWidget>("componentTree");
        auto* pages = part<QStackedWidget>("componentPages");
        auto* registry = part<QTableWidget>("nativeRegistryTable");
        QVERIFY(gallery && categories && pages && registry);
        QCOMPARE(gallery->property("descriptorCount").toInt(), 59);
        QCOMPARE(gallery->property("componentCount").toInt(), 62);
        QCOMPARE(categories->topLevelItemCount(), 6);
        QCOMPARE(categories->topLevelItem(0)->text(0).section(" (", 0, 0),
                 QString::fromUtf8("基础与按钮"));
        const QStringList firstComponents{"commonWidget::Z_PushButton",
                                          "commonWidget::Z_CheckBox",
                                          "commonWidget::Z_InputField",
                                          "commonWidget::Z_Select"};
        for (int i = 0; i < firstComponents.size(); ++i)
            QCOMPARE(categories->topLevelItem(0)->child(i)->data(0, Qt::UserRole).toString(),
                     firstComponents[i]);
        QCOMPARE(registry->rowCount(), 62);
        int created = 0;
        QSet<QString> seen;
        for (int group = 0; group < categories->topLevelItemCount(); ++group) {
            auto* branch = categories->topLevelItem(group);
            QVERIFY(branch->childCount() > 0);
            for (int row = 0; row < branch->childCount(); ++row) {
                auto* leaf = branch->child(row);
                categories->setCurrentItem(leaf);
                QCoreApplication::processEvents();
                QCOMPARE(gallery->property("activeComponent").toString(),
                         leaf->data(0, Qt::UserRole).toString());
                QVERIFY(pages->currentWidget());
                QCOMPARE(gallery->property("visibleDescriptorCount").toInt(), 1);
                const QString sdkClass = leaf->data(0, Qt::UserRole).toString();
                QVERIFY(sdkClass.startsWith("commonWidget::Z_"));
                QVERIFY(!seen.contains(sdkClass));
                seen.insert(sdkClass);
                if (sdkClass != "commonWidget::Z_PushButton" &&
                    sdkClass != "commonWidget::Z_CheckBox" &&
                    sdkClass != "commonWidget::Z_InputField" &&
                    sdkClass != "commonWidget::Z_Select") {
                    auto* detail = pages->currentWidget();
                    auto* detailSections = detail->findChild<QTabWidget*>("componentSections");
                    auto* objectTree = detail->findChild<QTreeWidget*>("componentObjectTree");
                    auto* stateTree = detail->findChild<QTreeWidget*>("componentStateObjectTree");
                    auto* stateChoice = detail->findChild<QComboBox*>("componentStateControl");
                    auto* enabled = detail->findChild<QCheckBox*>("componentEnabledControl");
                    QVERIFY(detailSections && objectTree && stateTree && stateChoice && enabled);
                    QCOMPARE(detailSections->count(), 2);
                    QCOMPARE(detailSections->tabText(0), QString::fromUtf8("外观"));
                    QCOMPARE(detailSections->tabText(1), QString::fromUtf8("分组与状态"));
                    QCOMPARE(objectTree->topLevelItemCount(), 1);
                    QCOMPARE(stateTree->topLevelItemCount(), 1);
                    QVERIFY(objectTree->topLevelItem(0)->childCount() > 0);
                    QVERIFY(!objectTree->findItems("enabled", Qt::MatchExactly | Qt::MatchRecursive)
                                 .isEmpty());
                    QVERIFY(!stateTree->findItems("enabled", Qt::MatchExactly | Qt::MatchRecursive)
                                 .isEmpty());
                    QWidget* preview = nullptr;
                    for (auto* widget : detail->findChildren<QWidget*>()) {
                        if (widget->objectName().startsWith("componentRoot_Z_")) {
                            preview = widget;
                            break;
                        }
                    }
                    QVERIFY(preview);
                    QCOMPARE(objectTree->topLevelItem(0)->text(0), preview->objectName());
                    enabled->setChecked(false);
                    QVERIFY(!preview->isEnabled());
                    enabled->setChecked(true);
                    QVERIFY(preview->isEnabled());
                    auto* statePreview = detail->findChild<QWidget*>(
                        "statePreview_" + preview->objectName().mid(QString("componentRoot_").size()));
                    QVERIFY(statePreview);
                    stateChoice->setCurrentIndex(1);
                    QVERIFY(!statePreview->isEnabled());
                    stateChoice->setCurrentIndex(0);
                    QVERIFY(statePreview->isEnabled());
                }
                ++created;
            }
        }
        QCOMPARE(created, 62);
        QCOMPARE(seen.size(), 62);
        QCOMPARE(pages->count(), 62);
        for (const auto& name : sections) {
            auto* detail = part<QTabWidget>(name.toLatin1().constData());
            auto* objectTree = detail->parentWidget()->findChild<QTreeWidget*>();
            QVERIFY(objectTree);
            QVERIFY(objectTree->topLevelItemCount() > 0);
            QVERIFY(!objectTree->findItems("enabled", Qt::MatchExactly | Qt::MatchRecursive)
                         .isEmpty());
        }
        QTreeWidgetItem* wizardItem = nullptr;
        for (int group = 0; group < categories->topLevelItemCount(); ++group) {
            auto* branch = categories->topLevelItem(group);
            for (int row = 0; row < branch->childCount(); ++row) {
                if (branch->child(row)->data(0, Qt::UserRole).toString() ==
                    "commonWidget::Z_Wizard")
                    wizardItem = branch->child(row);
            }
        }
        QVERIFY(wizardItem);
        categories->setCurrentItem(wizardItem);
        auto* open = pages->currentWidget()->findChild<QPushButton*>("componentOpenDialog");
        auto* dialog = pages->currentWidget()->findChild<QDialog*>("componentRoot_Z_Wizard");
        QVERIFY(open && dialog);
        QTest::mouseClick(open, Qt::LeftButton);
        QCoreApplication::processEvents();
        QVERIFY(dialog->isVisible());
        dialog->close();
        categories->setCurrentItem(categories->topLevelItem(0)->child(0));
        QCoreApplication::processEvents();
        QVERIFY(!window->findChild<QWidget*>("mappingTab"));
        QVERIFY(!window->findChild<QWidget*>("mappingImplementationGuide"));
    }
    void cleanup() {
        FocusIndicator::instance().setMode(FocusIndicator::Mode::Keyboard);
        window->activateWindow();
    }
    void focusOptionAndTokenDefaults() {
        auto* choice = part<QComboBox>("focusIndicatorCombo");
        QVERIFY(choice);
        QCOMPARE(choice->count(), 3);
        QCOMPARE(choice->currentData().toString(), QString("keyboard"));
        QCOMPARE(FocusIndicator::instance().opacity(), 0.55);
        for (const auto& mode : {"off", "always", "keyboard"}) {
            choice->setCurrentIndex(choice->findData(mode));
            QCOMPARE(FocusIndicator::modeName(FocusIndicator::instance().mode()), QString(mode));
        }
        QVERIFY_EXCEPTION_THROWN(FocusIndicator::parseMode("unknown"), std::invalid_argument);
    }
    void mouseFocusDoesNotOverrideDangerBorder() {
        auto* b = part<Z_PushButton>("BUTTON_ROOT");
        part<Z_CheckBox>("buttonDanger")->setChecked(true);
        window->activateWindow();
        QTest::mouseClick(b, Qt::LeftButton);
        QTRY_VERIFY(b->hasFocus());
        QTest::qWait(30);
        const auto qss = b->styleSheet();
        QVERIFY(!b->property("focusIndicatorVisible").toBool());
        part<Z_CheckBox>("buttonDanger")->setChecked(false);
        QVERIFY2(!qss.contains(":focus"), qPrintable(qss));
    }
    void focusModeSwitchPreservesKeyboardAndGeometry() {
        QWidget host;
        auto* layout = new QVBoxLayout(&host);
        auto* first = new Z_PushButton;
        auto* second = new Z_PushButton;
        first->setText("First"); second->setText("Second");
        layout->addWidget(first); layout->addWidget(second);
        QWidget::setTabOrder(first, second);
        host.show(); host.activateWindow();
        QTest::mouseClick(first, Qt::LeftButton);
        QTRY_VERIFY(first->hasFocus());
        QVERIFY(!first->property("focusIndicatorVisible").toBool());
        QTest::keyClick(first, Qt::Key_Tab);
        QTRY_VERIFY(second->hasFocus());
        QVERIFY(second->property("focusIndicatorVisible").toBool());
        const auto before = second->size();
        const auto withFocus = second->grab().toImage();
        FocusIndicator::instance().setMode(FocusIndicator::Mode::Off);
        QVERIFY(second->hasFocus());
        QVERIFY(!second->property("focusIndicatorVisible").toBool());
        QCOMPARE(second->size(), before);
        QVERIFY(second->grab().toImage() != withFocus);
        QSignalSpy clicks(second, &QPushButton::clicked);
        QTest::keyClick(second, Qt::Key_Space);
        QCOMPARE(clicks.count(), 1);
        QVERIFY(!second->property("focusIndicatorVisible").toBool());
        QTest::keyClick(second, Qt::Key_Backtab);
        QTRY_VERIFY(first->hasFocus());
        QVERIFY(!first->property("focusIndicatorVisible").toBool());
        FocusIndicator::instance().setMode(FocusIndicator::Mode::Always);
        QVERIFY(first->property("focusIndicatorVisible").toBool());
        QTest::mouseClick(second, Qt::LeftButton);
        QTRY_VERIFY(second->property("focusIndicatorVisible").toBool());
        FocusIndicator::instance().setMode(FocusIndicator::Mode::Keyboard);
        QVERIFY(!second->property("focusIndicatorVisible").toBool());
        // A click on an already-focused widget must also hide a keyboard-only indicator.
        QTest::keyClick(second, Qt::Key_Space);
        QVERIFY(second->property("focusIndicatorVisible").toBool());
        QTest::mouseClick(second, Qt::LeftButton);
        QVERIFY(!second->property("focusIndicatorVisible").toBool());
    }
    void dangerFocusFollowsRoleForEveryMaterial() {
        auto catalog = std::make_shared<surface::Catalog>(profile);
        Z_PushButton button;
        button.setType(ButtonType::Primary);
        button.setDanger(true);
        button.setText("Danger");
        ButtonAppearanceAdapter adapter(&button, catalog);
        button.resize(180, 40);
        button.show(); button.activateWindow();
        FocusIndicator::instance().setMode(FocusIndicator::Mode::Always);
        button.setFocus(Qt::MouseFocusReason);
        QTRY_VERIFY(button.hasFocus());
        const auto color = button.focusIndicatorColor();
        QVERIFY(color.red() > color.blue());
        QVERIFY(qAbs(color.alphaF() - 0.55) < 0.005);
        for (const auto& id : catalog->buttonPresetIds()) {
            adapter.setAppearance(id);
            QTest::qWait(15);
            QVERIFY2(button.styleSheet().contains(color.name(QColor::HexArgb)), qPrintable(id));
            const auto visible = button.grab().toImage();
            FocusIndicator::instance().setMode(FocusIndicator::Mode::Off);
            QTest::qWait(15);
            QVERIFY(!button.styleSheet().contains(":focus"));
            QVERIFY(button.grab().toImage() != visible);
            FocusIndicator::instance().setMode(FocusIndicator::Mode::Always);
        }
        button.setLoading(true);
        QVERIFY(!FocusIndicator::instance().visibleFor(&button));
        button.setLoading(false);
        button.setEnabled(false);
        QVERIFY(!FocusIndicator::instance().visibleFor(&button));
    }
    void composedFieldsAndCheckboxShareFocusPolicy() {
        auto catalog = std::make_shared<surface::Catalog>(profile);
        QWidget host;
        auto* layout = new QVBoxLayout(&host);
        auto* check = new Z_CheckBox(catalog, "Enable");
        auto* input = new Z_InputField(tokens);
        auto* select = new Z_Select(tokens);
        auto* otp = new Z_OtpInput(tokens);
        input->setAllowClear(true); input->setValue("recipe");
        select->setOptions({{"a", "Alpha", {}, false}});
        select->setAllowClear(true); select->setValues({"a"});
        for (QWidget* control : QList<QWidget*>{check, input, select, otp})
            layout->addWidget(control);
        host.resize(480, host.sizeHint().height());
        host.show(); host.activateWindow();
        QList<QWidget*> targets{check, input->lineEdit(),
                               select->findChild<QWidget*>("DISPLAY"), otp->editors().first()};
        QList<QWidget*> surfaces{check, input->control(), select->findChild<field::Chrome*>(),
                                otp->findChild<field::Chrome*>("CELL_0")};
        for (int i = 0; i < targets.size(); ++i) {
            auto* target = targets[i]; auto* frame = surfaces[i];
            QVERIFY(target && frame);
            FocusIndicator::instance().setMode(FocusIndicator::Mode::Always);
            target->setFocus(Qt::MouseFocusReason);
            QTRY_VERIFY(target->hasFocus());
            QVERIFY(frame->property("focusIndicatorVisible").toBool());
            const auto before = frame->size();
            const auto pixels = frame->grab().toImage();
            FocusIndicator::instance().setMode(FocusIndicator::Mode::Off);
            QVERIFY(!frame->property("focusIndicatorVisible").toBool());
            QVERIFY(target->hasFocus());
            QCOMPARE(frame->size(), before);
            QVERIFY(frame->grab().toImage() != pixels);
            if (auto* chrome = qobject_cast<field::Chrome*>(frame))
                QVERIFY(chrome->visualState() != "focus");
        }
        input->lineEdit()->setFocus();
        input->setValue({});
        QTest::keyClicks(input->lineEdit(), "abc");
        QCOMPARE(input->value(), QString("abc"));
        input->setStatus("error");
        QCOMPARE(input->control()->visualState(), QString("error"));
        QVERIFY(input->control()->styleSheet().contains(tokens->colorText("errorBorder")));
        FocusIndicator::instance().setMode(FocusIndicator::Mode::Always);
        QCOMPARE(input->control()->visualState(), QString("error"));
        FocusIndicator::instance().setMode(FocusIndicator::Mode::Off);
        QTest::mouseClick(input->findChild<field::Action*>("CLEAR"), Qt::LeftButton);
        QCOMPARE(input->value(), QString());
        check->setFocus();
        const bool checked = check->isChecked();
        QTest::keyClick(check, Qt::Key_Space);
        QCOMPARE(check->isChecked(), !checked);
    }
    void demoIconsHaveNoSeparateSurface() {
        QVERIFY(!part<QComboBox>("leadingStyle"));
        QVERIFY(!part<QComboBox>("trailingStyle"));
        auto* b = part<Z_PushButton>("BUTTON_ROOT");
        auto* material = part<QComboBox>("buttonAppearance");
        for (int i = 0; i < material->count(); ++i) {
            material->setCurrentIndex(i);
            for (auto* slot : b->findChildren<QWidget*>("ICON_CONTAINER"))
                QCOMPARE(slot->property("iconContainerStyle").toString(), QString("BARE"));
        }
        material->setCurrentIndex(material->findData("solid"));
    }
    void fieldActionsRemainInsideControl() {
        Z_InputField input(tokens);
        input.setAllowClear(true);
        input.setValue("recipe");
        Z_Select select(tokens);
        select.setOptions({{"a", "Alpha", {}, false}});
        select.setValues({"a"});
        select.setAllowClear(true);
        for (auto* host : QList<QWidget*>{&input, &select}) {
            host->resize(420, host->sizeHint().height());
            host->show();
            QTest::qWait(30);
            auto* clear = host->findChild<field::Action*>("CLEAR");
            auto* control = host->findChild<field::Chrome*>("CONTROL");
            QVERIFY(clear && control && clear->isVisible());
            QVERIFY(clear->height() >= 32);
            const auto top = clear->mapTo(control, QPoint()).y();
            QVERIFY2(top > tokens->metric("borderWidth"), "Clear action touches outer frame");
            QVERIFY(control->height() - top - clear->height() > tokens->metric("borderWidth"));
            clear->setFocus(Qt::TabFocusReason);
            QTest::qWait(20);
            const auto pixels = clear->grab().toImage();
            const auto focus = tokens->color("focusBorder");
            int edgePixels = 0;
            for (int y = pixels.height() / 4; y < pixels.height() * 3 / 4; ++y)
                edgePixels += pixels.pixelColor(0, y) == focus;
            QCOMPARE(edgePixels, 0);
            if (!evidence.isEmpty())
                QVERIFY(host->grab().save(
                    evidence + (host == &input ? "/input-action.png" : "/select-action.png")));
        }
    }
    void invalidResourceDisablesPreview() {
        auto* b = part<Z_PushButton>("BUTTON_ROOT");
        edit("leadingSource", ":/icons/missing.svg");
        const bool invalidEnabled = b->isEnabled();
        const bool hasError = !part<QLabel>("buttonValidation")->text().isEmpty();
        edit("leadingSource", ":/icons/arrow-left.svg");
        QVERIFY(hasError);
        QVERIFY(!invalidEnabled);
        QVERIFY(b->isEnabled());
    }
    void unavailableMaterialIsNotSelectable() {
        auto* type = part<QComboBox>("buttonType");
        auto* material = part<QComboBox>("buttonAppearance");
        material->setCurrentIndex(material->findData("solid"));
        type->setCurrentIndex(3);
        const auto flags = material->model()->flags(
            material->model()->index(material->findData("gradient-inset"), 0));
        type->setCurrentIndex(0);
        QVERIFY(!(flags & Qt::ItemIsEnabled));
    }
    void cascadingConfigurationAndRecovery() {
        auto* b = part<Z_PushButton>("BUTTON_ROOT");
        auto* type = part<QComboBox>("buttonType");
        auto* material = part<QComboBox>("buttonAppearance");
        auto* ghost = part<Z_CheckBox>("buttonGhost");
        material->setCurrentIndex(material->findData("solid"));
        ghost->setChecked(true);
        material->setCurrentIndex(material->findData("solid-inner-shadow"));
        QVERIFY(!ghost->isEnabled() && !ghost->isChecked());
        QVERIFY(!b->property("ghost").toBool());
        part<Z_CheckBox>("buttonDanger")->setChecked(true);
        QVERIFY(!b->findChild<surface::solid_inner_shadow::Layer*>()->isHidden());
        part<Z_CheckBox>("buttonDanger")->setChecked(false);
        material->setCurrentIndex(material->findData("gradient-inset"));
        type->setCurrentIndex(3);
        QCOMPARE(material->currentData().toString(), QString("solid"));
        QVERIFY(!part<QLabel>("buttonConfigurationStatus")->text().isEmpty());
        type->setCurrentIndex(0);
        part<QRadioButton>("buttonIconMode")->setChecked(true);
        QVERIFY(!part<QLineEdit>("trailingSource")->isEnabled());
        QVERIFY(!part<QComboBox>("trailingStyle"));
        QVERIFY(!part<Z_CheckBox>("trailingVisible")->isEnabled());
        part<QComboBox>("buttonIconSide")->setCurrentIndex(1);
        QVERIFY(!b->findChild<QWidget*>("TRAILING")->isHidden());
        QVERIFY(b->findChild<QWidget*>("LEADING")->isHidden());
        QVERIFY(!part<QLineEdit>("leadingSource")->isEnabled());
        QVERIFY(part<QLineEdit>("trailingSource")->isEnabled());
        edit("trailingSource", ":/icons/missing.svg");
        QVERIFY(!b->isEnabled());
        QSignalSpy clicks(b, &QPushButton::clicked);
        b->click();
        QCOMPARE(clicks.count(), 0);
        edit("trailingSource", ":/icons/arrow-right.svg");
        QVERIFY(b->isEnabled());
        part<QRadioButton>("buttonTextMode")->setChecked(true);
        QVERIFY(!b->findChild<QWidget*>("LEADING")->isHidden());
        QVERIFY(!b->findChild<QWidget*>("TRAILING")->isHidden());
        QVERIFY(!part<QComboBox>("buttonIconSide")->isEnabled());
        auto* state = part<QComboBox>("buttonState");
        state->setCurrentIndex(3);
        QVERIFY(b->isDown());
        state->setCurrentIndex(0);
        QVERIFY(!b->isDown());
        state->setCurrentIndex(2);
        part<Z_CheckBox>("buttonDisabled")->setChecked(true);
        QVERIFY(!state->isEnabled());
        QCOMPARE(state->currentIndex(), 0);
        part<Z_CheckBox>("buttonDisabled")->setChecked(false);
        part<Z_CheckBox>("buttonLoading")->setChecked(true);
        QVERIFY(!type->isEnabled() && !material->isEnabled());
        QVERIFY(!state->isEnabled());
        part<Z_CheckBox>("buttonLoading")->setChecked(false);
        QVERIFY(type->isEnabled() && material->isEnabled() && state->isEnabled());
        auto* statesMaterial = part<QComboBox>("buttonStateAppearance");
        statesMaterial->setCurrentIndex(statesMaterial->findData("solid-inner-shadow"));
        QVERIFY(!part<Z_PushButton>("buttonStateExample4")->isEnabled());
        QVERIFY(part<Z_PushButton>("buttonStateExample3")->isEnabled());
        statesMaterial->setCurrentIndex(statesMaterial->findData("solid"));
        QVERIFY(part<Z_PushButton>("buttonStateExample4")->isEnabled());
    }
    void buttonBothSourcesAreEditable() {
        QVERIFY(part<QLineEdit>("leadingSource"));
        QVERIFY(part<QLineEdit>("trailingSource"));
        auto* b = part<Z_PushButton>("BUTTON_ROOT");
        QCOMPARE(b->leadingIconSource(), QString(":/icons/arrow-left.svg"));
        QCOMPARE(b->trailingIconSource(), QString(":/icons/arrow-right.svg"));
        auto* left = b->findChild<QWidget*>("LEADING");
        auto* right = b->findChild<QWidget*>("TRAILING");
        QVERIFY(left && right);
        QVERIFY(!left->isHidden() && !right->isHidden());
        part<Z_CheckBox>("leadingVisible")->setChecked(false);
        QVERIFY(left->isHidden() && !right->isHidden());
        edit("trailingSource", ":/icons/x.svg");
        QCOMPARE(b->trailingIconSource(), QString(":/icons/x.svg"));
        QCOMPARE(b->leadingIconSource(), QString(":/icons/arrow-left.svg"));
        part<Z_CheckBox>("leadingVisible")->setChecked(true);
        QCOMPARE(
            left->findChild<QWidget*>("ICON_CONTAINER")->property("iconContainerStyle").toString(),
            QString("BARE"));
        QCOMPARE(
            right->findChild<QWidget*>("ICON_CONTAINER")->property("iconContainerStyle").toString(),
            QString("BARE"));
        edit("trailingSource", ":/icons/arrow-right.svg");
    }
    void iconOnlyLoadingAndRoundtrip() {
        auto* b = part<Z_PushButton>("BUTTON_ROOT");
        part<QRadioButton>("buttonIconMode")->setChecked(true);
        QVERIFY(b->isIconOnly());
        part<Z_CheckBox>("leadingVisible")->setChecked(false);
        QVERIFY(b->isIconOnly());
        const auto before = b->sizeHint();
        part<Z_CheckBox>("buttonLoading")->setChecked(true);
        QVERIFY(b->property("loading").toBool());
        QVERIFY(!b->isEnabled());
        QCOMPARE(b->sizeHint(), before);
        QSignalSpy clicked(b, &QPushButton::clicked);
        b->click();
        QCOMPARE(clicked.count(), 0);
        QTest::qWait(60);
        QCOMPARE(b->sizeHint(), before);
        part<Z_CheckBox>("buttonLoading")->setChecked(false);
        QVERIFY(b->isEnabled());
        part<QRadioButton>("buttonTextMode")->setChecked(true);
        QVERIFY(!b->isIconOnly());
        QCOMPARE(b->trailingIconSource(), QString(":/icons/arrow-right.svg"));
        part<Z_CheckBox>("leadingVisible")->setChecked(true);
        QVERIFY(!b->findChild<QWidget*>("LEADING")->isHidden());
        QVERIFY(!b->findChild<QWidget*>("TRAILING")->isHidden());
    }
    void referenceAppearanceIsButtonOnly() {
        auto* material = part<QComboBox>("buttonAppearance");
        const auto index = material->findData("solid-inner-shadow");
        QVERIFY(index >= 0);
        QVERIFY(part<QComboBox>("checkboxAppearance")->findData("solid-inner-shadow") < 0);
        QVERIFY(part<QComboBox>("inputAppearance")->findData("solid-inner-shadow") < 0);
        QVERIFY(!window->findChild<QWidget*>("shadowEditor"));
        material->setCurrentIndex(index);
        QTest::qWait(30);
        auto* b = part<Z_PushButton>("BUTTON_ROOT");
        auto* layer = b->findChild<surface::solid_inner_shadow::Layer*>();
        QVERIFY(layer && !layer->isHidden());
        part<Z_CheckBox>("buttonDisabled")->setChecked(true);
        QTest::qWait(30);
        QCOMPARE(layer->state(), QString("disabled"));
        part<Z_CheckBox>("buttonDisabled")->setChecked(false);
        material->setCurrentIndex(material->findData("solid"));
    }
    void restoredButtonPreservesMaterials() {
        auto* b = part<Z_PushButton>("BUTTON_ROOT");
        auto* material = part<QComboBox>("buttonAppearance");
        auto* adapter = b->findChild<ButtonAppearanceAdapter*>();
        QVERIFY(adapter);
        const auto before = b->sizeHint();
        for (auto id : {"gradient-inset", "gradient-elevated", "outline", "solid"}) {
            material->setCurrentIndex(material->findData(id));
            QTest::qWait(30);
            QCOMPARE(b->sizeHint(), before);
            QCOMPARE(b->trailingIconSource(), QString(":/icons/arrow-right.svg"));
            QCOMPARE(b->findChild<QWidget*>("TRAILING")
                         ->findChild<QWidget*>("ICON_CONTAINER")
                         ->property("iconContainerStyle")
                         .toString(),
                     QString("BARE"));
            if (QString(id).startsWith("gradient"))
                QVERIFY(b->styleSheet().contains("qlineargradient"));
            QVERIFY(b->findChild<surface::InsetLayer*>());
            QVERIFY(b->graphicsEffect());
            if (!evidence.isEmpty())
                QVERIFY(b->grab().save(evidence + "/button-" + id + ".png"));
        }
    }
    void fieldActionsReuseButton() {
        Z_InputField input(tokens);
        Z_Select select(tokens);
        select.setMode(Z_Select::Mode::Multiple);
        select.setOptions({{"a", "Alpha", {}, false}});
        select.setValues({"a"});
        Z_PushButton reference;
        reference.setType(ButtonType::Text);
        reference.setSize(ButtonSize::XS);
        reference.setLeadingIconSource(":/icons/x.svg");
        reference.setAccessibleName("Clear");
        reference.setIconOnly(true);
        input.setKind(Z_InputField::Kind::Search);
        input.setValue("recipe");
        input.setAllowClear(true);
        input.show();
        select.show();
        QTest::qWait(30);
        int total = 0;
        for (auto* parent : QList<QWidget*>{&input, &select}) {
            const auto actions = parent->findChildren<field::Action*>();
            for (auto* action : actions) {
                QVERIFY(qobject_cast<Z_PushButton*>(action));
                QVERIFY(action->isIconOnly());
                QCOMPARE(action->property("buttonType").toString(), QString("TEXT"));
                QCOMPARE(action->property("buttonSize").toString(), QString("XS"));
                QCOMPARE(action->sizeHint(), reference.sizeHint());
                ++total;
            }
        }
        QCOMPARE(total, 6);
        auto* search = input.findChild<Z_PushButton*>("SEARCH_ACTION");
        input.setLoading(true);
        QVERIFY(search->property("loading").toBool());
        QVERIFY(!search->isEnabled());
        input.setLoading(false);
        QVERIFY(search->isEnabled());
        auto* clear = input.findChild<Z_PushButton*>("CLEAR");
        clear->click();
        QCOMPARE(input.value(), QString());
    }
    void selectRowsReuseCheckbox() {
        Z_Select select(tokens);
        select.setMode(Z_Select::Mode::Multiple);
        select.setOptions({{"a", "Alpha", {}, false}, {"b", "Beta", {}, true}});
        select.setValues({"a"});
        QCOMPARE(select.findChildren<Z_CheckBox*>().count(), 1);
        select.resize(420, select.sizeHint().height());
        select.show();
        select.setOpen(true);
        QTest::qWait(40);
        auto* view = select.optionView();
        auto* renderer = select.findChild<Z_CheckBox*>("CHECKBOX_ROW_RENDERER");
        for (const auto& appearance : {"solid", "gradient-inset", "gradient-elevated"}) {
            select.setAppearance(appearance);
            for (int row = 0; row < 2; ++row) {
                QImage image(360, 48, QImage::Format_ARGB32_Premultiplied);
                image.fill(Qt::white);
                QStyleOptionViewItem opt;
                opt.initFrom(view);
                opt.widget = view;
                opt.rect = image.rect();
                opt.state = row == 0 ? QStyle::State_Enabled : QStyle::State_None;
                QPainter painter(&image);
                view->itemDelegate()->paint(&painter, opt, view->model()->index(row, 0));
                painter.end();
                QCOMPARE(renderer->isChecked(), row == 0);
                QCOMPARE(renderer->isEnabled(), row == 0);
                QCOMPARE(renderer->appearance(), QString(appearance));
                const auto area = renderer->indicatorRect().adjusted(3, 3, -3, -3);
                QImage direct(renderer->size(), QImage::Format_ARGB32_Premultiplied);
                direct.fill(Qt::white);
                renderer->render(&direct);
                const QPoint offset(tokens->metric("popupPadding"), 0);
                QCOMPARE(image.copy(area.translated(offset)).convertToFormat(QImage::Format_RGB32),
                         direct.copy(area).convertToFormat(QImage::Format_RGB32));
            }
            QTest::qWait(20);
            if (!evidence.isEmpty())
                QVERIFY(select.popupWidget()->grab().save(evidence + "/multiple-" + appearance +
                                                          ".png"));
        }
        QCOMPARE(select.values(), QStringList{"a"});
        select.setOpen(false);
        QVector<Z_Select::Option> many;
        for (int i = 0; i < 10000; ++i)
            many.append({QString::number(i), QString("Row %1").arg(i), {}, false});
        QElapsedTimer timer;
        timer.start();
        select.setOptions(many);
        qInfo("Multiple model 10000 rows: %lld ms", timer.elapsed());
        QCOMPARE(view->model()->rowCount(), 10000);
        QCOMPARE(select.findChildren<Z_CheckBox*>().count(), 1);
        QVERIFY(select.findChildren<QWidget*>().count() < 100);
    }
};
int main(int argc, char** argv) {
    QCoreApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
    QCoreApplication::setAttribute(Qt::AA_UseHighDpiPixmaps);
    QApplication app(argc, argv);
    app.setStyle(QStyleFactory::create("Fusion"));
    button_demo::install(app, qEnvironmentVariable("DEMO_PROFILE", "light-comfortable"));
    CompositionTests tests;
    return QTest::qExec(&tests, argc, argv);
}
#include "CompositionTests.moc"
