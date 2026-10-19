#pragma once

#include "Z_InputField.h"
#include <QAbstractButton>
#include <QColor>
#include <QList>
#include <QModelIndex>
#include <QPointer>
#include <QPoint>
#include <QStringList>
#include <QToolButton>
#include <QUrl>
#include <QWidget>

class QAbstractSpinBox;
class QAbstractItemModel;
class QBoxLayout;
class QButtonGroup;
class QDoubleSpinBox;
class QDragEnterEvent;
class QDragLeaveEvent;
class QDropEvent;
class QGridLayout;
class QHBoxLayout;
class QKeyEvent;
class QLabel;
class QListView;
class QMouseEvent;
class QSpinBox;
class QVBoxLayout;
namespace commonWidget {

class Z_Badge final : public QWidget {
    Q_OBJECT
    Q_PROPERTY(QString label READ label WRITE setLabel NOTIFY labelChanged)
    Q_PROPERTY(QString kind READ kind WRITE setKind NOTIFY kindChanged)
    Q_PROPERTY(QString tone READ tone WRITE setTone NOTIFY toneChanged)
  public:
    explicit Z_Badge(const QString& profile = QStringLiteral("standard-light"),
                     QWidget* parent = nullptr);
    QString label() const { return labelValue; }
    QString kind() const { return kindValue; }
    QString tone() const { return toneValue; }
    void setLabel(const QString& value);
    void setKind(const QString& value);
    void setTone(const QString& value);
    QSize sizeHint() const override;
  signals:
    void labelChanged(const QString& value);
    void kindChanged(const QString& value);
    void toneChanged(const QString& value);
  protected:
    void paintEvent(QPaintEvent*) override;
  private:
    QString profileValue;
    QString labelValue = QStringLiteral("99+");
    QString kindValue = QStringLiteral("Text");
    QString toneValue = QStringLiteral("Neutral");
};

class Z_StatusIndicator final : public QWidget {
    Q_OBJECT
    Q_PROPERTY(QString label READ label WRITE setLabel NOTIFY labelChanged)
    Q_PROPERTY(bool showLabel READ showLabel WRITE setShowLabel NOTIFY showLabelChanged)
    Q_PROPERTY(QString size READ size WRITE setSize NOTIFY sizeChanged)
    Q_PROPERTY(QString tone READ tone WRITE setTone NOTIFY toneChanged)
  public:
    explicit Z_StatusIndicator(const QString& profile = QStringLiteral("standard-light"),
                               QWidget* parent = nullptr);
    QString label() const { return labelValue; }
    bool showLabel() const { return showLabelValue; }
    QString size() const { return sizeValue; }
    QString tone() const { return toneValue; }
    void setLabel(const QString& value);
    void setShowLabel(bool value);
    void setSize(const QString& value);
    void setTone(const QString& value);
    QSize sizeHint() const override;
  signals:
    void labelChanged(const QString& value);
    void showLabelChanged(bool value);
    void sizeChanged(const QString& value);
    void toneChanged(const QString& value);
  protected:
    void paintEvent(QPaintEvent*) override;
  private:
    QString profileValue;
    QString labelValue = QStringLiteral("Status");
    bool showLabelValue = true;
    QString sizeValue = QStringLiteral("Medium");
    QString toneValue = QStringLiteral("Neutral");
};

class Z_Link final : public QAbstractButton {
    Q_OBJECT
    Q_PROPERTY(QUrl target READ target WRITE setTarget NOTIFY targetChanged)
    Q_PROPERTY(QString snapshotState READ snapshotState WRITE setSnapshotState NOTIFY snapshotStateChanged)
  public:
    explicit Z_Link(const QString& profile = QStringLiteral("standard-light"),
                    QWidget* parent = nullptr);
    QUrl target() const { return targetValue; }
    QString snapshotState() const { return snapshotStateValue; }
    void setTarget(const QUrl& value);
    // Empty restores native hover/press/focus behavior. Named values provide
    // deterministic Figma/runtime evidence and generated static states.
    void setSnapshotState(const QString& value);
    QSize sizeHint() const override;
  signals:
    void targetChanged(const QUrl& value);
    void snapshotStateChanged(const QString& value);
    void activated(const QUrl& target);
  protected:
    void paintEvent(QPaintEvent*) override;
    void keyPressEvent(QKeyEvent* event) override;
  private:
    QString profileValue;
    QUrl targetValue;
    QString snapshotStateValue;
};

class Z_Tag final : public QWidget {
    Q_OBJECT
    Q_PROPERTY(QString label READ label WRITE setLabel NOTIFY labelChanged)
    Q_PROPERTY(QString size READ size WRITE setSize NOTIFY sizeChanged)
    Q_PROPERTY(QString tone READ tone WRITE setTone NOTIFY toneChanged)
    Q_PROPERTY(bool dismissible READ isDismissible WRITE setDismissible NOTIFY dismissibleChanged)
  public:
    explicit Z_Tag(const QString& profile = QStringLiteral("standard-light"),
                   QWidget* parent = nullptr);
    QString label() const { return labelValue; }
    QString size() const { return sizeValue; }
    QString tone() const { return toneValue; }
    bool isDismissible() const { return dismissibleValue; }
    void setLabel(const QString& value);
    void setSize(const QString& value);
    void setTone(const QString& value);
    void setDismissible(bool value);
    QSize sizeHint() const override;
  signals:
    void labelChanged(const QString& value);
    void sizeChanged(const QString& value);
    void toneChanged(const QString& value);
    void dismissibleChanged(bool value);
    void dismissed();
  protected:
    void paintEvent(QPaintEvent*) override;
    void resizeEvent(QResizeEvent*) override;
  private:
    QString profileValue;
    QString labelValue = QStringLiteral("Tag");
    QString sizeValue = QStringLiteral("Medium");
    QString toneValue = QStringLiteral("Neutral");
    bool dismissibleValue = false;
    QToolButton* dismissButton = nullptr;
};

class Z_Tooltip final : public QWidget {
    Q_OBJECT
    Q_PROPERTY(QString content READ content WRITE setContent NOTIFY contentChanged)
    Q_PROPERTY(QString placement READ placement WRITE setPlacement NOTIFY placementChanged)
    Q_PROPERTY(bool showArrow READ showArrow WRITE setShowArrow NOTIFY showArrowChanged)
  public:
    explicit Z_Tooltip(const QString& profile = QStringLiteral("standard-light"),
                       QWidget* parent = nullptr);
    ~Z_Tooltip() override;
    QString content() const { return contentValue; }
    QString placement() const { return placementValue; }
    bool showArrow() const { return showArrowValue; }
    QWidget* targetWidget() const { return targetValue.data(); }
    void setContent(const QString& value);
    void setPlacement(const QString& value);
    void setShowArrow(bool value);
    void attachTo(QWidget* target);
    void detach();
    QSize sizeHint() const override;
  signals:
    void contentChanged(const QString& value);
    void placementChanged(const QString& value);
    void showArrowChanged(bool value);
  protected:
    bool eventFilter(QObject* watched, QEvent* event) override;
    void paintEvent(QPaintEvent*) override;
  private:
    void showForTarget();
    QString profileValue;
    QString contentValue = QStringLiteral("Helpful context");
    QString placementValue = QStringLiteral("Top");
    bool showArrowValue = true;
    QPointer<QWidget> targetValue;
};

// Search is a semantic specialization of Input: query submission and clearing
// remain public behavior while the existing field implementation owns editing.
class Z_SearchInput final : public Z_InputField {
    Q_OBJECT
    Q_PROPERTY(QString shortcutText READ shortcutText WRITE setShortcutText NOTIFY shortcutTextChanged)
    Q_PROPERTY(QString snapshotState READ snapshotState WRITE setSnapshotState NOTIFY snapshotStateChanged)
  public:
    explicit Z_SearchInput(const QString& profile = QStringLiteral("standard-light"),
                           QWidget* parent = nullptr);
    QString shortcutText() const { return shortcutValue; }
    QString snapshotState() const { return snapshotStateValue; }
    void setShortcutText(const QString& value);
    void setSnapshotState(const QString& value);
    QSize sizeHint() const override;
  signals:
    void shortcutTextChanged(const QString& value);
    void snapshotStateChanged(const QString& value);
  private:
    QString shortcutValue = QStringLiteral("⌘K");
    QString snapshotStateValue = QStringLiteral("Default");
};

// One public design identity adapts the integer and decimal Qt editors without
// exposing two competing canonical components.
class Z_NumberInput final : public QWidget {
    Q_OBJECT
    Q_PROPERTY(QString type READ type WRITE setType NOTIFY typeChanged)
    Q_PROPERTY(double value READ value WRITE setValue NOTIFY valueChanged)
    Q_PROPERTY(double minimum READ minimum WRITE setMinimum)
    Q_PROPERTY(double maximum READ maximum WRITE setMaximum)
    Q_PROPERTY(double singleStep READ singleStep WRITE setSingleStep)
    Q_PROPERTY(QString snapshotState READ snapshotState WRITE setSnapshotState NOTIFY snapshotStateChanged)
  public:
    explicit Z_NumberInput(const QString& profile = QStringLiteral("standard-light"),
                           QWidget* parent = nullptr);
    QString type() const { return typeValue; }
    double value() const;
    double minimum() const { return minimumValue; }
    double maximum() const { return maximumValue; }
    double singleStep() const { return stepValue; }
    QString snapshotState() const { return snapshotStateValue; }
    QAbstractSpinBox* editor() const;
    void setType(const QString& value);
    void setValue(double value);
    void setMinimum(double value);
    void setMaximum(double value);
    void setRange(double minimum, double maximum);
    void setSingleStep(double value);
    void setSnapshotState(const QString& value);
    QSize sizeHint() const override;
  signals:
    void typeChanged(const QString& value);
    void valueChanged(double value);
    void snapshotStateChanged(const QString& value);
  private:
    void syncContracts();
    QString typeValue = QStringLiteral("Integer");
    QString snapshotStateValue = QStringLiteral("Default");
    double minimumValue = 0.0;
    double maximumValue = 100.0;
    double stepValue = 1.0;
    QSpinBox* integerEditor = nullptr;
    QDoubleSpinBox* decimalEditor = nullptr;
    QHBoxLayout* editorLayout = nullptr;
};

// ColorPicker is the stable field identity, not the dialog implementation.
// It displays the controlled color value and requests an external selection
// flow; preview, commit/cancel, color space and recent colors stay external.
class Z_ColorPicker final : public QAbstractButton {
    Q_OBJECT
    Q_PROPERTY(QColor color READ color WRITE setColor NOTIFY colorChanged)
    Q_PROPERTY(QString colorFormat READ colorFormat WRITE setColorFormat NOTIFY colorFormatChanged)
    Q_PROPERTY(QString snapshotState READ snapshotState WRITE setSnapshotState NOTIFY snapshotStateChanged)
  public:
    explicit Z_ColorPicker(
        const QString& profile = QStringLiteral("standard-light"),
        QWidget* parent = nullptr);
    QColor color() const { return colorValue; }
    QString colorFormat() const { return colorFormatValue; }
    QString snapshotState() const { return snapshotStateValue; }
    QString colorText() const;
    void setColor(const QColor& value);
    void setColorFormat(const QString& value);
    void setSnapshotState(const QString& value);
    QSize sizeHint() const override;
  signals:
    void colorChanged(const QColor& value);
    void colorFormatChanged(const QString& value);
    void snapshotStateChanged(const QString& value);
    void pickerRequested(const QColor& currentColor);
  protected:
    void paintEvent(QPaintEvent*) override;
    void keyPressEvent(QKeyEvent* event) override;
    void changeEvent(QEvent* event) override;
  private:
    QString profileValue;
    QColor colorValue = QColor(QStringLiteral("#5B5BD6"));
    QString colorFormatValue = QStringLiteral("HexRgb");
    QString snapshotStateValue = QStringLiteral("Default");
};

// Transfer is one controlled dual-list selection contract. The SDK owns the
// two views, command affordances and focus/accessibility behavior; external
// models/controllers own collection queries, mutations, ordering, permission
// checks and error recovery.
class Z_Transfer final : public QWidget {
    Q_OBJECT
    Q_PROPERTY(QString snapshotState READ snapshotState WRITE setSnapshotState NOTIFY snapshotStateChanged)
    Q_PROPERTY(QString availableTitle READ availableTitle WRITE setAvailableTitle NOTIFY availableTitleChanged)
    Q_PROPERTY(QString selectedTitle READ selectedTitle WRITE setSelectedTitle NOTIFY selectedTitleChanged)
    Q_PROPERTY(bool moveToSelectedAllowed READ isMoveToSelectedAllowed WRITE setMoveToSelectedAllowed NOTIFY moveToSelectedAllowedChanged)
    Q_PROPERTY(bool moveToAvailableAllowed READ isMoveToAvailableAllowed WRITE setMoveToAvailableAllowed NOTIFY moveToAvailableAllowedChanged)
  public:
    explicit Z_Transfer(
        const QString& profile = QStringLiteral("standard-light"),
        QWidget* parent = nullptr);
    QString snapshotState() const { return snapshotStateValue; }
    QString availableTitle() const { return availableTitleValue; }
    QString selectedTitle() const { return selectedTitleValue; }
    bool isMoveToSelectedAllowed() const { return moveToSelectedAllowedValue; }
    bool isMoveToAvailableAllowed() const { return moveToAvailableAllowedValue; }
    QAbstractItemModel* availableModel() const;
    QAbstractItemModel* selectedModel() const;
    QListView* availableView() const { return availableViewValue; }
    QListView* selectedView() const { return selectedViewValue; }
    QToolButton* moveToSelectedButton() const { return moveToSelectedButtonValue; }
    QToolButton* moveToAvailableButton() const { return moveToAvailableButtonValue; }
    void setSnapshotState(const QString& value);
    void setAvailableTitle(const QString& value);
    void setSelectedTitle(const QString& value);
    void setMoveToSelectedAllowed(bool value);
    void setMoveToAvailableAllowed(bool value);
    void setAvailableModel(QAbstractItemModel* model);
    void setSelectedModel(QAbstractItemModel* model);
    QSize sizeHint() const override;
  signals:
    void snapshotStateChanged(const QString& value);
    void availableTitleChanged(const QString& value);
    void selectedTitleChanged(const QString& value);
    void moveToSelectedAllowedChanged(bool value);
    void moveToAvailableAllowedChanged(bool value);
    void availableModelChanged(QAbstractItemModel* model);
    void selectedModelChanged(QAbstractItemModel* model);
    void selectionChanged();
    void moveToSelectedRequested(const QModelIndexList& rows);
    void moveToAvailableRequested(const QModelIndexList& rows);
  protected:
    void paintEvent(QPaintEvent*) override;
    void changeEvent(QEvent* event) override;
  private:
    void bindView(QListView* view);
    QModelIndexList requestedRows(QListView* view) const;
    void refreshContent();
    void refreshStyle();
    void refreshAccessibility();
    QString profileValue;
    QString snapshotStateValue = QStringLiteral("Default");
    QString availableTitleValue = QStringLiteral("Available");
    QString selectedTitleValue = QStringLiteral("Selected");
    bool moveToSelectedAllowedValue = true;
    bool moveToAvailableAllowedValue = true;
    QLabel* availableLabelValue = nullptr;
    QLabel* selectedLabelValue = nullptr;
    QListView* availableViewValue = nullptr;
    QListView* selectedViewValue = nullptr;
    QToolButton* moveToSelectedButtonValue = nullptr;
    QToolButton* moveToAvailableButtonValue = nullptr;
};

// Upload is a controlled acquisition and lifecycle surface, not QFileDialog
// and not an upload service. It requests browsing or reports dropped local
// files; validation, transport, authentication, cancellation and retry stay
// with the external page controller/service, which writes state back here.
class Z_Upload final : public QAbstractButton {
    Q_OBJECT
    Q_PROPERTY(QString snapshotState READ snapshotState WRITE setSnapshotState NOTIFY snapshotStateChanged)
    Q_PROPERTY(int progress READ progress WRITE setProgress NOTIFY progressChanged)
    Q_PROPERTY(QString fileName READ fileName WRITE setFileName NOTIFY fileNameChanged)
    Q_PROPERTY(QString helperText READ helperText WRITE setHelperText NOTIFY helperTextChanged)
  public:
    explicit Z_Upload(
        const QString& profile = QStringLiteral("standard-light"),
        QWidget* parent = nullptr);
    QString snapshotState() const { return snapshotStateValue; }
    int progress() const { return progressValue; }
    QString fileName() const { return fileNameValue; }
    QString helperText() const { return helperTextValue; }
    void setSnapshotState(const QString& value);
    void setProgress(int value);
    void setFileName(const QString& value);
    void setHelperText(const QString& value);
    QSize sizeHint() const override;
  signals:
    void snapshotStateChanged(const QString& value);
    void progressChanged(int value);
    void fileNameChanged(const QString& value);
    void helperTextChanged(const QString& value);
    void browseRequested();
    void filesDropped(const QStringList& localFiles);
  protected:
    void paintEvent(QPaintEvent*) override;
    void keyPressEvent(QKeyEvent* event) override;
    void dragEnterEvent(QDragEnterEvent* event) override;
    void dragLeaveEvent(QDragLeaveEvent* event) override;
    void dropEvent(QDropEvent* event) override;
    void changeEvent(QEvent* event) override;
  private:
    QString primaryText() const;
    void refreshAccessibility();
    QString profileValue;
    QString snapshotStateValue = QStringLiteral("Default");
    int progressValue = 0;
    QString fileNameValue;
    QString helperTextValue = QStringLiteral("CSV, JSON · max 20 MB");
    bool dragActive = false;
};

// NumericKeypad is a controlled touch-input surface. It emits semantic key
// intentions; the target field, value constraints, locale parsing and commit
// policy remain owned by the external input controller.
class Z_NumericKeypad final : public QWidget {
    Q_OBJECT
    Q_PROPERTY(QString density READ density WRITE setDensity NOTIFY densityChanged)
    Q_PROPERTY(QStringList keyLayout READ keyLayout WRITE setKeyLayout NOTIFY keyLayoutChanged)
    Q_PROPERTY(QStringList allowedKeys READ allowedKeys WRITE setAllowedKeys NOTIFY allowedKeysChanged)
    Q_PROPERTY(QString decimalSeparator READ decimalSeparator WRITE setDecimalSeparator NOTIFY decimalSeparatorChanged)
  public:
    explicit Z_NumericKeypad(
        const QString& profile = QStringLiteral("standard-light"),
        QWidget* parent = nullptr);
    QString density() const { return densityValue; }
    QStringList keyLayout() const { return keyValues; }
    QStringList allowedKeys() const { return allowedKeyValues; }
    QString decimalSeparator() const { return decimalSeparatorValue; }
    void setDensity(const QString& value);
    void setKeyLayout(const QStringList& values);
    void setAllowedKeys(const QStringList& values);
    void setDecimalSeparator(const QString& value);
    QSize sizeHint() const override;
  signals:
    void densityChanged(const QString& value);
    void keyLayoutChanged(const QStringList& values);
    void allowedKeysChanged(const QStringList& values);
    void decimalSeparatorChanged(const QString& value);
    void keyPressed(const QString& semanticKey);
    void commitRequested();
  protected:
    bool eventFilter(QObject* watched, QEvent* event) override;
    void paintEvent(QPaintEvent*) override;
    void changeEvent(QEvent* event) override;
  private:
    QString displayText(const QString& semanticKey) const;
    QString accessibleNameFor(const QString& semanticKey) const;
    void refreshGeometry();
    void refreshStyle();
    void refreshAccessibleDescription();
    QString profileValue;
    QString densityValue = QStringLiteral("Comfortable");
    QStringList keyValues{QStringLiteral("7"), QStringLiteral("8"), QStringLiteral("9"),
                          QStringLiteral("4"), QStringLiteral("5"), QStringLiteral("6"),
                          QStringLiteral("1"), QStringLiteral("2"), QStringLiteral("3"),
                          QStringLiteral("decimal"), QStringLiteral("0"),
                          QStringLiteral("backspace")};
    QStringList allowedKeyValues;
    QString decimalSeparatorValue = QStringLiteral(".");
    QGridLayout* keyLayoutValue = nullptr;
    QList<QPointer<QToolButton>> keyButtons;
};

class Z_OverflowMenuButton final : public QToolButton {
    Q_OBJECT
    Q_PROPERTY(QString snapshotState READ snapshotState WRITE setSnapshotState NOTIFY snapshotStateChanged)
  public:
    explicit Z_OverflowMenuButton(const QString& profile = QStringLiteral("standard-light"),
                                  QWidget* parent = nullptr);
    QString snapshotState() const { return snapshotStateValue; }
    void setSnapshotState(const QString& value);
    QSize sizeHint() const override;
  signals:
    void snapshotStateChanged(const QString& value);
  protected:
    void paintEvent(QPaintEvent*) override;
  private:
    QString profileValue;
    QString snapshotStateValue;
};

class Z_SegmentedControl final : public QWidget {
    Q_OBJECT
    Q_PROPERTY(QStringList items READ items WRITE setItems NOTIFY itemsChanged)
    Q_PROPERTY(int segmentCount READ segmentCount WRITE setSegmentCount NOTIFY segmentCountChanged)
    Q_PROPERTY(int currentIndex READ currentIndex WRITE setCurrentIndex NOTIFY currentIndexChanged)
  public:
    explicit Z_SegmentedControl(const QString& profile = QStringLiteral("standard-light"),
                                QWidget* parent = nullptr);
    QStringList items() const { return itemValues; }
    int segmentCount() const { return itemValues.size(); }
    int currentIndex() const { return currentIndexValue; }
    QString currentText() const;
    void setItems(const QStringList& values);
    void setSegmentCount(int value);
    void setCurrentIndex(int value);
    QSize sizeHint() const override;
  signals:
    void itemsChanged(const QStringList& values);
    void segmentCountChanged(int value);
    void currentIndexChanged(int value);
    void currentTextChanged(const QString& value);
  protected:
    bool eventFilter(QObject* watched, QEvent* event) override;
    void paintEvent(QPaintEvent*) override;
    void changeEvent(QEvent* event) override;
  private:
    void rebuild();
    void refreshStyle();
    QString profileValue;
    QStringList itemValues{QStringLiteral("Overview"), QStringLiteral("Parameters")};
    int currentIndexValue = 0;
    QButtonGroup* group = nullptr;
    QHBoxLayout* segmentLayout = nullptr;
};

// Stepper visualizes task progress. It is intentionally non-interactive: the
// workflow owner changes the current step and state through the public model.
class Z_Stepper final : public QWidget {
    Q_OBJECT
    Q_PROPERTY(QStringList items READ items WRITE setItems NOTIFY itemsChanged)
    Q_PROPERTY(int currentIndex READ currentIndex WRITE setCurrentIndex NOTIFY currentIndexChanged)
    Q_PROPERTY(QString snapshotState READ snapshotState WRITE setSnapshotState NOTIFY snapshotStateChanged)
  public:
    explicit Z_Stepper(const QString& profile = QStringLiteral("standard-light"),
                       QWidget* parent = nullptr);
    QStringList items() const { return itemValues; }
    int currentIndex() const { return currentIndexValue; }
    QString snapshotState() const { return snapshotStateValue; }
    void setItems(const QStringList& values);
    void setCurrentIndex(int value);
    void setSnapshotState(const QString& value);
    QSize sizeHint() const override;
  signals:
    void itemsChanged(const QStringList& values);
    void currentIndexChanged(int value);
    void snapshotStateChanged(const QString& value);
  protected:
    void paintEvent(QPaintEvent*) override;
  private:
    QString profileValue;
    QStringList itemValues{QStringLiteral("Setup"), QStringLiteral("Parameters"),
                           QStringLiteral("Review")};
    int currentIndexValue = 1;
    QString snapshotStateValue = QStringLiteral("Default");
};

// Timeline presents immutable event history. It deliberately owns no
// navigation or activation behavior; workflow navigation belongs to Stepper.
class Z_Timeline final : public QWidget {
    Q_OBJECT
    Q_PROPERTY(QString density READ density WRITE setDensity NOTIFY densityChanged)
  public:
    explicit Z_Timeline(const QString& profile = QStringLiteral("standard-light"),
                        QWidget* parent = nullptr);
    QString density() const { return densityValue; }
    QStringList eventLabels() const { return eventLabelValues; }
    QStringList eventTimes() const { return eventTimeValues; }
    QStringList eventStates() const { return eventStateValues; }
    void setDensity(const QString& value);
    void setEvents(const QStringList& labels, const QStringList& times,
                   const QStringList& states);
    QSize sizeHint() const override;
  signals:
    void densityChanged(const QString& value);
    void eventsChanged();
  protected:
    void paintEvent(QPaintEvent*) override;
  private:
    void refreshAccessibleDescription();
    QString profileValue;
    QString densityValue = QStringLiteral("Comfortable");
    QStringList eventLabelValues{QStringLiteral("Created"),
                                 QStringLiteral("Processing"),
                                 QStringLiteral("Completed")};
    QStringList eventTimeValues{QStringLiteral("09:20"), QStringLiteral("09:32"),
                                QStringLiteral("10:05")};
    QStringList eventStateValues{QStringLiteral("Brand"), QStringLiteral("Process"),
                                 QStringLiteral("Success")};
};

// AnchorNavigation is a controlled page-navigation component. It emits an
// activation request, while the page owns scrolling and feeds the observed
// section back through currentIndex.
class Z_AnchorNavigation final : public QWidget {
    Q_OBJECT
    Q_PROPERTY(QString orientation READ orientation WRITE setOrientation NOTIFY orientationChanged)
    Q_PROPERTY(QString snapshotState READ snapshotState WRITE setSnapshotState NOTIFY snapshotStateChanged)
    Q_PROPERTY(QStringList items READ items WRITE setItems NOTIFY itemsChanged)
    Q_PROPERTY(int currentIndex READ currentIndex WRITE setCurrentIndex NOTIFY currentIndexChanged)
  public:
    explicit Z_AnchorNavigation(
        const QString& profile = QStringLiteral("standard-light"),
        QWidget* parent = nullptr);
    QString orientation() const { return orientationValue; }
    QString snapshotState() const;
    QStringList items() const { return itemValues; }
    int currentIndex() const { return currentIndexValue; }
    QString currentText() const;
    void setOrientation(const QString& value);
    void setSnapshotState(const QString& value);
    void setItems(const QStringList& values);
    void setCurrentIndex(int value);
    QSize sizeHint() const override;
  signals:
    void orientationChanged(const QString& value);
    void snapshotStateChanged(const QString& value);
    void itemsChanged(const QStringList& values);
    void currentIndexChanged(int value);
    void currentTextChanged(const QString& value);
    void anchorActivated(int index, const QString& text);
  protected:
    bool eventFilter(QObject* watched, QEvent* event) override;
    void paintEvent(QPaintEvent*) override;
    void changeEvent(QEvent* event) override;
  private:
    void rebuild();
    void refreshStyle();
    void refreshAccessibleDescription();
    QString profileValue;
    QString orientationValue = QStringLiteral("Vertical");
    QStringList itemValues{QStringLiteral("Overview"), QStringLiteral("Parameters"),
                           QStringLiteral("Results")};
    int currentIndexValue = 0;
    QButtonGroup* group = nullptr;
    QBoxLayout* itemLayout = nullptr;
};

// Filter is a controlled criteria-composition surface. The component owns
// presentation and activation affordances only; validation, persistence,
// apply/clear state transitions and domain values belong to the page's filter
// controller.
class Z_Filter final : public QWidget {
    Q_OBJECT
    Q_PROPERTY(QString layoutMode READ layoutMode WRITE setLayoutMode NOTIFY layoutModeChanged)
    Q_PROPERTY(QString snapshotState READ snapshotState WRITE setSnapshotState NOTIFY snapshotStateChanged)
    Q_PROPERTY(QStringList criteria READ criteria WRITE setCriteria NOTIFY criteriaChanged)
    Q_PROPERTY(QString applyLabel READ applyLabel WRITE setApplyLabel NOTIFY applyLabelChanged)
  public:
    explicit Z_Filter(const QString& profile = QStringLiteral("standard-light"),
                      QWidget* parent = nullptr);
    QString layoutMode() const { return layoutModeValue; }
    QString snapshotState() const { return snapshotStateValue; }
    QStringList criteria() const { return criterionValues; }
    QString applyLabel() const { return applyLabelValue; }
    void setLayoutMode(const QString& value);
    void setSnapshotState(const QString& value);
    void setCriteria(const QStringList& values);
    void setApplyLabel(const QString& value);
    QSize sizeHint() const override;
  signals:
    void layoutModeChanged(const QString& value);
    void snapshotStateChanged(const QString& value);
    void criteriaChanged(const QStringList& values);
    void applyLabelChanged(const QString& value);
    void criterionActivated(int index, const QString& text);
    void applyRequested();
  protected:
    bool eventFilter(QObject* watched, QEvent* event) override;
    void paintEvent(QPaintEvent*) override;
    void resizeEvent(QResizeEvent* event) override;
    void changeEvent(QEvent* event) override;
  private:
    void layoutChildren();
    void refreshStyle();
    void refreshAccessibleDescription();
    QString profileValue;
    QString layoutModeValue = QStringLiteral("Inline");
    QString snapshotStateValue = QStringLiteral("Default");
    QStringList criterionValues{QStringLiteral("Material"), QStringLiteral("Status")};
    QString applyLabelValue = QStringLiteral("Apply");
    QList<QPointer<QToolButton>> criterionButtons;
    QPointer<QToolButton> applyButtonValue;
};

class Z_Pagination final : public QWidget {
    Q_OBJECT
    Q_PROPERTY(QString density READ density WRITE setDensity NOTIFY densityChanged)
    Q_PROPERTY(QString snapshotState READ snapshotState WRITE setSnapshotState NOTIFY snapshotStateChanged)
    Q_PROPERTY(int currentPage READ currentPage WRITE setCurrentPage NOTIFY currentPageChanged)
    Q_PROPERTY(int pageCount READ pageCount WRITE setPageCount NOTIFY pageCountChanged)
  public:
    explicit Z_Pagination(const QString& profile = QStringLiteral("standard-light"),
                          QWidget* parent = nullptr);
    QString density() const { return densityValue; }
    QString snapshotState() const { return snapshotStateValue; }
    int currentPage() const { return currentPageValue; }
    int pageCount() const { return pageCountValue; }
    void setDensity(const QString& value);
    void setSnapshotState(const QString& value);
    void setCurrentPage(int value);
    void setPageCount(int value);
    QSize sizeHint() const override;
  signals:
    void densityChanged(const QString& value);
    void snapshotStateChanged(const QString& value);
    void currentPageChanged(int value);
    void pageCountChanged(int value);
  protected:
    void paintEvent(QPaintEvent*) override;
    void changeEvent(QEvent* event) override;
  private:
    QList<int> displayPages() const;
    void rebuild();
    void refreshStyle();
    QString profileValue;
    QString densityValue = QStringLiteral("Default");
    QString snapshotStateValue = QStringLiteral("Default");
    int currentPageValue = 5;
    int pageCountValue = 29;
    QHBoxLayout* pageLayout = nullptr;
    QList<QToolButton*> pageButtons;
    QList<QLabel*> ellipsisLabels;
};

class Z_PageHeader final : public QWidget {
    Q_OBJECT
    Q_PROPERTY(QString title READ title WRITE setTitle NOTIFY titleChanged)
    Q_PROPERTY(QString contextText READ contextText WRITE setContextText NOTIFY contextTextChanged)
    Q_PROPERTY(QString actionText READ actionText WRITE setActionText NOTIFY actionTextChanged)
    Q_PROPERTY(QString density READ density WRITE setDensity NOTIFY densityChanged)
    Q_PROPERTY(bool actionsVisible READ actionsVisible WRITE setActionsVisible NOTIFY actionsVisibleChanged)
  public:
    explicit Z_PageHeader(const QString& profile = QStringLiteral("standard-light"),
                          QWidget* parent = nullptr);
    QString title() const { return titleValue; }
    QString contextText() const { return contextValue; }
    QString actionText() const { return actionValue; }
    QString density() const { return densityValue; }
    bool actionsVisible() const { return actionsVisibleValue; }
    QToolButton* actionButton() const { return actionControl; }
    void setTitle(const QString& value);
    void setContextText(const QString& value);
    void setActionText(const QString& value);
    void setDensity(const QString& value);
    void setActionsVisible(bool value);
    QSize sizeHint() const override;
  signals:
    void titleChanged(const QString& value);
    void contextTextChanged(const QString& value);
    void actionTextChanged(const QString& value);
    void densityChanged(const QString& value);
    void actionsVisibleChanged(bool value);
    void actionTriggered();
  protected:
    void paintEvent(QPaintEvent*) override;
  private:
    void refreshStyle();
    QString profileValue;
    QString titleValue = QStringLiteral("Process recipe");
    QString contextValue = QStringLiteral("Chamber / Etch / Recipe setup");
    QString actionValue = QStringLiteral("Create");
    QString densityValue = QStringLiteral("Comfortable");
    bool actionsVisibleValue = true;
    QLabel* titleLabel = nullptr;
    QLabel* contextLabel = nullptr;
    QToolButton* actionControl = nullptr;
    QVBoxLayout* headerLayout = nullptr;
};

// The SDK owns the title-bar presentation and action signals. The host window
// decides whether and how to minimize, maximize or close itself.
class Z_TitleBar final : public QWidget {
    Q_OBJECT
    Q_PROPERTY(QString title READ title WRITE setTitle NOTIFY titleChanged)
    Q_PROPERTY(QString snapshotState READ snapshotState WRITE setSnapshotState NOTIFY snapshotStateChanged)
  public:
    explicit Z_TitleBar(const QString& profile = QStringLiteral("standard-light"),
                        QWidget* parent = nullptr);
    QString title() const { return titleValue; }
    QString snapshotState() const { return snapshotStateValue; }
    bool isMaximizedState() const { return snapshotStateValue == QStringLiteral("Maximized"); }
    void setTitle(const QString& value);
    void setSnapshotState(const QString& value);
    QSize sizeHint() const override;
  signals:
    void titleChanged(const QString& value);
    void snapshotStateChanged(const QString& value);
    void minimizeRequested();
    void maximizeRestoreRequested(bool maximize);
    void closeRequested();
    void dragStarted(const QPoint& globalPosition);
    void dragMoved(const QPoint& globalPosition);
    void dragEnded();
  protected:
    void paintEvent(QPaintEvent*) override;
    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;
  private:
    void refreshStyle();
    QString profileValue;
    QString titleValue = QStringLiteral("ComponentLab");
    QString snapshotStateValue = QStringLiteral("Default");
    QLabel* titleLabel = nullptr;
    QToolButton* minimizeButton = nullptr;
    QToolButton* maximizeButton = nullptr;
    QToolButton* closeButton = nullptr;
};

} // namespace commonWidget
