#include "NativeComponentRegistry.h"
#include "NativeComponents.h"

namespace commonWidget {

QList<NativeComponentDescriptor> nativeComponentDescriptors() {
    return {
        {QStringLiteral("QWidget"), QStringLiteral("commonWidget::Z_Widget"), QStringLiteral("ComponentLab/Widget"), QStringLiteral("visual-component")},
        {QStringLiteral("QFrame"), QStringLiteral("commonWidget::Z_Frame"), QStringLiteral("ComponentLab/Frame"), QStringLiteral("visual-component")},
        {QStringLiteral("QPushButton"), QStringLiteral("commonWidget::Z_PushButton"), QStringLiteral("ComponentLab/Button"), QStringLiteral("visual-component")},
        {QStringLiteral("QToolButton"), QStringLiteral("commonWidget::Z_ToolButton"), QStringLiteral("ComponentLab/ToolButton"), QStringLiteral("visual-component")},
        {QStringLiteral("QRadioButton"), QStringLiteral("commonWidget::Z_RadioButton"), QStringLiteral("ComponentLab/RadioButton"), QStringLiteral("visual-component")},
        {QStringLiteral("QCheckBox"), QStringLiteral("commonWidget::Z_CheckBox"), QStringLiteral("ComponentLab/Checkbox"), QStringLiteral("visual-component")},
        {QStringLiteral("QCommandLinkButton"), QStringLiteral("commonWidget::Z_CommandLinkButton"), QStringLiteral("ComponentLab/CommandLinkButton"), QStringLiteral("visual-component")},
        {QStringLiteral("QDialogButtonBox"), QStringLiteral("commonWidget::Z_DialogButtonBox"), QStringLiteral("ComponentLab/DialogButtonGroup"), QStringLiteral("visual-component")},
        {QStringLiteral("QLineEdit"), QStringLiteral("commonWidget::Z_LineEdit"), QStringLiteral("ComponentLab/LineEdit"), QStringLiteral("visual-component")},
        {QStringLiteral("QSpinBox"), QStringLiteral("commonWidget::Z_SpinBox"), QStringLiteral("ComponentLab/SpinBox"), QStringLiteral("visual-component")},
        {QStringLiteral("QDoubleSpinBox"), QStringLiteral("commonWidget::Z_DoubleSpinBox"), QStringLiteral("ComponentLab/DoubleSpinBox"), QStringLiteral("visual-component")},
        {QStringLiteral("QSlider"), QStringLiteral("commonWidget::Z_Slider"), QStringLiteral("ComponentLab/Slider"), QStringLiteral("visual-component")},
        {QStringLiteral("QScrollBar"), QStringLiteral("commonWidget::Z_ScrollBar"), QStringLiteral("ComponentLab/ScrollBar"), QStringLiteral("visual-component")},
        {QStringLiteral("QDial"), QStringLiteral("commonWidget::Z_Dial"), QStringLiteral("ComponentLab/Dial"), QStringLiteral("visual-component")},
        {QStringLiteral("QComboBox"), QStringLiteral("commonWidget::Z_ComboBox"), QStringLiteral("ComponentLab/ComboBox"), QStringLiteral("visual-component")},
        {QStringLiteral("QFontComboBox"), QStringLiteral("commonWidget::Z_FontComboBox"), QStringLiteral("ComponentLab/FontComboBox"), QStringLiteral("visual-component")},
        {QStringLiteral("QDateTimeEdit"), QStringLiteral("commonWidget::Z_DateTimeEdit"), QStringLiteral("ComponentLab/DateTimePicker"), QStringLiteral("visual-component")},
        {QStringLiteral("QDateEdit"), QStringLiteral("commonWidget::Z_DateEdit"), QStringLiteral("ComponentLab/DatePicker"), QStringLiteral("visual-component")},
        {QStringLiteral("QTimeEdit"), QStringLiteral("commonWidget::Z_TimeEdit"), QStringLiteral("ComponentLab/TimePicker"), QStringLiteral("visual-component")},
        {QStringLiteral("QKeySequenceEdit"), QStringLiteral("commonWidget::Z_KeySequenceEdit"), QStringLiteral("ComponentLab/ShortcutInput"), QStringLiteral("visual-component")},
        {QStringLiteral("QLabel"), QStringLiteral("commonWidget::Z_Label"), QStringLiteral("ComponentLab/Label"), QStringLiteral("visual-component")},
        {QStringLiteral("QLCDNumber"), QStringLiteral("commonWidget::Z_LCDNumber"), QStringLiteral("ComponentLab/LCDNumber"), QStringLiteral("visual-component")},
        {QStringLiteral("QProgressBar"), QStringLiteral("commonWidget::Z_ProgressBar"), QStringLiteral("ComponentLab/ProgressBar"), QStringLiteral("visual-component")},
        {QStringLiteral("QTextBrowser"), QStringLiteral("commonWidget::Z_TextBrowser"), QStringLiteral("ComponentLab/TextBrowser"), QStringLiteral("visual-component")},
        {QStringLiteral("QGraphicsView"), QStringLiteral("commonWidget::Z_GraphicsView"), QStringLiteral("ComponentLab/GraphicsView"), QStringLiteral("visual-component")},
        {QStringLiteral("QCalendarWidget"), QStringLiteral("commonWidget::Z_CalendarWidget"), QStringLiteral("ComponentLab/Calendar"), QStringLiteral("visual-component")},
        {QStringLiteral("QTextEdit"), QStringLiteral("commonWidget::Z_TextEdit"), QStringLiteral("ComponentLab/RichTextEdit"), QStringLiteral("visual-component")},
        {QStringLiteral("QPlainTextEdit"), QStringLiteral("commonWidget::Z_PlainTextEdit"), QStringLiteral("ComponentLab/PlainTextEdit"), QStringLiteral("visual-component")},
        {QStringLiteral("QListView"), QStringLiteral("commonWidget::Z_ListView"), QStringLiteral("ComponentLab/ListView"), QStringLiteral("visual-component")},
        {QStringLiteral("QTreeView"), QStringLiteral("commonWidget::Z_TreeView"), QStringLiteral("ComponentLab/TreeView"), QStringLiteral("visual-component")},
        {QStringLiteral("QTableView"), QStringLiteral("commonWidget::Z_TableView"), QStringLiteral("ComponentLab/TableView"), QStringLiteral("visual-component")},
        {QStringLiteral("QColumnView"), QStringLiteral("commonWidget::Z_ColumnView"), QStringLiteral("ComponentLab/ColumnView"), QStringLiteral("visual-component")},
        {QStringLiteral("QUndoView"), QStringLiteral("commonWidget::Z_UndoView"), QStringLiteral("ComponentLab/UndoView"), QStringLiteral("visual-component")},
        {QStringLiteral("QListWidget"), QStringLiteral("commonWidget::Z_ListWidget"), QStringLiteral("ComponentLab/ListWidget"), QStringLiteral("visual-component")},
        {QStringLiteral("QTreeWidget"), QStringLiteral("commonWidget::Z_TreeWidget"), QStringLiteral("ComponentLab/TreeWidget"), QStringLiteral("visual-component")},
        {QStringLiteral("QTableWidget"), QStringLiteral("commonWidget::Z_TableWidget"), QStringLiteral("ComponentLab/TableWidget"), QStringLiteral("visual-component")},
        {QStringLiteral("QGroupBox"), QStringLiteral("commonWidget::Z_GroupBox"), QStringLiteral("ComponentLab/GroupBox"), QStringLiteral("visual-component")},
        {QStringLiteral("QScrollArea"), QStringLiteral("commonWidget::Z_ScrollArea"), QStringLiteral("ComponentLab/ScrollArea"), QStringLiteral("visual-component")},
        {QStringLiteral("QTabWidget"), QStringLiteral("commonWidget::Z_TabWidget"), QStringLiteral("ComponentLab/TabWidget"), QStringLiteral("visual-component")},
        {QStringLiteral("QToolBox"), QStringLiteral("commonWidget::Z_ToolBox"), QStringLiteral("ComponentLab/ToolBox"), QStringLiteral("visual-component")},
        {QStringLiteral("QStackedWidget"), QStringLiteral("commonWidget::Z_StackedWidget"), QStringLiteral("ComponentLab/StackedWidget"), QStringLiteral("visual-component")},
        {QStringLiteral("QSplitter"), QStringLiteral("commonWidget::Z_Splitter"), QStringLiteral("ComponentLab/Splitter"), QStringLiteral("visual-component")},
        {QStringLiteral("QDockWidget"), QStringLiteral("commonWidget::Z_DockWidget"), QStringLiteral("ComponentLab/DockWidget"), QStringLiteral("visual-component")},
        {QStringLiteral("QMdiArea"), QStringLiteral("commonWidget::Z_MdiArea"), QStringLiteral("ComponentLab/MdiArea"), QStringLiteral("visual-component")},
        {QStringLiteral("QMdiSubWindow"), QStringLiteral("commonWidget::Z_MdiSubWindow"), QStringLiteral("ComponentLab/MdiSubWindow"), QStringLiteral("visual-component")},
        {QStringLiteral("QWizard"), QStringLiteral("commonWidget::Z_Wizard"), QStringLiteral("ComponentLab/Wizard"), QStringLiteral("visual-component")},
        {QStringLiteral("QWizardPage"), QStringLiteral("commonWidget::Z_WizardPage"), QStringLiteral("ComponentLab/WizardPage"), QStringLiteral("visual-component")},
        {QStringLiteral("QMenu"), QStringLiteral("commonWidget::Z_Menu"), QStringLiteral("ComponentLab/Menu"), QStringLiteral("visual-component")},
        {QStringLiteral("QMenuBar"), QStringLiteral("commonWidget::Z_MenuBar"), QStringLiteral("ComponentLab/MenuBar"), QStringLiteral("visual-component")},
        {QStringLiteral("QToolBar"), QStringLiteral("commonWidget::Z_ToolBar"), QStringLiteral("ComponentLab/ToolBar"), QStringLiteral("visual-component")},
        {QStringLiteral("QStatusBar"), QStringLiteral("commonWidget::Z_StatusBar"), QStringLiteral("ComponentLab/StatusBar"), QStringLiteral("visual-component")},
        {QStringLiteral("QOpenGLWidget"), QStringLiteral("commonWidget::Z_OpenGLWidget"), QStringLiteral("ComponentLab/OpenGLViewport"), QStringLiteral("visual-component")},
        {QStringLiteral("QDialog"), QStringLiteral("commonWidget::Z_Dialog"), QStringLiteral("ComponentLab/Dialog"), QStringLiteral("visual-component")},
        {QStringLiteral("QMessageBox"), QStringLiteral("commonWidget::Z_MessageBox"), QStringLiteral("ComponentLab/MessageDialog"), QStringLiteral("visual-component")},
        {QStringLiteral("QFileDialog"), QStringLiteral("commonWidget::Z_FileDialog"), QStringLiteral("ComponentLab/FileDialog"), QStringLiteral("visual-component")},
        {QStringLiteral("QColorDialog"), QStringLiteral("commonWidget::Z_ColorDialog"), QStringLiteral("ComponentLab/ColorDialog"), QStringLiteral("visual-component")},
        {QStringLiteral("QFontDialog"), QStringLiteral("commonWidget::Z_FontDialog"), QStringLiteral("ComponentLab/FontDialog"), QStringLiteral("visual-component")},
        {QStringLiteral("QInputDialog"), QStringLiteral("commonWidget::Z_InputDialog"), QStringLiteral("ComponentLab/InputDialog"), QStringLiteral("visual-component")},
        {QStringLiteral("QProgressDialog"), QStringLiteral("commonWidget::Z_ProgressDialog"), QStringLiteral("ComponentLab/ProgressDialog"), QStringLiteral("visual-component")},
    };
}

QWidget* createGeneratedNativeComponent(const QString& sdkClass, QWidget* parent) {
    if (sdkClass == QStringLiteral("commonWidget::Z_Widget")) return new Z_Widget(parent);
    if (sdkClass == QStringLiteral("commonWidget::Z_Frame")) return new Z_Frame(parent);
    if (sdkClass == QStringLiteral("commonWidget::Z_ToolButton")) return new Z_ToolButton(parent);
    if (sdkClass == QStringLiteral("commonWidget::Z_RadioButton")) return new Z_RadioButton(parent);
    if (sdkClass == QStringLiteral("commonWidget::Z_CommandLinkButton")) return new Z_CommandLinkButton(parent);
    if (sdkClass == QStringLiteral("commonWidget::Z_DialogButtonBox")) return new Z_DialogButtonBox(parent);
    if (sdkClass == QStringLiteral("commonWidget::Z_LineEdit")) return new Z_LineEdit(parent);
    if (sdkClass == QStringLiteral("commonWidget::Z_SpinBox")) return new Z_SpinBox(parent);
    if (sdkClass == QStringLiteral("commonWidget::Z_DoubleSpinBox")) return new Z_DoubleSpinBox(parent);
    if (sdkClass == QStringLiteral("commonWidget::Z_Slider")) return new Z_Slider(parent);
    if (sdkClass == QStringLiteral("commonWidget::Z_ScrollBar")) return new Z_ScrollBar(parent);
    if (sdkClass == QStringLiteral("commonWidget::Z_Dial")) return new Z_Dial(parent);
    if (sdkClass == QStringLiteral("commonWidget::Z_ComboBox")) return new Z_ComboBox(parent);
    if (sdkClass == QStringLiteral("commonWidget::Z_FontComboBox")) return new Z_FontComboBox(parent);
    if (sdkClass == QStringLiteral("commonWidget::Z_DateTimeEdit")) return new Z_DateTimeEdit(parent);
    if (sdkClass == QStringLiteral("commonWidget::Z_DateEdit")) return new Z_DateEdit(parent);
    if (sdkClass == QStringLiteral("commonWidget::Z_TimeEdit")) return new Z_TimeEdit(parent);
    if (sdkClass == QStringLiteral("commonWidget::Z_KeySequenceEdit")) return new Z_KeySequenceEdit(parent);
    if (sdkClass == QStringLiteral("commonWidget::Z_Label")) return new Z_Label(parent);
    if (sdkClass == QStringLiteral("commonWidget::Z_LCDNumber")) return new Z_LCDNumber(parent);
    if (sdkClass == QStringLiteral("commonWidget::Z_ProgressBar")) return new Z_ProgressBar(parent);
    if (sdkClass == QStringLiteral("commonWidget::Z_TextBrowser")) return new Z_TextBrowser(parent);
    if (sdkClass == QStringLiteral("commonWidget::Z_GraphicsView")) return new Z_GraphicsView(parent);
    if (sdkClass == QStringLiteral("commonWidget::Z_CalendarWidget")) return new Z_CalendarWidget(parent);
    if (sdkClass == QStringLiteral("commonWidget::Z_TextEdit")) return new Z_TextEdit(parent);
    if (sdkClass == QStringLiteral("commonWidget::Z_PlainTextEdit")) return new Z_PlainTextEdit(parent);
    if (sdkClass == QStringLiteral("commonWidget::Z_ListView")) return new Z_ListView(parent);
    if (sdkClass == QStringLiteral("commonWidget::Z_TreeView")) return new Z_TreeView(parent);
    if (sdkClass == QStringLiteral("commonWidget::Z_TableView")) return new Z_TableView(parent);
    if (sdkClass == QStringLiteral("commonWidget::Z_ColumnView")) return new Z_ColumnView(parent);
    if (sdkClass == QStringLiteral("commonWidget::Z_UndoView")) return new Z_UndoView(parent);
    if (sdkClass == QStringLiteral("commonWidget::Z_ListWidget")) return new Z_ListWidget(parent);
    if (sdkClass == QStringLiteral("commonWidget::Z_TreeWidget")) return new Z_TreeWidget(parent);
    if (sdkClass == QStringLiteral("commonWidget::Z_TableWidget")) return new Z_TableWidget(parent);
    if (sdkClass == QStringLiteral("commonWidget::Z_GroupBox")) return new Z_GroupBox(parent);
    if (sdkClass == QStringLiteral("commonWidget::Z_ScrollArea")) return new Z_ScrollArea(parent);
    if (sdkClass == QStringLiteral("commonWidget::Z_TabWidget")) return new Z_TabWidget(parent);
    if (sdkClass == QStringLiteral("commonWidget::Z_ToolBox")) return new Z_ToolBox(parent);
    if (sdkClass == QStringLiteral("commonWidget::Z_StackedWidget")) return new Z_StackedWidget(parent);
    if (sdkClass == QStringLiteral("commonWidget::Z_Splitter")) return new Z_Splitter(parent);
    if (sdkClass == QStringLiteral("commonWidget::Z_DockWidget")) return new Z_DockWidget(parent);
    if (sdkClass == QStringLiteral("commonWidget::Z_MdiArea")) return new Z_MdiArea(parent);
    if (sdkClass == QStringLiteral("commonWidget::Z_MdiSubWindow")) return new Z_MdiSubWindow(parent);
    if (sdkClass == QStringLiteral("commonWidget::Z_Wizard")) return new Z_Wizard(parent);
    if (sdkClass == QStringLiteral("commonWidget::Z_WizardPage")) return new Z_WizardPage(parent);
    if (sdkClass == QStringLiteral("commonWidget::Z_Menu")) return new Z_Menu(parent);
    if (sdkClass == QStringLiteral("commonWidget::Z_MenuBar")) return new Z_MenuBar(parent);
    if (sdkClass == QStringLiteral("commonWidget::Z_ToolBar")) return new Z_ToolBar(parent);
    if (sdkClass == QStringLiteral("commonWidget::Z_StatusBar")) return new Z_StatusBar(parent);
    if (sdkClass == QStringLiteral("commonWidget::Z_OpenGLWidget")) return new Z_OpenGLWidget(parent);
    if (sdkClass == QStringLiteral("commonWidget::Z_Dialog")) return new Z_Dialog(parent);
    if (sdkClass == QStringLiteral("commonWidget::Z_MessageBox")) return new Z_MessageBox(parent);
    if (sdkClass == QStringLiteral("commonWidget::Z_FileDialog")) return new Z_FileDialog(parent);
    if (sdkClass == QStringLiteral("commonWidget::Z_ColorDialog")) return new Z_ColorDialog(parent);
    if (sdkClass == QStringLiteral("commonWidget::Z_FontDialog")) return new Z_FontDialog(parent);
    if (sdkClass == QStringLiteral("commonWidget::Z_InputDialog")) return new Z_InputDialog(parent);
    if (sdkClass == QStringLiteral("commonWidget::Z_ProgressDialog")) return new Z_ProgressDialog(parent);
    return nullptr;
}

} // namespace commonWidget
