"""Build the project-owned Qt capability catalog from the reviewed reference document.

The Markdown file is an authoring input only. The generated catalog is the stable
project contract consumed by C++, tests, Pixso publishing, and SDK packaging.
"""

from __future__ import annotations

import argparse
import hashlib
import json
import re
from pathlib import Path


SECTION_CATEGORY = {
    "根基类": "foundation",
    "QWidget 体系": "foundation",
    "布局类": "layout",
    "按钮类": "button",
    "输入类": "input",
    "显示类": "display",
    "文本编辑类": "text-editor",
    "视图类": "item-view",
    "项组件类": "item-widget",
    "容器类": "container",
    "菜单 / 工具栏 / 动作类": "command",
    "模型类": "model",
    "图形 / 绘图类": "graphics",
    "对话框类": "dialog",
    "其他常用类": "support",
}

ABSTRACT_CLASSES = {
    "QObject",
    "QLayout",
    "QAbstractButton",
    "QAbstractSpinBox",
    "QAbstractSlider",
    "QAbstractItemView",
    "QAbstractItemModel",
}

APPLICATION_CLASSES = {"QCoreApplication", "QGuiApplication", "QApplication"}
LAYOUT_CLASSES = {
    "QLayout",
    "QBoxLayout",
    "QHBoxLayout",
    "QVBoxLayout",
    "QGridLayout",
    "QFormLayout",
    "QStackedLayout",
}
MODEL_CLASSES = {
    "QAbstractItemModel",
    "QStandardItemModel",
    "QStringListModel",
    "QFileSystemModel",
    "QSortFilterProxyModel",
}
BEHAVIOR_CLASSES = {"QAction", "QGraphicsScene"}
SUPPORT_CLASSES = {"QSizePolicy", "QPalette", "QFont", "QCursor", "QKeySequence"}

# Every concrete QWidget in the supplied reference receives a public Z_* wrapper.
VISUAL_CLASSES = {
    "QWidget", "QFrame", "QPushButton", "QToolButton", "QRadioButton", "QCheckBox",
    "QCommandLinkButton", "QDialogButtonBox", "QLineEdit", "QSpinBox", "QDoubleSpinBox",
    "QSlider", "QScrollBar", "QDial", "QComboBox", "QFontComboBox", "QDateTimeEdit",
    "QDateEdit", "QTimeEdit", "QKeySequenceEdit", "QLabel", "QLCDNumber", "QProgressBar",
    "QTextBrowser", "QGraphicsView", "QCalendarWidget", "QTextEdit", "QPlainTextEdit",
    "QListView", "QTreeView", "QTableView", "QColumnView", "QUndoView", "QListWidget",
    "QTreeWidget", "QTableWidget", "QGroupBox", "QScrollArea", "QTabWidget", "QToolBox",
    "QStackedWidget", "QSplitter", "QDockWidget", "QMdiArea", "QMdiSubWindow", "QWizard",
    "QWizardPage", "QMenu", "QMenuBar", "QToolBar", "QStatusBar", "QOpenGLWidget",
    "QDialog", "QMessageBox", "QFileDialog", "QColorDialog", "QFontDialog", "QInputDialog",
    "QProgressDialog",
}

BASE_CLASS = {
    "QCoreApplication": "QObject", "QGuiApplication": "QCoreApplication",
    "QApplication": "QGuiApplication", "QWidget": "QObject", "QFrame": "QWidget",
    "QLayout": "QObject", "QBoxLayout": "QLayout", "QHBoxLayout": "QBoxLayout",
    "QVBoxLayout": "QBoxLayout", "QGridLayout": "QLayout", "QFormLayout": "QLayout",
    "QStackedLayout": "QLayout", "QAbstractButton": "QWidget", "QPushButton": "QAbstractButton",
    "QToolButton": "QAbstractButton", "QRadioButton": "QAbstractButton", "QCheckBox": "QAbstractButton",
    "QCommandLinkButton": "QPushButton", "QDialogButtonBox": "QWidget", "QLineEdit": "QWidget",
    "QAbstractSpinBox": "QWidget", "QSpinBox": "QAbstractSpinBox",
    "QDoubleSpinBox": "QAbstractSpinBox", "QAbstractSlider": "QWidget", "QSlider": "QAbstractSlider",
    "QScrollBar": "QAbstractSlider", "QDial": "QAbstractSlider", "QComboBox": "QWidget",
    "QFontComboBox": "QComboBox", "QDateTimeEdit": "QAbstractSpinBox", "QDateEdit": "QDateTimeEdit",
    "QTimeEdit": "QDateTimeEdit", "QKeySequenceEdit": "QWidget", "QLabel": "QFrame",
    "QLCDNumber": "QFrame", "QProgressBar": "QWidget", "QTextBrowser": "QTextEdit",
    "QGraphicsView": "QAbstractScrollArea", "QCalendarWidget": "QWidget", "QTextEdit": "QAbstractScrollArea",
    "QPlainTextEdit": "QAbstractScrollArea", "QAbstractItemView": "QAbstractScrollArea",
    "QListView": "QAbstractItemView", "QTreeView": "QAbstractItemView", "QTableView": "QAbstractItemView",
    "QColumnView": "QAbstractItemView", "QUndoView": "QListView", "QListWidget": "QListView",
    "QTreeWidget": "QTreeView", "QTableWidget": "QTableView", "QGroupBox": "QWidget",
    "QScrollArea": "QAbstractScrollArea", "QTabWidget": "QWidget", "QToolBox": "QFrame",
    "QStackedWidget": "QFrame", "QSplitter": "QFrame", "QDockWidget": "QWidget",
    "QMdiArea": "QAbstractScrollArea", "QMdiSubWindow": "QWidget", "QWizard": "QDialog",
    "QWizardPage": "QWidget", "QAction": "QObject", "QMenu": "QWidget", "QMenuBar": "QWidget",
    "QToolBar": "QWidget", "QStatusBar": "QWidget", "QAbstractItemModel": "QObject",
    "QStandardItemModel": "QAbstractItemModel", "QStringListModel": "QAbstractItemModel",
    "QFileSystemModel": "QAbstractItemModel", "QSortFilterProxyModel": "QAbstractItemModel",
    "QGraphicsScene": "QObject", "QOpenGLWidget": "QWidget", "QDialog": "QWidget",
    "QMessageBox": "QDialog", "QFileDialog": "QDialog", "QColorDialog": "QDialog",
    "QFontDialog": "QDialog", "QInputDialog": "QDialog", "QProgressDialog": "QDialog",
}

DESIGN_NAME = {
    "QPushButton": "Button", "QToolButton": "ToolButton", "QRadioButton": "RadioButton",
    "QCheckBox": "Checkbox", "QCommandLinkButton": "CommandLinkButton",
    "QDialogButtonBox": "DialogButtonGroup", "QLineEdit": "LineEdit", "QSpinBox": "SpinBox",
    "QDoubleSpinBox": "DoubleSpinBox", "QSlider": "Slider", "QScrollBar": "ScrollBar",
    "QDial": "Dial", "QComboBox": "ComboBox", "QFontComboBox": "FontComboBox",
    "QDateTimeEdit": "DateTimePicker", "QDateEdit": "DatePicker", "QTimeEdit": "TimePicker",
    "QKeySequenceEdit": "ShortcutInput", "QLabel": "Label", "QLCDNumber": "LCDNumber",
    "QProgressBar": "ProgressBar", "QTextBrowser": "TextBrowser", "QGraphicsView": "GraphicsView",
    "QCalendarWidget": "Calendar", "QTextEdit": "RichTextEdit", "QPlainTextEdit": "PlainTextEdit",
    "QListView": "ListView", "QTreeView": "TreeView", "QTableView": "TableView",
    "QColumnView": "ColumnView", "QUndoView": "UndoView", "QListWidget": "ListWidget",
    "QTreeWidget": "TreeWidget", "QTableWidget": "TableWidget", "QGroupBox": "GroupBox",
    "QScrollArea": "ScrollArea", "QTabWidget": "TabWidget", "QToolBox": "ToolBox",
    "QStackedWidget": "StackedWidget", "QSplitter": "Splitter", "QDockWidget": "DockWidget",
    "QMdiArea": "MdiArea", "QMdiSubWindow": "MdiSubWindow", "QWizard": "Wizard",
    "QWizardPage": "WizardPage", "QMenu": "Menu", "QMenuBar": "MenuBar",
    "QToolBar": "ToolBar", "QStatusBar": "StatusBar", "QOpenGLWidget": "OpenGLViewport",
    "QDialog": "Dialog", "QMessageBox": "MessageDialog", "QFileDialog": "FileDialog",
    "QColorDialog": "ColorDialog", "QFontDialog": "FontDialog", "QInputDialog": "InputDialog",
    "QProgressDialog": "ProgressDialog", "QWidget": "Widget", "QFrame": "Frame",
}

# Ant Design is a semantic and interaction reference, not a runtime dependency.
ANT_REFERENCE = {
    "QPushButton": ("Button", "direct"), "QToolButton": ("Button", "adapted"),
    "QRadioButton": ("Radio", "direct"), "QCheckBox": ("Checkbox", "direct"),
    "QCommandLinkButton": ("Button", "adapted"), "QDialogButtonBox": ("Space+Button", "adapted"),
    "QLineEdit": ("Input", "direct"), "QSpinBox": ("InputNumber", "direct"),
    "QDoubleSpinBox": ("InputNumber", "direct"), "QSlider": ("Slider", "direct"),
    "QDial": ("Slider", "adapted"), "QComboBox": ("Select", "direct"),
    "QFontComboBox": ("Select", "adapted"), "QDateTimeEdit": ("DatePicker", "adapted"),
    "QDateEdit": ("DatePicker", "direct"), "QTimeEdit": ("TimePicker", "direct"),
    "QKeySequenceEdit": ("Input", "adapted"), "QLabel": ("Typography", "direct"),
    "QLCDNumber": ("Statistic", "adapted"), "QProgressBar": ("Progress", "direct"),
    "QTextBrowser": ("Typography", "adapted"), "QCalendarWidget": ("Calendar", "direct"),
    "QTextEdit": ("Input.TextArea", "adapted"), "QPlainTextEdit": ("Input.TextArea", "direct"),
    "QListView": ("Listy", "adapted"), "QTreeView": ("Tree", "direct"),
    "QTableView": ("Table", "direct"), "QColumnView": ("Cascader", "adapted"),
    "QListWidget": ("Listy", "adapted"), "QTreeWidget": ("Tree", "adapted"),
    "QTableWidget": ("Table", "adapted"), "QGroupBox": ("Card", "adapted"),
    "QScrollArea": ("Layout", "adapted"), "QTabWidget": ("Tabs", "direct"),
    "QToolBox": ("Collapse", "direct"), "QStackedWidget": ("Tabs", "adapted"),
    "QSplitter": ("Splitter", "direct"), "QDockWidget": ("Drawer", "adapted"),
    "QMdiSubWindow": ("Card", "adapted"), "QWizard": ("Steps+Form", "adapted"),
    "QWizardPage": ("Form", "adapted"), "QMenu": ("Menu", "direct"),
    "QMenuBar": ("Menu", "adapted"), "QToolBar": ("Space+Button", "adapted"),
    "QStatusBar": ("App", "adapted"), "QDialog": ("Modal", "direct"),
    "QMessageBox": ("Modal", "adapted"), "QFileDialog": ("Upload", "adapted"),
    "QColorDialog": ("ColorPicker", "adapted"), "QFontDialog": ("Select", "adapted"),
    "QInputDialog": ("Modal+Input", "adapted"), "QProgressDialog": ("Modal+Progress", "adapted"),
}

READ_ONLY = {"displayText", "hasSelectedText", "selectedText", "isUndoAvailable", "isRedoAvailable"}
STATE_SPECIMENS = {
    "QProgressBar": ["determinate", "indeterminate", "disabled"],
    "QScrollBar": ["default", "hover", "dragging", "disabled"],
    "QSlider": ["default", "focus", "dragging", "disabled"],
    "QDial": ["default", "focus", "dragging", "disabled"],
}
STATE_BOUNDARIES = {
    "QProgressBar": (
        "Figma State values are static design specimens, not Qt writable properties; "
        "hover, pressed and focus are not a QProgressBar interaction contract. "
        "Indeterminate operation and QWidget disabled behavior require runtime verification."
    ),
    "QScrollBar": (
        "Figma Hover and Dragging are visual specimens, not a standalone Qt state enum. "
        "QAbstractSlider sliderDown and QWidget enabled remain runtime behavior; "
        "QStyle subcontrols, keyboard and RTL require separate verification."
    ),
    "QSlider": (
        "Figma Focus and Dragging are visual specimens, not extra writable QSlider properties. "
        "Inherited QAbstractSlider sliderDown is DESIGNABLE false; focus, keyboard "
        "and enabled behavior require runtime verification."
    ),
    "QDial": (
        "Figma Focus and Dragging are visual specimens, not extra writable QDial properties. "
        "QDial wrapping is an independent native Q_PROPERTY; inherited sliderDown "
        "and focus/keyboard behavior require runtime verification."
    ),
}
RUNTIME = {"sliderDown", "underMouse", "hasFocus", "activeWindow", "visibleRegion"}
STRUCTURE = {
    "geometry", "sizePolicy", "minimumSize", "maximumSize", "margin", "contentsMargins",
    "spacing", "horizontalSpacing", "verticalSpacing", "stretch", "rowStretch", "columnStretch",
    "rowMinimumHeight", "columnMinimumWidth", "direction", "layoutDirection", "sizeConstraint",
    "fieldGrowthPolicy", "rowWrapPolicy", "labelAlignment", "formAlignment", "orientation",
}
TOKEN = {
    "font", "iconSize", "cursor", "styleSheet", "autoFillBackground", "windowOpacity",
    "frameShape", "frameShadow", "lineWidth", "midLineWidth", "frame", "textMargins",
    "tickPosition", "tickInterval", "alignment", "centerButtons", "documentMargin",
}
BUSINESS = {
    "model", "selectionModel", "rootModelIndex", "currentIndex", "currentRow", "currentColumn",
    "sortingEnabled", "filterRole", "filterKeyColumn", "sourceModel", "filePath", "nameFilters",
    "history", "undoStack", "menu", "validator", "completer",
}


def classify_property(name: str) -> str:
    if name in READ_ONLY:
        return "read-only"
    if name in RUNTIME:
        return "runtime-state"
    if name in STRUCTURE:
        return "structure"
    if name in TOKEN:
        return "token"
    if name in BUSINESS:
        return "business-data"
    if name in {"enabled", "checked", "readOnly", "visible", "loading"}:
        return "design-state"
    return "instance-property"


def mapping_kind(name: str) -> str:
    if name in APPLICATION_CLASSES:
        return "application-runtime"
    if name in LAYOUT_CLASSES:
        return "layout"
    if name in MODEL_CLASSES:
        return "data-model"
    if name in BEHAVIOR_CLASSES:
        return "behavior"
    if name in SUPPORT_CLASSES:
        return "value-adapter"
    if name in ABSTRACT_CLASSES:
        return "abstract-base"
    if name in VISUAL_CLASSES:
        return "visual-component"
    return "support"


def split_property_names(raw: str) -> list[str]:
    names = []
    for part in re.split(r"\s*/\s*", raw.replace("`", "")):
        name = re.sub(r"\s+", "", part)
        if re.fullmatch(r"[A-Za-z_][A-Za-z0-9_]*", name):
            names.append(name)
    return names or [raw.strip()]


def parse_reference(text: str) -> list[dict]:
    current_section = ""
    current_classes: list[str] = []
    records: dict[str, dict] = {}
    for line in text.splitlines():
        if line.startswith("## "):
            current_section = re.sub(r"^##\s+(?:[一二三四五六七八九十]+、)?", "", line).strip()
            current_classes = []
            continue
        if line.startswith("### "):
            heading = line[4:].strip()
            current_classes = re.findall(r"\bQ[A-Z][A-Za-z0-9_]+\b", heading)
            for class_name in current_classes:
                records.setdefault(class_name, {
                    "qtClass": class_name,
                    "section": current_section,
                    "category": SECTION_CATEGORY.get(current_section, "support"),
                    "declaredProperties": [],
                })
            continue
        if not current_classes or not line.startswith("|") or "---" in line or "属性名" in line:
            continue
        cells = [cell.strip() for cell in line.strip().strip("|").split("|")]
        if len(cells) < 3:
            continue
        for property_name in split_property_names(cells[0]):
            item = {
                "name": property_name,
                "declaredType": cells[1],
                "description": cells[2],
                "target": classify_property(property_name),
                "metaVerification": "pending",
            }
            for class_name in current_classes:
                if not any(p["name"] == property_name for p in records[class_name]["declaredProperties"]):
                    records[class_name]["declaredProperties"].append(item.copy())
    return list(records.values())


def enrich(records: list[dict]) -> list[dict]:
    for record in records:
        name = record["qtClass"]
        kind = mapping_kind(name)
        record.update({
            "id": f"qt.{record['category']}.{name}",
            "baseClass": BASE_CLASS.get(name),
            "mappingKind": kind,
            "abstract": name in ABSTRACT_CLASSES,
            "owner": "GenerateLayout" if kind == "layout" else "GenerateComponent",
            "sdkClass": f"commonWidget::Z_{name[1:]}" if kind == "visual-component" else None,
            "designComponent": f"ComponentLab/{DESIGN_NAME[name]}" if name in DESIGN_NAME else None,
            "designExposure": "component-set" if kind == "visual-component" else "engineering-contract",
            "states": (["default", "hover", "pressed", "focus", "disabled"]
                       if kind == "visual-component" else []),
            "slots": ["LABEL", "LEADING", "TRAILING", "CONTENT"] if record["category"] in {"button", "input"} else ["CONTENT"],
            "tokenBindings": ({
                "font": "semantic.typography.control",
                "foreground": "semantic.color.text.primary",
                "background": "semantic.color.background.elevated",
                "border": "semantic.color.border.default",
                "radius": "semantic.radius.control-md",
                "minimumHeight": "semantic.dimension.control.height-md",
            } if kind == "visual-component" else {}),
        })
        if name in STATE_SPECIMENS:
            record["states"] = STATE_SPECIMENS[name]
            record["stateBoundary"] = STATE_BOUNDARIES[name]
        if name == "QProgressBar":
            # Qt 5.15.2 qprogressbar.h declares text without a WRITE accessor.
            text_property = next((item for item in record["declaredProperties"]
                                  if item["name"] == "text"), None)
            if text_property is None:
                record["declaredProperties"].append({
                    "name": "text",
                    "declaredType": "QString",
                    "description": "格式化后的显示文本（只读）",
                    "target": "read-only",
                    "metaVerification": "qt-5.15.2-header",
                })
            else:
                text_property.update({"target": "read-only", "metaVerification": "qt-5.15.2-header"})
        if name in STATE_SPECIMENS:
            record["designReference"] = {
                "system": "DSM",
                "component": f"ComponentLab/{DESIGN_NAME[name]}",
                "equivalence": "project-defined",
            }
        else:
            ant = ANT_REFERENCE.get(name)
            record["designReference"] = ({
                "system": "Ant Design",
                "versionObserved": "6.6.5",
                "component": ant[0],
                "equivalence": ant[1],
                "url": "https://ant.design/components/overview-cn/",
            } if ant else {
                "system": "Qt Native Extension",
                "equivalence": "project-defined",
                "component": None,
            })
    return sorted(records, key=lambda item: item["id"])


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("reference", type=Path)
    parser.add_argument("output", type=Path)
    args = parser.parse_args()
    source = args.reference.read_bytes()
    records = enrich(parse_reference(source.decode("utf-8-sig")))
    referenced_types = sorted(set(re.findall(r"\bQ[A-Z][A-Za-z0-9_]+\b", source.decode("utf-8-sig")))
                                  - {record["qtClass"] for record in records})
    output = {
        "schemaVersion": 1,
        "id": "qt-5.15.2-native-capability-catalog",
        "source": {
            "title": "Qt 组件类与主属性汇总",
            "sha256": hashlib.sha256(source).hexdigest(),
            "qtVersion": "5.15.2",
            "note": "Imported once from the reviewed Markdown reference; this catalog is the project-owned stable contract.",
        },
        "designProvider": {"contract": "platform-neutral", "firstAdapter": "pixso", "fileKey": "4S0Ktm81WnvzuJmZHvRuOg", "pageId": "2:145"},
        "designReference": {"system": "Ant Design", "versionObserved": "6.6.5", "runtimeDependency": False},
        "propertyTargets": ["design-variant", "instance-property", "design-state", "token", "structure", "runtime-state", "business-data", "read-only"],
        "components": records,
        "referencedSupportTypes": referenced_types,
    }
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(json.dumps(output, ensure_ascii=False, indent=2) + "\n", encoding="utf-8")
    print(json.dumps({"classes": len(records), "visual": sum(r["mappingKind"] == "visual-component" for r in records), "output": str(args.output)}, ensure_ascii=False))


if __name__ == "__main__":
    main()
