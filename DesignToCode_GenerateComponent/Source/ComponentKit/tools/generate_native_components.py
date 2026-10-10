"""Generate one Z_* C++ and QSS contract for every concrete Qt visual component."""

from __future__ import annotations

import argparse
import hashlib
import json
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
DEFINITIONS = ROOT / "component-definitions/native-components.json"
COMPONENT_DIR = ROOT / "runtime/component/native"
STYLE_DIR = ROOT / "design-assets/styles/native"
GENERATED_STYLE_DIR = ROOT / "design-assets/generated/native"
SOURCE_MAP = ROOT / "component-definitions/generated-component-source-map.json"
BUTTON_MANIFEST = ROOT / "design-assets/generated/button/manifest.json"
RUNTIME_TOKENS = ROOT / "design-assets/DesignSystem/generated/runtime/tokens_qt.json"


def load_inputs():
    contract = json.loads(DEFINITIONS.read_text(encoding="utf-8"))
    components = contract["components"]
    qt_classes = {item["qtClass"] for item in components}
    sdk_classes = {item["sdkClass"] for item in components}
    assert len(components) == 59, len(components)
    assert len(qt_classes) == len(components)
    assert len(sdk_classes) == len(components)
    for item in components:
        expected = "Z_" + item["qtClass"][1:]
        assert item["sdkClass"] == expected, (item["sdkClass"], expected)
        assert item["implementation"] in {"generated", "specialized"}
    token_manifest = json.loads(BUTTON_MANIFEST.read_text(encoding="utf-8"))
    profiles = {item["profile"]: item for item in token_manifest["profiles"]}
    runtime_tokens = json.loads(RUNTIME_TOKENS.read_text(encoding="utf-8"))
    assert set(profiles) == {
        "light-comfortable", "light-compact", "dark-comfortable", "dark-compact"
    }
    for name, profile in profiles.items():
        values = runtime_tokens["profiles"][name]
        profile["nativeMetrics"] = {
            "height": values["layout"]["semantic.dimension.control.height-md"],
            "radius": values["layout"]["semantic.radius.control-md"],
            "fontSize": values["font"]["base.stoken.typography.font-size.label.default"],
        }
    return components, profiles


def header_text(item):
    return f'''#pragma once

#include <{item["qtClass"]}>

namespace commonWidget {{

// Project-owned component identity for Qt's {item["qtClass"]} behavior.
class {item["sdkClass"]} : public {item["qtClass"]} {{
    Q_OBJECT
  public:
    explicit {item["sdkClass"]}(QWidget* parent = nullptr);
}};

}} // namespace commonWidget
'''


def source_text(item):
    return f'''#include "{item["sdkClass"]}.h"
#include <QVariant>

namespace commonWidget {{

{item["sdkClass"]}::{item["sdkClass"]}(QWidget* parent) : {item["qtClass"]}(parent) {{
    setProperty("designSystemNative", true);
    setProperty("designSystemClass", "commonWidget::{item["sdkClass"]}");
    setProperty("nativeQtClass", "{item["qtClass"]}");
}}

}} // namespace commonWidget
'''


def registry_header_text():
    return '''#pragma once

#include "../ComponentFactory.h"

class QWidget;

namespace commonWidget {

// Compiled registry generated from native-components.json. This is product
// source: callers can inspect and debug it like any other C++ implementation.
QList<NativeComponentDescriptor> nativeComponentDescriptors();
QWidget* createGeneratedNativeComponent(const QString& sdkClass,
                                        QWidget* parent = nullptr);

} // namespace commonWidget
'''


def registry_source_text(components):
    descriptors = "\n".join(
        '        {QStringLiteral("%s"), QStringLiteral("commonWidget::%s"), '
        'QStringLiteral("ComponentLab/%s"), QStringLiteral("visual-component")},'
        % (item["qtClass"], item["sdkClass"], item["designComponent"])
        for item in components
    )
    factories = "\n".join(
        '    if (sdkClass == QStringLiteral("commonWidget::%s")) '
        'return new %s(parent);' % (item["sdkClass"], item["sdkClass"])
        for item in components if item["implementation"] == "generated"
    )
    return f'''#include "NativeComponentRegistry.h"
#include "NativeComponents.h"

namespace commonWidget {{

QList<NativeComponentDescriptor> nativeComponentDescriptors() {{
    return {{
{descriptors}
    }};
}}

QWidget* createGeneratedNativeComponent(const QString& sdkClass, QWidget* parent) {{
{factories}
    return nullptr;
}}

}} // namespace commonWidget
'''


def qss_template(item):
    selector = f'*[designSystemClass="commonWidget::{item["sdkClass"]}"]'
    common = f'''/* {item["sdkClass"]} <- {item["qtClass"]}; compiled from the active Token profile. */
{selector} {{
    color: @FOREGROUND@;
    background-color: @SURFACE@;
    font-size: @FONT_SIZE@px;
}}
{selector}:disabled {{
    color: @DISABLED_TEXT@;
    background-color: @DISABLED@;
    border-color: @DISABLED_BORDER@;
}}
'''
    if item["implementation"] == "specialized":
        return (f'/* {item["sdkClass"]} uses its specialized Token compiler output. '
                'This file records its native mapping without overriding that style. */\n')
    if item["category"] == "input":
        common += f'''{selector} {{
    min-height: @HEIGHT@px;
    border: 1px solid @BORDER@;
    border-radius: @RADIUS@px;
    padding-left: 8px;
    padding-right: 8px;
}}
{selector}:focus {{ border-color: @FOCUS@; }}
'''
    elif item["category"] == "button":
        common += f'''{selector} {{
    min-height: @HEIGHT@px;
    border: 1px solid @BORDER@;
    border-radius: @RADIUS@px;
    padding: 0 12px;
}}
{selector}:hover {{ background-color: @PRIMARY_SOFT@; }}
{selector}:pressed {{ border-color: @PRIMARY@; }}
'''
    elif item["qtClass"] == "QProgressBar":
        common += f'''{selector} {{
    min-height: @HEIGHT@px;
    border: 1px solid @BORDER@;
    border-radius: @RADIUS@px;
    text-align: center;
}}
{selector}::chunk {{ background-color: @PRIMARY@; border-radius: @RADIUS@px; }}
'''
    elif item["qtClass"] == "QLabel":
        common += f'{selector} {{ background: transparent; border: none; }}\n'
    elif item["category"] in {"data", "display", "container"}:
        common += f'''{selector} {{
    border: 1px solid @BORDER@;
    border-radius: @RADIUS@px;
}}
'''
    return common


def render_qss(template, profile):
    palette = profile["palette"]
    metrics = profile["nativeMetrics"]
    values = {
        "FOREGROUND": palette["foreground"],
        "SURFACE": palette["surface"],
        "PRIMARY": palette["primary"],
        "PRIMARY_SOFT": palette["primarySoft"],
        "BORDER": palette["border"],
        "DISABLED": palette["disabled"],
        "DISABLED_TEXT": palette["disabledText"],
        "DISABLED_BORDER": palette["disabledBorder"],
        "FOCUS": palette["focus"],
        "HEIGHT": str(metrics["height"]),
        "FONT_SIZE": str(metrics["fontSize"]),
        "RADIUS": str(metrics["radius"]),
    }
    for name, value in values.items():
        template = template.replace(f"@{name}@", value)
    assert "@" not in template, template
    return template


def expected_outputs(components, profiles):
    outputs = {}
    generated = [item for item in components if item["implementation"] == "generated"]
    for item in generated:
        outputs[COMPONENT_DIR / f'{item["sdkClass"]}.h'] = header_text(item)
        outputs[COMPONENT_DIR / f'{item["sdkClass"]}.cpp'] = source_text(item)

    includes = ["#pragma once", "", '#include "../Z_PushButton.h"',
                '#include "../Z_CheckBox.h"']
    includes.extend(f'#include "{item["sdkClass"]}.h"' for item in generated)
    outputs[COMPONENT_DIR / "NativeComponents.h"] = "\n".join(includes) + "\n"
    outputs[COMPONENT_DIR / "NativeComponentRegistry.h"] = registry_header_text()
    outputs[COMPONENT_DIR / "NativeComponentRegistry.cpp"] = registry_source_text(components)

    source_map = {"schemaVersion": 1, "owner": "DesignToCode_GenerateComponent",
                  "components": []}
    for item in components:
        base = ("runtime/component" if item["implementation"] == "specialized"
                else "runtime/component/native")
        style_family = ("button" if item["qtClass"] == "QPushButton" else
                        "checkbox" if item["qtClass"] == "QCheckBox" else "native")
        source_template_path = (f'design-assets/styles/{item["sdkClass"]}.qss.in'
                                if item["implementation"] == "specialized"
                                else f'design-assets/styles/native/{item["sdkClass"]}.qss.in')
        source_map["components"].append({
            "qtClass": item["qtClass"],
            "sdkClass": "commonWidget::" + item["sdkClass"],
            "designComponent": "ComponentLab/" + item["designComponent"],
            "header": f'{base}/{item["sdkClass"]}.h',
            "implementation": f'{base}/{item["sdkClass"]}.cpp',
            "qss": f'design-assets/generated/{style_family}/<profile>/{item["sdkClass"]}.qss',
            "qssTemplate": source_template_path,
            "category": item["category"],
            "implementationKind": item["implementation"],
        })
    outputs[SOURCE_MAP] = json.dumps(source_map, ensure_ascii=False, indent=2) + "\n"

    templates = {}
    for item in components:
        template = qss_template(item)
        templates[item["sdkClass"]] = template
        outputs[STYLE_DIR / f'{item["sdkClass"]}.qss.in'] = template
    for profile_name, profile in profiles.items():
        combined = []
        for item in components:
            rendered = render_qss(templates[item["sdkClass"]], profile)
            outputs[GENERATED_STYLE_DIR / profile_name / f'{item["sdkClass"]}.qss'] = rendered
            combined.append(rendered)
        outputs[GENERATED_STYLE_DIR / profile_name / "native-components.qss"] = "\n".join(combined)
    manifest = {
        "schemaVersion": 1,
        "componentCount": len(components),
        "generatedCppCount": len(generated),
        "profiles": list(profiles),
        "definitionSha256": hashlib.sha256(DEFINITIONS.read_bytes()).hexdigest(),
        "runtimeTokenSha256": hashlib.sha256(RUNTIME_TOKENS.read_bytes()).hexdigest(),
        "namingRule": "Q<Class> -> commonWidget::Z_<Class>",
    }
    outputs[GENERATED_STYLE_DIR / "manifest.json"] = json.dumps(
        manifest, ensure_ascii=False, indent=2
    ) + "\n"
    return outputs


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--check", action="store_true")
    args = parser.parse_args()
    components, profiles = load_inputs()
    outputs = expected_outputs(components, profiles)
    drift = []
    for path, content in outputs.items():
        if args.check:
            if not path.is_file() or path.read_text(encoding="utf-8") != content:
                drift.append(str(path.relative_to(ROOT)))
            continue
        if path.is_file() and path.read_text(encoding="utf-8") == content:
            continue
        path.parent.mkdir(parents=True, exist_ok=True)
        path.write_text(content, encoding="utf-8", newline="\n")
    if drift:
        raise SystemExit("Generated native component drift:\n" + "\n".join(drift))
    print(json.dumps({"components": len(components), "outputs": len(outputs),
                      "mode": "check" if args.check else "write"}))


if __name__ == "__main__":
    main()
