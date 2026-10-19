"""Focused contracts for generated native component adapters."""

from __future__ import annotations

import unittest

import generate_native_components as generator


TABLE = {
    "qtClass": "QTableView",
    "sdkClass": "Z_TableView",
    "designComponent": "TableView",
    "category": "data",
    "implementation": "generated",
}


class TableGenerationTests(unittest.TestCase):
    def test_table_exposes_registry_axes_and_consumes_table_tokens(self):
        header = generator.header_text(TABLE)
        source = generator.source_text(TABLE)
        qss = generator.qss_template(TABLE)

        self.assertIn("Q_PROPERTY(QString density", header)
        self.assertIn("Q_PROPERTY(QString striping", header)
        self.assertIn("table_detail::Runtime", source)
        self.assertIn("component.table.base.density.", source)
        self.assertIn("setAlternatingRowColors", source)
        self.assertIn("@TABLE_HEADER_BACKGROUND@", qss)
        self.assertIn("@TABLE_SELECTED_BACKGROUND@", qss)
        self.assertIn("QHeaderView::section", qss)


if __name__ == "__main__":
    unittest.main()
