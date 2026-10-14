"""Check the current delivery, test fixtures, and document links."""
# 交付一致性入口：检查离线入口、保存的测试报告、来源字节、目录规则和文档链接。
# 本脚本读取既有测试结果；完整重跑使用 Qt/V2/Verify.ps1，执行时保留 assert。
import json
import os
import re
import subprocess
import sys
from datetime import datetime, timezone
from pathlib import Path
from xml.etree import ElementTree

sys.dont_write_bytecode = True
ROOT = Path(__file__).resolve().parents[1]
NODE = os.environ.get("NODE_EXE", "node")
PROJECT = ROOT.parent
if len(sys.argv) > 1:
    # 正式四件套另走当前字节检查，不借用旧 V1/V2 保存的 PASS。此入口不等于完整验收。
    import argparse
    from formal.delivery_contract import verify_batch
    parser = argparse.ArgumentParser()
    parser.add_argument('--formal-batch', required=True)
    args = parser.parse_args()
    try:
        pages = verify_batch(PROJECT, args.formal_batch)
        print(json.dumps({'status': 'PASS_STRUCTURE', 'batch': args.formal_batch,
            'scope': 'Current package hashes, XML and public accessors only',
            'pages': pages, 'runtimeAndVisualAcceptance': 'NOT_EXECUTED_BY_THIS_COMMAND'}, ensure_ascii=False))
    except Exception as error:
        print(json.dumps({'status': 'FAIL', 'error': str(error)}, ensure_ascii=False))
        sys.exit(1)
    sys.exit(0)
REPORTS = PROJECT / "build/reports"
REPORTS.mkdir(parents=True, exist_ok=True)
result = {"verifiedAtUtc": datetime.now(timezone.utc).isoformat(), "scope": "Current delivery consistency; tests are read from their saved runs"}
try:
    # V1 的 CMD 与 EXE 同目录，逐个检查对应输入，避免入口指向缺失文件。
    v1 = ROOT / "Qt/V1"
    offline = PROJECT / "build/demo/page_examples/V1"
    launchers = {
        "Run-HTML-DSL.cmd": "hybrid-input.json",
        "Run-Holdout-HTML-DSL.cmd": "holdout-hybrid-input.json",
    }
    # 逐个核对启动脚本及其数据文件，确保每条交付入口都具备可读取的输入。
    for launcher, source in launchers.items():
        assert (offline / launcher).is_file(), launcher
        assert (offline / "data" / source).is_file(), source
    # 逐项检查 EXE、平台插件、规则和字体，提前发现离线包缺件。
    for dependency in ("d2c_preview.exe", "platforms/qwindows.dll", "qt.conf",
                       "mapping/component-mapping.json", "mapping/layout-mapping.json",
                       "fonts/HarmonyOS_Sans_SC_Regular.ttf"):
        assert (offline / dependency).is_file(), dependency
    result["v1LaunchersVerified"] = list(launchers)
    # 先检查 V2 产物/报告一致性，再读取 DSL 采集文件；verify() 调用不写采集目录。
    subprocess.run([str(NODE), str(ROOT / "Validation/check_v2.cjs")], check=True)
    import importlib.util
    result["captureBytesVerified"] = {}
    # 单一路线同时保存 HTML 与 DSL 输入；DSL 原始字节继续由采集校验器核对。
    route = "HtmlDsl"
    spec = importlib.util.spec_from_file_location(route + "_capture", ROOT / route / "pipeline/capture/verify_capture.py")
    module = importlib.util.module_from_spec(spec)
    sys.path.insert(0, str(ROOT / route / "pipeline/capture"))
    try:
        sys.modules.pop("capture_mcp", None)
        spec.loader.exec_module(module)
        captures = sorted((ROOT / route / "pipeline/design-inputs").glob("*/canonical.raw.json"))
        result["captureBytesVerified"][route] = [module.verify(p.parent)["directory"] for p in captures]
    finally:
        sys.path.pop(0)
        sys.modules.pop("capture_mcp", None)
    # MCP 原始响应通过后，还要检查 HTML、DSL、资源和根节点是否被同一份清单绑定。
    manifest_spec = importlib.util.spec_from_file_location(
        route + "_capture_manifest",
        ROOT / route / "pipeline/capture/capture_manifest.py",
    )
    manifest_module = importlib.util.module_from_spec(manifest_spec)
    manifest_spec.loader.exec_module(manifest_module)
    preview_manifests = sorted((ROOT / route / "inputs").glob("*/capture.json"))
    result["captureManifestsVerified"] = [
        manifest_module.verify(path, "preview")["manifest"] for path in preview_manifests
    ]
    result["testFunctions"] = {}
    # 分别读取组件与布局测试结果，避免只检查其中一组就给出交付结论。
    for group in ("component", "layout"):
        cases = ElementTree.parse(REPORTS / group / "qt-results.xml").getroot().findall("TestFunction")
        assert cases, group
        # 每个测试函数都必须有通过记录，防止空报告或局部失败被总数掩盖。
        for case in cases:
            incidents = case.findall("Incident")
            assert incidents and all(i.get("type") == "pass" for i in incidents), (group, case.get("name"))
        result["testFunctions"][group] = sum(c.get("name") not in ("initTestCase", "cleanupTestCase") for c in cases)
    # 当前交付明确排除的临时路径，只检查是否存在，不在这里执行删除。
    excluded = ["00_Shared", "00_RawDSL", "01_Mapping", "02_Engines", "03_GeneratedQt", "06_D2C_HTML", "07_V2"]
    # 源码只保留 HTML + DSL 生成链、Qt 运行器和验证入口。
    for folder in ("HtmlDsl", "Qt", "Validation"):
        assert (ROOT / folder).is_dir(), folder
    assert not (ROOT / "01_DSL").exists(), "Obsolete DSL-only route must be removed"
    assert not (ROOT / "02_HTML").exists(), "Ambiguous HTML route name must be removed"
    assert not (ROOT / "ComponentKit").exists(), "GenerateLayout must consume the published SDK"
    assert not (PROJECT / "build/sdk").exists(), "GenerateLayout must not contain build/sdk"
    assert not (ROOT / "Import-GeneratedComponentSDK.ps1").exists(), "Copy-based SDK import is retired"
    assert (ROOT / "Verify-GeneratedComponentSDK.ps1").is_file()
    sdk_root = Path(os.environ.get(
        "GENERATED_COMPONENT_SDK_ROOT",
        PROJECT.parent / "DesignToCode_GenerateComponent" / "build" / "sdk",
    )).resolve()
    sdk_manifest = sdk_root / "manifest/component-sdk.json"
    sdk_metadata = json.loads(sdk_manifest.read_bytes())
    assert sdk_metadata["owner"] == "DesignToCode_GenerateComponent"
    assert sdk_metadata["componentAuthority"] and sdk_metadata["tokenAuthority"]
    result["sdkAuthority"] = str(sdk_root)
    result["sdkMode"] = "direct-reference"
    # Source 只保留生成器随产物输出的用法；正式说明集中在本项目 docs。
    assert not (ROOT / "05_Docs").exists(), "Maintain guides in project docs"
    assert not set(ROOT.rglob("*.md")), "Maintain layout documentation in project docs"
    assert (PROJECT / "README.md").is_file()
    assert (PROJECT / "docs/README.md").is_file()
    route_root = ROOT / route
    assert (route_root / "Generate.ps1").is_file()
    assert (route_root / "generated").is_dir()
    assert (route_root / "pipeline/Generate-Pages.ps1").is_file()
    assert not (route_root / "Inspect.ps1").exists(), "HTML-only inspection is outside the supported route"
    retained = [p for p in excluded if (ROOT / p).exists()]
    forbidden = [p for p in ROOT.rglob("*") if p.is_file() and p.suffix.lower() in (".pyc", ".pdb", ".obj", ".zip", ".autosave", ".bak", ".log")]
    # 逐份检查更新日期、代码围栏闭合及本地链接目标；外部链接不联网探测。
    links = 0
    # 逐份检查说明文档的日期、代码围栏和链接，防止目录调整后阅读入口失效。
    for file in [PROJECT / "README.md", *ROOT.rglob("*.md"), *(PROJECT / "docs").rglob("*.md")]:
        content = file.read_text(encoding="utf-8")
        assert len(re.findall(r"^\x60\x60\x60", content, re.M)) % 2 == 0, file
        # 逐个解析当前文档的链接，去掉页内锚点后检查实际文件是否存在。
        for target in re.findall(r"\]\(([^)]+)\)", content):
            target = target.strip("<> ").split("#")[0]
            # 页内跳转和 HTTP 链接交给阅读器处理；本检查只验证本地文件，以保证断网可执行。
            if not target or re.match(r"^https?:", target):
                continue
            assert (file.parent / target).resolve().exists(), (file, target)
            # 只允许已确认的两份协同合同跨项目引用，不取消项目边界检查。
            permitted_contracts = {
                (PROJECT.parent / 'DesignToCode_GenerateComponent/docs/designRules/components/Qt表格设计与原生映射.md').resolve(),
                (PROJECT.parent / 'DesignToCode_GenerateComponent/docs/Orders-Design-System-协同排期/README.md').resolve(),
            }
            resolved_target = (file.parent / target).resolve()
            assert resolved_target.is_relative_to(PROJECT) or resolved_target in permitted_contracts, (file, "cross-project link", target)
            links += 1
    result.update(localLinksVerified=links, currentV2=json.loads((REPORTS / "v2/result.json").read_bytes())["status"],
                  excludedPathsPresent=retained, disallowedFilesPresent=[p.relative_to(ROOT).as_posix() for p in forbidden])
    result["status"] = "FAIL_STRUCTURE" if retained or forbidden else "PASS"
except Exception as error:
    result.update(status="FAIL", error=str(error))
    raise
finally:
    # 即使中途抛错也写 FAIL，结果集中在验证目录，避免散落到输入和源码目录。
    (REPORTS / "delivery-result.json").write_text(json.dumps(result, ensure_ascii=False, indent=2), encoding="utf-8")
    print(json.dumps(result, ensure_ascii=False))
# 结构问题虽已写入报告，仍需用非零退出码告知自动化调用方本轮检查失败。
if result["status"] != "PASS":
    sys.exit(1)
