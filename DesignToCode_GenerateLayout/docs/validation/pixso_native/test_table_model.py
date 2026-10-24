import copy
from pathlib import Path
import sys
import unittest

ROOT = Path(__file__).resolve().parents[3]
sys.path.insert(0, str(ROOT / "Source/pixso_native"))
from component_binding import bind_table
from component_engine import ComponentEngine, ComponentError


class TableTests(unittest.TestCase):
    def setUp(self):
        self.spec = {"file": "Table.vue", "when": {"State": "Default"},
                     "headers": ["h1", "h2"], "rows": [["a1", "a2"]]}
        self.record = {"source": {"instancePath": "root/table"},
                       "declaredProps": {"State": "Default"}, "content": {"text": []}}
        for slot, text in zip(["h1", "h2", "a1", "a2"], ["Name", "Value", "Item", "001"]):
            self.record["content"]["text"].append({"text": text, "slots": [
                {"file": "Table.vue", "name": slot, "instancePath": "root/table"}]})

    def test_matrix_uses_explicit_slots_not_list_order(self):
        self.record["content"]["text"].reverse()
        table, consumed = bind_table(self.record, self.spec)
        self.assertEqual(table, {"headers": ["Name", "Value"], "rows": [["Item", "001"]]})
        self.assertEqual(len(consumed), 4)
        calls = ComponentEngine.table_model("table", table)
        self.assertIn("new QStandardItemModel(1, 2, ui->table)", calls[0])
        self.assertTrue(any("setFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable)" in c for c in calls))
        self.assertIn("ui->table->setModel(table_model);", calls)
        self.assertFalse(any("JSON" in c for c in calls))

    def test_matrix_rejects_missing_reused_or_foreign_slot(self):
        for mode in ("missing", "reused", "foreign", "variant", "width"):
            record, spec = copy.deepcopy(self.record), copy.deepcopy(self.spec)
            if mode == "missing":
                record["content"]["text"].pop()
            elif mode == "reused":
                spec["rows"][0][0] = "h1"
            elif mode == "foreign":
                record["content"]["text"][0]["slots"][0]["instancePath"] = "root/other"
            elif mode == "variant":
                record["declaredProps"]["State"] = "Disabled"
            else:
                spec["rows"][0].pop()
            with self.subTest(mode=mode), self.assertRaises(ComponentError):
                bind_table(record, spec)

    def test_model_rejects_implicit_numeric_conversion(self):
        with self.assertRaises(ComponentError):
            ComponentEngine.table_model("table", {"headers": ["Value"], "rows": [[128]]})


if __name__ == "__main__":
    unittest.main()
