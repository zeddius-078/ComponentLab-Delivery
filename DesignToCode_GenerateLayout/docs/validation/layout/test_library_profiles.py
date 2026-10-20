"""验证可切换组件库配置：默认 token-sdk 行为不变，qt-native 批量改名为 Q* 并只保留公开 Qt API，
custom-library 支持批量改名与逐组件覆盖（渐进式适配）。"""
import json
import os
from pathlib import Path
import subprocess
import tempfile
import unittest

PROJECT = Path(__file__).resolve().parents[3]
PIPELINE = PROJECT / "Source/HtmlDsl/pipeline"
FIXTURES = PROJECT / "docs/fixtures/layout"
COMPILER = Path(os.environ.get(
    "D2C_COMPILER",
    str(PROJECT / "build/tools/layout/bin/v2_ui_compiler.exe"),
))


class LibraryProfileTests(unittest.TestCase):
    """使用同一个 CLI 检查三种组件库配置的类名与调用策略切换。"""

    def setUp(self):
        (PROJECT / 'output/work').mkdir(parents=True, exist_ok=True)
        self.review = json.loads(
            (FIXTURES / "design-inputs/review-page/canonical.raw.json").read_text(
                encoding="utf-8"
            )
        )

    def generate(self, raw, output, profile=None):
        input_path = output.parent / "input.json"
        input_path.write_text(json.dumps(raw, ensure_ascii=False), encoding="utf-8")
        environment = dict(os.environ, QT_QPA_PLATFORM="offscreen", D2C_RUNTIME_ROOT=str(PROJECT / "build/tools/layout"), D2C_WORK_ROOT=str(PROJECT))
        qt_root = environment.get("QT_ROOT")
        if qt_root:
            environment["PATH"] = str(Path(qt_root) / "bin") + os.pathsep + environment.get("PATH", "")
        command = [
            str(COMPILER),
            "--raw", str(input_path),
            "--mapping", str(FIXTURES / "mapping"),
            "--extensions", str(FIXTURES / "mapping/extensions.json"),
            "--font", str(PROJECT / "docs/examples/resources/HarmonyOS_Sans_SC_Regular.ttf"),
            "--output", str(output),
            "--strict-layout",
        ]
        if profile:
            command += ["--library-profile", profile]
        return subprocess.run(
            command, capture_output=True, timeout=60, env=environment,
        )

    def classes_in(self, output):
        text = Path(str(output) + ".ui").read_text(encoding="utf-8")
        return {
            line.split('class="')[1].split('"')[0]
            for line in text.splitlines()
            if 'class="' in line and "<widget" in line
        }

    def test_token_sdk_default_unchanged(self):
        with tempfile.TemporaryDirectory(dir=PROJECT / 'output/work', prefix="lib-token-") as directory:
            result = self.generate(self.review, Path(directory) / "page")
            self.assertEqual(
                result.returncode, 0, result.stderr.decode("utf-8", errors="replace")
            )
            classes = self.classes_in(Path(directory) / "page")
            self.assertTrue(any(c.startswith("commonWidget::Z_") for c in classes))
            components = json.loads(
                Path(str(Path(directory) / "page") + ".components.json").read_text(
                    encoding="utf-8"
                )
            )
            self.assertEqual(components["libraryProfile"], "token-sdk")

    def test_qt_native_batch_rename_and_api_filter(self):
        with tempfile.TemporaryDirectory(dir=PROJECT / 'output/work', prefix="lib-qt-") as directory:
            result = self.generate(self.review, Path(directory) / "page", profile="qt-native")
            self.assertEqual(
                result.returncode, 0, result.stderr.decode("utf-8", errors="replace")
            )
            classes = self.classes_in(Path(directory) / "page")
            self.assertTrue(classes)
            for cls in classes:
                self.assertTrue(
                    cls.startswith("Q"),
                    f"qt-native 类必须是公开 Qt 类，得到: {cls}",
                )
            self.assertTrue("QPushButton" in classes)
            components = json.loads(
                Path(str(Path(directory) / "page") + ".components.json").read_text(
                    encoding="utf-8"
                )
            )
            self.assertEqual(components["libraryProfile"], "qt-native")
            allowed = {
                "setText", "setEnabled", "setReadOnly", "setChecked",
                "setPlaceholderText", "setPlaceholder", "setCurrentIndex",
                "setValue", "setRange", "setFormat", "setTitle", "setGrid",
            }
            for entry in components["components"]:
                for call in entry.get("calls", []):
                    self.assertIn(call["api"], allowed)
                # SDK 专属 setter（setType/setControlSize 等）应被透明记录为跳过。
                skipped = [c["api"] for c in entry.get("skippedCalls", [])]
                if skipped:
                    self.assertNotIn("setType", entry.get("calls", []))

    def test_custom_library_batch_rename_and_override(self):
        with tempfile.TemporaryDirectory(dir=PROJECT / 'output/work', prefix="lib-custom-") as directory:
            result = self.generate(self.review, Path(directory) / "page", profile="custom-library")
            self.assertEqual(
                result.returncode, 0, result.stderr.decode("utf-8", errors="replace")
            )
            classes = self.classes_in(Path(directory) / "page")
            self.assertIn("MyOrg::PushButton", classes)
            self.assertTrue(any(c.startswith("MyOrg::") for c in classes))

    def test_component_override_wins_over_batch_rename(self):
        with tempfile.TemporaryDirectory(dir=PROJECT / 'output/work', prefix="lib-priority-") as directory:
            result = self.generate(self.review, Path(directory) / "page", profile="qt-native")
            self.assertEqual(
                result.returncode, 0, result.stderr.decode("utf-8", errors="replace")
            )
            classes = self.classes_in(Path(directory) / "page")
            # ComponentLab/Input 同时被 classRenames（-> QInputField）与
            # componentOverrides（-> QLineEdit）覆盖；override 必须胜出。
            self.assertIn("QLineEdit", classes)
            self.assertNotIn("QInputField", classes)
            components = json.loads(
                Path(str(Path(directory) / "page") + ".components.json").read_text(
                    encoding="utf-8"
                )
            )
            input_entries = [
                entry for entry in components["components"]
                if entry.get("componentName") == "ComponentLab/Input"
            ]
            self.assertTrue(input_entries)
            for entry in input_entries:
                self.assertEqual(entry["qtClass"], "QLineEdit")

    def test_unknown_profile_rejected(self):
        with tempfile.TemporaryDirectory(dir=PROJECT / 'output/work', prefix="lib-unknown-") as directory:
            result = self.generate(self.review, Path(directory) / "page", profile="nope")
            self.assertNotEqual(result.returncode, 0)
            self.assertIn(
                "UNKNOWN_LIBRARY_PROFILE",
                result.stderr.decode("utf-8", errors="replace"),
            )


if __name__ == "__main__":
    unittest.main()
