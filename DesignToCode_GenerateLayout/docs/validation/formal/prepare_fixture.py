"""生成隔离 Compact 技术测试页；不修改原始设计，不代表 UCD 已签核。"""
import argparse
import hashlib
import json
from pathlib import Path
import re
from test_generator_contract import GeneratorContractTests, PROJECT


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--batch", required=True)
    parser.add_argument("--page", choices=["review", "result", "horizontal"], default="review")
    parser.add_argument("--page-assets", action="store_true")
    args = parser.parse_args()
    if not re.fullmatch(r"[A-Za-z0-9][A-Za-z0-9_-]*", args.batch):
        raise ValueError("INVALID_BATCH")
    case = GeneratorContractTests()
    case.setUp()
    try:
        sources = {
            "result": PROJECT / "output/work/cache/formal-inputs/result/hybrid-result.json",
            "horizontal": PROJECT / "output/work/cache/Case-27-450/working/hybrid-result.json",
        }
        if args.page in sources:
            case.raw = json.loads(sources[args.page].read_bytes())
        source = json.dumps(case.raw, sort_keys=True).encode()
        case.compact_fixture()
        if args.page_assets:
            if args.page == "review":
                raise ValueError("RESOURCE_FIXTURE_REQUIRES_ICON_CONSUMER")
            case.page_asset_fixture()
        output = PROJECT / "output/delivery" / args.batch / args.page
        code, error = case.generate(formal=True, output=output)
        if code:
            raise RuntimeError(error)
        record_path = PROJECT / "output/logs/formal-delivery" / args.batch / args.page / "generation-record.json"
        record = json.loads(record_path.read_bytes())
        record["fixture"] = {
            "purpose": "Compact technical contract verification; not approved design",
            "change": "Profile variants only; explicit layout and typography unchanged",
            "originalNormalizedSha256": hashlib.sha256(source).hexdigest(),
        }
        record_path.write_text(json.dumps(record, ensure_ascii=False, indent=2), encoding="utf-8")
        print(output)
    finally:
        case.doCleanups()


if __name__ == "__main__":
    main()
