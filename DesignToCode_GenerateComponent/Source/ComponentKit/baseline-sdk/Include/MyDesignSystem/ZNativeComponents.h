#pragma once

// Public baseline SDK surface.  Every class in this file is a direct Qt 5
// QWidget wrapper: it preserves the native constructor/event/model contract
// and contributes only the stable commonWidget::Z_* identity used by QSS.

#include <QtWidgets>

class QString;

namespace commonWidget {

#define Z_NATIVE_COMPONENT_TYPES(X) \
    X(QWidget, Z_Widget) \
    X(QFrame, Z_Frame) \
    X(QPushButton, Z_PushButton) \
    X(QToolButton, Z_ToolButton) \
    X(QRadioButton, Z_RadioButton) \
    X(QCheckBox, Z_CheckBox) \
    X(QCommandLinkButton, Z_CommandLinkButton) \
    X(QDialogButtonBox, Z_DialogButtonBox) \
    X(QLineEdit, Z_LineEdit) \
    X(QSpinBox, Z_SpinBox) \
    X(QDoubleSpinBox, Z_DoubleSpinBox) \
    X(QSlider, Z_Slider) \
    X(QScrollBar, Z_ScrollBar) \
    X(QDial, Z_Dial) \
    X(QComboBox, Z_ComboBox) \
    X(QFontComboBox, Z_FontComboBox) \
    X(QDateTimeEdit, Z_DateTimeEdit) \
    X(QDateEdit, Z_DateEdit) \
    X(QTimeEdit, Z_TimeEdit) \
    X(QKeySequenceEdit, Z_KeySequenceEdit) \
    X(QLabel, Z_Label) \
    X(QLCDNumber, Z_LCDNumber) \
    X(QProgressBar, Z_ProgressBar) \
    X(QTextBrowser, Z_TextBrowser) \
    X(QGraphicsView, Z_GraphicsView) \
    X(QCalendarWidget, Z_CalendarWidget) \
    X(QTextEdit, Z_TextEdit) \
    X(QPlainTextEdit, Z_PlainTextEdit) \
    X(QListView, Z_ListView) \
    X(QTreeView, Z_TreeView) \
    X(QTableView, Z_TableView) \
    X(QColumnView, Z_ColumnView) \
    X(QUndoView, Z_UndoView) \
    X(QListWidget, Z_ListWidget) \
    X(QTreeWidget, Z_TreeWidget) \
    X(QTableWidget, Z_TableWidget) \
    X(QGroupBox, Z_GroupBox) \
    X(QScrollArea, Z_ScrollArea) \
    X(QTabWidget, Z_TabWidget) \
    X(QToolBox, Z_ToolBox) \
    X(QStackedWidget, Z_StackedWidget) \
    X(QSplitter, Z_Splitter) \
    X(QDockWidget, Z_DockWidget) \
    X(QMdiArea, Z_MdiArea) \
    X(QMdiSubWindow, Z_MdiSubWindow) \
    X(QWizard, Z_Wizard) \
    X(QWizardPage, Z_WizardPage) \
    X(QMenu, Z_Menu) \
    X(QMenuBar, Z_MenuBar) \
    X(QToolBar, Z_ToolBar) \
    X(QStatusBar, Z_StatusBar) \
    X(QOpenGLWidget, Z_OpenGLWidget) \
    X(QDialog, Z_Dialog) \
    X(QMessageBox, Z_MessageBox) \
    X(QFileDialog, Z_FileDialog) \
    X(QColorDialog, Z_ColorDialog) \
    X(QFontDialog, Z_FontDialog) \
    X(QInputDialog, Z_InputDialog) \
    X(QProgressDialog, Z_ProgressDialog)

#define Z_DECLARE_NATIVE_WRAPPER(QT_CLASS, SDK_CLASS) \
    class SDK_CLASS : public QT_CLASS { \
      public: \
        explicit SDK_CLASS(QWidget* parent = nullptr); \
    };
Z_NATIVE_COMPONENT_TYPES(Z_DECLARE_NATIVE_WRAPPER)
#undef Z_DECLARE_NATIVE_WRAPPER

// Call after QApplication construction and before applying nativeComponentStyleSheet().
// It registers the package-contained HarmonyOS Sans / HarmonyOS Sans SC fonts
// and initializes the QRC owned by this package.  It is idempotent.
void initializeNativeComponentResources();

// Returns the package-owned, fixed-value standard-light QSS.  It deliberately
// reads no Token JSON and creates no runtime visual-variable carrier.
QString nativeComponentStyleSheet();

}  // namespace commonWidget
