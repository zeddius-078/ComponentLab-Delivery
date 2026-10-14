"""Validate that GenerateLayout consumes the published component SDK only."""
import ast
import json
import os
from pathlib import Path
import unittest
import xml.etree.ElementTree as ET

ROOT = Path(__file__).resolve().parents[1]
PROJECT = ROOT.parent
SDK = Path(os.environ.get(
    'GENERATED_COMPONENT_SDK_ROOT',
    PROJECT.parent / 'DesignToCode_GenerateComponent' / 'build' / 'sdk',
)).resolve()


class IndependenceTests(unittest.TestCase):
    def test_resources_and_published_sdk(self):
        # Layout-owned resources stay local; component contracts enter through the authoritative SDK.
        self.assertFalse((PROJECT / 'build/sdk').exists())
        self.assertFalse((ROOT / 'Import-GeneratedComponentSDK.ps1').exists())
        self.assertTrue((ROOT / 'Verify-GeneratedComponentSDK.ps1').is_file())
        for qrc in ROOT.rglob('*.qrc'):
            for node in ET.parse(qrc).findall('.//file'):
                resolved = (qrc.parent / node.text).resolve()
                self.assertTrue(resolved.is_relative_to(ROOT), str(resolved))
                self.assertTrue(resolved.is_file(), str(resolved))
        manifest = json.loads((SDK / 'manifest/component-sdk.json').read_bytes())
        self.assertEqual(manifest['owner'], 'DesignToCode_GenerateComponent')
        self.assertTrue(manifest['componentAuthority'])
        self.assertEqual(manifest['linkage'], 'static')
        self.assertTrue((SDK / 'lib/GeneratedComponentSDK.lib').is_file())
        self.assertTrue((SDK / 'include/MyDesignSystem/MyDesignSystem.h').is_file())
        self.assertTrue((SDK / 'resources/tokens/default').is_dir())
        self.assertFalse(list(SDK.rglob('*.exe')))
        self.assertFalse(list(SDK.rglob('*.dll')))
        self.assertFalse((ROOT / 'ComponentKit').exists())
        self.assertFalse((PROJECT / 'build/dist').exists())
        self.assertFalse((PROJECT / 'build/dependencies').exists())
        demo = PROJECT / 'build/demo/page_examples'
        if demo.exists():
            self.assertFalse(list(demo.rglob('*.lib')))
            self.assertFalse(list(demo.rglob('*.h')))

    def test_no_sibling_project_or_python_ui(self):
        # Generated evidence may retain design names; inspect executable code and build configuration.
        for path in ROOT.rglob('*'):
            if not path.is_file() or path == Path(__file__):
                continue
            if path.suffix not in {'.cpp', '.h', '.ps1', '.py', '.cjs', '.qrc'} and path.name != 'CMakeLists.txt':
                continue
            raw = path.read_text(encoding='utf-8-sig')
            text = raw.replace('\\', '/')
            # 组件项目只通过已发布的权威 SDK 接入；这条发布路径属于允许的边界依赖。
            text = text.replace(
                '/ComponentLab-Delivery/DesignToCode_GenerateComponent/build/sdk/',
                '/AUTHORIZED_GENERATE_COMPONENT_SDK/',
            )
            self.assertNotIn('MappingWorkbench', text, str(path))
            for name in ('ComponentLab', 'DesignToCode_GenerateComponent'):
                self.assertNotIn('/' + name + '/', text, str(path))
            if path.suffix == '.py':
                # Python is allowed for input processing; the running GUI remains C++ Qt.
                for node in ast.walk(ast.parse(raw)):
                    names = ([item.name for item in node.names] if isinstance(node, ast.Import)
                             else [node.module or ''] if isinstance(node, ast.ImportFrom) else [])
                    self.assertFalse(any(n.startswith(('PySide', 'PyQt', 'shiboken')) for n in names), str(path))


if __name__ == '__main__':
    unittest.main()
