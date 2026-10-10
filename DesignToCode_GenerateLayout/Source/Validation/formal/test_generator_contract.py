"""使用真实复核输入验证生成器门禁；所有变体和输出仅存在于临时目录。"""
import json
import hashlib
import os
from pathlib import Path
import shutil
import subprocess
import tempfile
import unittest
import xml.etree.ElementTree as ET
from delivery_contract import verify_page

PROJECT = Path(__file__).resolve().parents[3]
PIPELINE = PROJECT / "Source/HtmlDsl/pipeline"
COMPILER = Path(os.environ.get("D2C_COMPILER", str(PROJECT / "build/cache/V2-html-dsl/v2_ui_compiler.exe")))


class GeneratorContractTests(unittest.TestCase):
    def setUp(self):
        self.temp = tempfile.TemporaryDirectory(prefix="layout-contract-")
        self.addCleanup(self.temp.cleanup)
        self.root = Path(self.temp.name)
        self.raw = json.loads((PIPELINE / "design-inputs/review-page/canonical.raw.json").read_bytes())
        self.names = json.loads((PIPELINE / "mapping/engineering-names.json").read_bytes())
        shutil.copytree(PIPELINE / "mapping", self.root / "mapping")

    def generate(self, formal=False, output=None, compiler=COMPILER):
        (self.root / "input.json").write_text(json.dumps(self.raw), encoding="utf-8")
        (self.root / "mapping/engineering-names.json").write_text(json.dumps(self.names), encoding="utf-8")
        env = dict(os.environ, QT_QPA_PLATFORM="offscreen")
        arguments = [
            str(compiler), "--raw", str(self.root / "input.json"),
            "--mapping", str(self.root / "mapping"),
            "--extensions", str(self.root / "mapping/extensions.json"),
            "--font", str(PIPELINE / "resources/HarmonyOS_Sans_SC_Regular.ttf"),
            "--output", str(output or self.root / "page"), "--strict-layout",
        ]
        if formal:
            arguments.append("--formal")
        result = subprocess.run(arguments, capture_output=True, timeout=30, env=env)
        return result.returncode, result.stderr.decode("utf-8", errors="replace")

    def assert_rejected(self, message):
        code, error = self.generate()
        self.assertNotEqual(code, 0, error)
        self.assertIn(message, error)
        self.assertFalse((self.root / "page.ui").exists())

    def test_complete_real_input_is_accepted(self):
        code, error = self.generate()
        self.assertEqual(code, 0, error)
        ui = ET.parse(self.root / "page.ui")
        self.assertEqual(ui.getroot().find("widget").get("name"), "reviewPage")

    def test_missing_gap_is_not_explicit_zero(self):
        del self.raw["pixTreeNodes"][0]["autoLayout"]["autoLayoutItemSpacing"]
        self.assert_rejected("MISSING_LAYOUT_METRIC: autoLayoutItemSpacing")

    def test_cpp_keyword_name_is_rejected(self):
        self.names["19:46"] = "class"
        self.assert_rejected("INVALID_ENGINEERING_NAME: class")

    def test_duplicate_name_is_not_suffixed_with_guid(self):
        self.names["19:46"] = self.names["19:47"]
        self.assert_rejected("DUPLICATE_ENGINEERING_NAME")

    def test_unmapped_chinese_name_requires_explicit_mapping(self):
        del self.names["19:46"]
        self.assert_rejected("ENGINEERING_NAME_MAPPING_REQUIRED")

    def compact_fixture(self):
        # 仅用于测试接口：显式改变测试副本的 Profile，不冒充原 Pixso 已批准 Compact。
        mapping = self.root / "mapping/component-mapping.json"
        rules = json.loads(mapping.read_bytes())
        rules["profile"] = "light-compact"
        mapping.write_text(json.dumps(rules), encoding="utf-8")
        extensions = self.root / "mapping/extensions.json"
        extension_rules = json.loads(extensions.read_bytes())
        extension_rules["selectOption"]["properties"]["Profile"] = "light-compact"
        extensions.write_text(json.dumps(extension_rules), encoding="utf-8")
        def visit(node):
            for prop in node.get("propRefMap", {}).get("variant", []):
                if prop.get("name") == "Profile":
                    prop["value"] = "light-compact"
            for child in node.get("childNode", []):
                visit(child)
        for root in self.raw["pixTreeNodes"]:
            visit(root)

    def formal_output(self):
        base = PROJECT / "build/delivery"
        base.mkdir(parents=True, exist_ok=True)
        batch = tempfile.TemporaryDirectory(prefix="contract-", dir=base)
        self.addCleanup(batch.cleanup)
        return Path(batch.name) / "review"

    def reject_formal_fixture(self, expected):
        output = self.formal_output()
        code, error = self.generate(formal=True, output=output)
        self.assertNotEqual(code, 0, error)
        self.assertIn(expected, error)
        self.assertFalse(output.exists())

    def test_boolean_api_rejects_string_instead_of_coercing_false(self):
        self.compact_fixture()
        path = self.root / "mapping/component-mapping.json"
        mapping = json.loads(path.read_bytes())
        mapping['rules'][0]['stateMappings']['disabled'][0]['value'] = 'true'
        path.write_text(json.dumps(mapping), encoding='utf-8')
        self.reject_formal_fixture('INVALID_API_VALUE_TYPE')

    def test_string_api_rejects_number_instead_of_empty_text(self):
        self.compact_fixture()
        path = self.root / "mapping/component-mapping.json"
        mapping = json.loads(path.read_bytes())
        mapping['rules'][0]['stateMappings']['disabled'].append({'api': 'setText', 'value': 123})
        path.write_text(json.dumps(mapping), encoding='utf-8')
        self.reject_formal_fixture('INVALID_API_VALUE_TYPE')

    def test_color_channels_require_explicit_numeric_values(self):
        self.compact_fixture()
        heading = next(n for n in self.raw['pixTreeNodes'][0]['childNode'] if n['guid'] == '19:46')
        color = heading['fillPaints'][0]['color']
        original = dict(color)
        for value in ['31', -1, 256, 1.5, {'alias': 'missing'}]:
            with self.subTest(value=value):
                color.update(original)
                color['r'] = value
                self.reject_formal_fixture('INVALID_COLOR_CHANNEL')

    def test_missing_text_color_cannot_fall_back_to_black(self):
        self.compact_fixture()
        heading = next(n for n in self.raw['pixTreeNodes'][0]['childNode'] if n['guid'] == '19:46')
        heading['fillPaints'] = []
        self.reject_formal_fixture('MISSING_TEXT_COLOR')

    def test_text_value_must_be_string(self):
        self.compact_fixture()
        heading = next(n for n in self.raw['pixTreeNodes'][0]['childNode'] if n['guid'] == '19:46')
        heading['nodeText'] = 123
        self.reject_formal_fixture('INVALID_TEXT_VALUE')

    def test_formal_rejects_implicit_profile_switch(self):
        code, error = self.generate(formal=True)
        self.assertNotEqual(code, 0)
        self.assertIn("FORMAL_REQUIRES_LIGHT_COMPACT_MAPPING", error)

    def test_entry_defaults_to_formal_and_rejects_mixed_modes(self):
        entry = PROJECT / 'Source/HtmlDsl/Generate.ps1'
        shell = shutil.which('pwsh') or 'powershell.exe'
        for flags, expected in [([], 'FORMAL_BATCH_AND_PAGE_REQUIRED'),
                                (['-Formal', '-Preview'], 'FORMAL_AND_PREVIEW_ARE_EXCLUSIVE'),
                                (['-Preview', '-Batch', 'bad'], 'PREVIEW_DOES_NOT_ACCEPT_FORMAL_ARGUMENTS'),
                                (['-OutputDirectory', str(self.root / 'business')], 'FORMAL_OUTPUT_IS_TOOL_OWNED')]:
            with self.subTest(flags=flags):
                result = subprocess.run([shell, '-NoProfile', '-NonInteractive', '-File', str(entry), *flags],
                                        capture_output=True, timeout=30)
                self.assertNotEqual(result.returncode, 0)
                self.assertIn(expected, result.stderr.decode('utf-8', errors='replace'))
        self.assertFalse((self.root / 'business').exists())

    def alias_fixture(self):
        self.compact_fixture()
        self.raw.setdefault('variableSetMap', {})['test-set'] = [{'id': 'test-mode', 'name': 'light-compact'}]
        self.raw.setdefault('variableMap', {})['test-size'] = {
            'type': 'FLOAT', 'variableSetID': 'test-set', 'name': 'test/size',
            'modeValues': {'test-mode': {'dataType': 'FLOAT', 'value': {'floatValue': 24}}}}
        heading = next(n for n in self.raw['pixTreeNodes'][0]['childNode'] if n['guid'] == '19:46')
        heading['variablesAlias'] = [{'field': 'FONT_SIZE', 'data': {
            'dataType': 'ALIAS', 'resolvedDataType': 'FLOAT', 'value': {'alias': 'test-size'}}}]

    def test_unknown_component_family_is_rejected(self):
        self.compact_fixture()
        instance = next(n for n in self.raw['pixTreeNodes'][0]['childNode'] if n['type'] == 'INSTANCE')
        family = next(n for n in self.raw['pixComponentTreeDslNodes'] if n['guid'] == instance['mainStateGroup'])
        family['name'] = 'Unmapped/Component'
        self.reject_formal_fixture('UNMAPPED_COMPONENT')

    def test_broken_component_master_is_rejected(self):
        self.compact_fixture()
        instance = next(n for n in self.raw['pixTreeNodes'][0]['childNode'] if n['type'] == 'INSTANCE')
        instance['mainComponent'] = 'missing:master'
        self.reject_formal_fixture('INVALID_COMPONENT_REFERENCE')

    def test_broken_variable_alias_is_not_ignored(self):
        self.alias_fixture()
        del self.raw['variableMap']['test-size']
        self.reject_formal_fixture('VARIABLE_REFERENCE_MISSING')

    def test_variable_alias_cycle_is_rejected(self):
        self.alias_fixture()
        self.raw['variableMap']['test-size']['modeValues']['test-mode'] = {
            'dataType': 'ALIAS', 'value': {'alias': 'test-size'}}
        self.reject_formal_fixture('VARIABLE_ALIAS_CYCLE')

    def test_variable_alias_type_mismatch_is_rejected(self):
        self.alias_fixture()
        variable = self.raw['variableMap']['test-size']
        variable['type'] = 'COLOR'
        variable['modeValues']['test-mode'] = {'dataType': 'COLOR', 'value': {'colorValue': {'r': 0, 'g': 0, 'b': 0, 'a': 1}}}
        self.reject_formal_fixture('VARIABLE_TYPE_MISMATCH')

    def test_variable_literal_type_mismatch_is_rejected(self):
        self.alias_fixture()
        self.raw['variableMap']['test-size']['modeValues']['test-mode']['value']['floatValue'] = '24'
        self.reject_formal_fixture('INVALID_VARIABLE_LITERAL')

    def test_valid_variable_alias_is_reported_outside_package(self):
        self.alias_fixture()
        output = self.formal_output()
        code, error = self.generate(formal=True, output=output)
        self.assertEqual(code, 0, error)
        record = json.loads((PROJECT / 'build/reports/formal-delivery' / output.parent.name / output.name / 'generation-record.json').read_bytes())
        self.assertEqual(record['variableValidation']['status'], 'PASS')
        self.assertGreater(record['variableValidation']['referenceCount'], 0)

    def test_current_package_check_rejects_changed_qss(self):
        self.compact_fixture()
        output = self.formal_output()
        code, error = self.generate(formal=True, output=output)
        self.assertEqual(code, 0, error)
        record = PROJECT / 'build/reports/formal-delivery' / output.parent.name / output.name / 'generation-record.json'
        verify_page(output, record)
        qss = output / 'view/res/qss/ReviewPage.qss'
        qss.write_bytes(qss.read_bytes() + b'\n/* changed after generation */\n')
        with self.assertRaisesRegex(ValueError, 'OUTPUT_BYTES_CHANGED'):
            verify_page(output, record)

    def test_current_package_check_rejects_extra_runtime_file(self):
        self.compact_fixture()
        output = self.formal_output()
        code, error = self.generate(formal=True, output=output)
        self.assertEqual(code, 0, error)
        record = PROJECT / 'build/reports/formal-delivery' / output.parent.name / output.name / 'generation-record.json'
        (output / 'design-trace.json').write_text('{}', encoding='utf-8')
        with self.assertRaisesRegex(ValueError, 'OUTPUT_FILE_SET_CHANGED'):
            verify_page(output, record)

    def test_formal_refuses_external_directory(self):
        self.compact_fixture()
        code, error = self.generate(formal=True, output=self.root / "business")
        self.assertNotEqual(code, 0)
        self.assertIn("FORMAL_OUTPUT_OUTSIDE_DELIVERY", error)
        self.assertFalse((self.root / "business").exists())

    def test_formal_four_files_without_runtime_trace_or_spacers(self):
        self.compact_fixture()
        output = self.formal_output()
        code, error = self.generate(formal=True, output=output)
        self.assertEqual(code, 0, error)
        files = sorted(str(p.relative_to(output)).replace("\\", "/") for p in output.rglob("*") if p.is_file())
        self.assertEqual(files, ["form/ReviewPage.ui", "include/ReviewPage.h", "src/ReviewPage.cpp", "view/res/qss/ReviewPage.qss"])
        record_path = PROJECT / 'build/reports/formal-delivery' / output.parent.name / output.name / 'generation-record.json'
        record = json.loads(record_path.read_bytes())
        self.assertEqual(set(record['outputSha256']), set(files))
        for name in files:
            self.assertEqual(record['outputSha256'][name], hashlib.sha256((output / name).read_bytes()).hexdigest())
        ui = ET.parse(output / "form/ReviewPage.ui")
        self.assertFalse(ui.findall(".//spacer"))
        self.assertEqual(len(ui.findall(".//property[@name='geometry']")), 1)
        self.assertFalse(ui.findall(".//property[@name='designNodeId']"))
        self.assertTrue(ui.find(".//layout[@name='reviewPageLayout']") is not None)
        for p in output.rglob("*"):
            if p.is_file():
                self.assertNotIn("19:45", p.read_text(encoding="utf-8"))
        code = (output / "src/ReviewPage.cpp").read_text(encoding="utf-8")
        # 原始复核页主轴为 end，交叉轴 Fill；不能把设计改成顶部对齐。
        self.assertIn("setAlignment(Qt::AlignBottom)", code)
        self.assertIn("getWorkOrderInput", code)
        again, error = self.generate(formal=True, output=output)
        self.assertNotEqual(again, 0)
        self.assertIn("FORMAL_OUTPUT_ALREADY_EXISTS", error)

    def test_unknown_layout_and_absolute_child_fail(self):
        for field, value, message in [
            ("stackMode", "GRID", "UNMAPPED_LAYOUT_MODE"),
            ("autoLayoutItemAbsolutePos", True, "UNMAPPED_ABSOLUTE_CHILD"),
        ]:
            with self.subTest(field=field):
                original = json.loads(json.dumps(self.raw))
                node = self.raw["pixTreeNodes"][0]
                if field == "autoLayoutItemAbsolutePos":
                    node = node["childNode"][0]
                node.setdefault("autoLayout", {})[field] = value
                self.assert_rejected(message)
                self.raw = original

    def test_formal_failure_does_not_publish_partial_package(self):
        self.compact_fixture()
        output = self.formal_output()
        # 名称与生成的布局名冲突，必须在发布任何文件前失败。
        self.names["19:46"] = "reviewPageLayout"
        code, error = self.generate(formal=True, output=output)
        self.assertNotEqual(code, 0)
        self.assertIn("DUPLICATE_ENGINEERING_NAME", error)
        self.assertFalse(output.exists())

    def test_formal_repeat_is_byte_identical(self):
        self.compact_fixture()
        outputs = [self.formal_output(), self.formal_output()]
        for output in outputs:
            code, error = self.generate(formal=True, output=output)
            self.assertEqual(code, 0, error)
        first = {p.relative_to(outputs[0]): p.read_bytes() for p in outputs[0].rglob("*") if p.is_file()}
        second = {p.relative_to(outputs[1]): p.read_bytes() for p in outputs[1].rglob("*") if p.is_file()}
        self.assertEqual(first, second)

    def result_fixture(self):
        self.raw = json.loads((PROJECT / "build/cache/formal-inputs/result/hybrid-result.json").read_bytes())
        self.compact_fixture()

    def resource_entry(self, update):
        path = self.root / "mapping/component-mapping.json"
        mapping = json.loads(path.read_bytes())
        entry = mapping["rules"][0]["resourceMappings"]["entries"]["GLYPH / arrow-left"]
        entry.update(update)
        path.write_text(json.dumps(mapping), encoding="utf-8")

    def page_asset_fixture(self):
        # 本地自制测试图形，专用于验证 QRC；不是 Pixso 导出素材或正式公共前缀。
        self.resource_entry({
            "ownership": "page",
            "source": str(Path(__file__).parent / "fixtures/page-marker.svg"),
            "qrcPrefix": "/formal-fixture/result",
            "alias": "page-marker.svg",
            "provenance": "Self-authored geometric integration-test fixture; not a Pixso asset",
            "license": "CC0-1.0",
        })

    def test_page_resource_is_portable_and_trace_stays_outside(self):
        self.result_fixture()
        self.page_asset_fixture()
        output = self.formal_output()
        code, error = self.generate(formal=True, output=output)
        self.assertEqual(code, 0, error)
        qrc = ET.parse(output / "view/res/ReviewResultPage.qrc")
        resource = qrc.getroot().find("qresource")
        self.assertEqual(resource.get("prefix"), "/formal-fixture/result")
        asset = resource.find("file")
        self.assertEqual(asset.get("alias"), "page-marker.svg")
        self.assertTrue((output / "view/res" / asset.text).is_file())
        cpp = (output / "src/ReviewResultPage.cpp").read_text(encoding="utf-8")
        self.assertIn(":/formal-fixture/result/page-marker.svg", cpp)
        self.assertNotIn(str(Path(__file__).parent), cpp)
        self.assertFalse(list(output.rglob("*.json")))

    def test_public_resource_is_not_copied(self):
        self.result_fixture()
        output = self.formal_output()
        code, error = self.generate(formal=True, output=output)
        self.assertEqual(code, 0, error)
        self.assertFalse(list(output.rglob("*.qrc")))
        self.assertFalse(list(output.rglob("*.svg")))

    def test_unknown_resource_ownership_is_rejected(self):
        self.result_fixture()
        self.resource_entry({"ownership": "unknown"})
        output = self.formal_output()
        code, error = self.generate(formal=True, output=output)
        self.assertNotEqual(code, 0)
        self.assertIn("RESOURCE_OWNERSHIP_REQUIRED", error)
        self.assertFalse(output.exists())

    def test_page_asset_cannot_shadow_sdk_namespace(self):
        self.result_fixture()
        self.page_asset_fixture()
        self.resource_entry({"qrcPrefix": "/icons/page"})
        output = self.formal_output()
        code, error = self.generate(formal=True, output=output)
        self.assertNotEqual(code, 0)
        self.assertIn("RESOURCE_SHARED_NAMESPACE_COLLISION", error)
        self.assertFalse(output.exists())

    def test_missing_shared_resource_is_not_local_fallback(self):
        self.result_fixture()
        self.resource_entry({"source": ":/icons/absent-formal-test.svg"})
        output = self.formal_output()
        code, error = self.generate(formal=True, output=output)
        self.assertNotEqual(code, 0)
        self.assertIn("Cannot open icon", error)
        self.assertFalse(output.exists())

    def test_business_files_are_unchanged_when_output_is_rejected(self):
        self.compact_fixture()
        business = self.root / "business"
        business.mkdir()
        files = {"Page.cpp": b"business-owned implementation\n", "Page.ui": b"business-owned layout\n"}
        for name, data in files.items():
            (business / name).write_bytes(data)
        before = {name: hashlib.sha256((business / name).read_bytes()).hexdigest() for name in files}
        code, error = self.generate(formal=True, output=business / "new-page")
        self.assertNotEqual(code, 0)
        self.assertIn("FORMAL_OUTPUT_OUTSIDE_DELIVERY", error)
        after = {name: hashlib.sha256((business / name).read_bytes()).hexdigest() for name in files}
        self.assertEqual(before, after)
        self.assertEqual(set(p.name for p in business.iterdir()), set(files))

    def powershell_fixture(self, script, **paths):
        env = dict(os.environ, **{key: str(value) for key, value in paths.items()})
        result = subprocess.run([shutil.which('pwsh') or 'powershell.exe', '-NoProfile', '-NonInteractive', '-Command',
                                 "$ErrorActionPreference='Stop'; " + script],
                                env=env, capture_output=True, timeout=30)
        self.assertEqual(result.returncode, 0, result.stderr.decode(errors='replace'))

    @unittest.skipUnless(os.name == 'nt', 'Windows junction contract')
    def test_junction_cannot_redirect_delivery_into_business(self):
        self.compact_fixture()
        business = self.root / 'business-junction-target'
        business.mkdir()
        marker = business / 'owned.cpp'
        marker.write_bytes(b'business source stays unchanged')
        # G: 为 exFAT，不支持联接。在 NTFS 临时目录放置同一生成器字节来验证重定向。
        tool = self.root / 'ntfs-tool'
        (tool / 'Source/HtmlDsl/pipeline').mkdir(parents=True)
        compiler = tool / 'build/cache/compiler/v2_ui_compiler.exe'
        compiler.parent.mkdir(parents=True)
        shutil.copy2(COMPILER, compiler)
        output = tool / 'build/delivery/junction/page'
        output.parent.mkdir(parents=True)
        link = output.parent / 'redirected'
        self.powershell_fixture('New-Item -ItemType Junction -Path $env:LINK -Target $env:TARGET | Out-Null',
                                LINK=link, TARGET=business)
        # rmdir 只移除已核对目标的联接本身，不递归删除目标目录。
        self.assertEqual(link.resolve(), business.resolve())
        self.addCleanup(lambda: os.rmdir(link))
        code, error = self.generate(formal=True, output=link / 'page', compiler=compiler)
        self.assertNotEqual(code, 0)
        self.assertIn('FORMAL_OUTPUT_REDIRECTED', error)
        self.assertEqual(list(business.iterdir()), [marker])
        self.assertEqual(marker.read_bytes(), b'business source stays unchanged')

    @unittest.skipUnless(os.name == 'nt', 'Windows read-only ACL contract')
    def test_readonly_business_directory_is_not_written(self):
        self.compact_fixture()
        business = self.root / 'business-readonly'
        business.mkdir()
        marker = business / 'owned.cpp'
        marker.write_bytes(b'business source stays unchanged')
        acl = self.root / 'original-acl.xml'
        self.powershell_fixture('Get-Acl -LiteralPath $env:TARGET | Export-Clixml -LiteralPath $env:ACL',
                                TARGET=business, ACL=acl)
        self.addCleanup(lambda: self.powershell_fixture(
            'Set-Acl -LiteralPath $env:TARGET -AclObject (Import-Clixml -LiteralPath $env:ACL)',
            TARGET=business, ACL=acl))
        self.powershell_fixture('''
            $acl=Get-Acl -LiteralPath $env:TARGET
            $sid=[Security.Principal.WindowsIdentity]::GetCurrent().User
            $rule=New-Object Security.AccessControl.FileSystemAccessRule($sid,'Write','ContainerInherit,ObjectInherit','None','Deny')
            $acl.AddAccessRule($rule)
            Set-Acl -LiteralPath $env:TARGET -AclObject $acl
        ''', TARGET=business)
        with self.assertRaises(PermissionError):
            (business / 'write-probe').write_bytes(b'must fail')
        code, error = self.generate(formal=True, output=business / 'generated')
        self.assertNotEqual(code, 0)
        self.assertIn('FORMAL_OUTPUT_OUTSIDE_DELIVERY', error)
        self.assertEqual(list(business.iterdir()), [marker])
        self.assertEqual(marker.read_bytes(), b'business source stays unchanged')

    def test_page_resource_missing_provenance_does_not_publish(self):
        self.result_fixture()
        self.page_asset_fixture()
        self.resource_entry({"provenance": ""})
        output = self.formal_output()
        code, error = self.generate(formal=True, output=output)
        self.assertNotEqual(code, 0)
        self.assertIn("PAGE_RESOURCE_PROVENANCE_REQUIRED", error)
        self.assertFalse(output.exists())

    def test_page_resource_path_escape_does_not_publish(self):
        self.result_fixture()
        self.page_asset_fixture()
        self.resource_entry({"alias": "../outside.svg"})
        output = self.formal_output()
        code, error = self.generate(formal=True, output=output)
        self.assertNotEqual(code, 0)
        self.assertIn("INVALID_PAGE_RESOURCE_PATH", error)
        self.assertFalse(output.exists())


if __name__ == "__main__":
    unittest.main()
