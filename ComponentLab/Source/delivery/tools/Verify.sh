#!/usr/bin/env bash
# ComponentLab 暂存包的 Linux 本机验收：治理检查、离屏运行、截图与运行时报告契约。
# 用法: bash Verify.sh --dist <暂存目录> [--report <报告目录>] [--sdk <SDK目录>]
set -euo pipefail

TOOLS_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
SOURCE_ROOT="$(dirname "$(dirname "$TOOLS_ROOT")")"
PROJECT_ROOT="$(dirname "${SOURCE_ROOT}")"
WORKSPACE="$(dirname "${PROJECT_ROOT}")"
SUPPORT="${D2C_SUPPORT_ROOT:-$WORKSPACE/Source}"
PY="${D2C_PYTHON_EXE:-python3.9}"
DIST=""
REPORT_DIR=""
SDK_ROOT=""
while [ $# -gt 0 ]; do
    case "$1" in
        --dist) DIST="$2"; shift 2 ;;
        --report) REPORT_DIR="$2"; shift 2 ;;
        --sdk) SDK_ROOT="$2"; shift 2 ;;
        *) echo "Unknown argument: $1" >&2; exit 2 ;;
    esac
done
[ -n "$DIST" ] || DIST="$PROJECT_ROOT/build/tools/gallery"
DIST="$(cd "$DIST" && pwd)"
[ -n "$SDK_ROOT" ] || SDK_ROOT="${GENERATED_COMPONENT_SDK_ROOT:-$WORKSPACE/DesignToCode_GenerateComponent/build/sdk}"
STAMP="$(date +%Y%m%d-%H%M%S)"
[ -n "$REPORT_DIR" ] || REPORT_DIR="$PROJECT_ROOT/output/logs/verify-$STAMP"
mkdir -p "$REPORT_DIR"
RESULTS_FILE="$REPORT_DIR/results.jsonl"
: > "$RESULTS_FILE"

export D2C_SUPPORT_ROOT="$SUPPORT"
export D2C_PYTHON_EXE="$PY"
export PYTHONDONTWRITEBYTECODE=1
export GENERATED_COMPONENT_SDK_ROOT="$SDK_ROOT"

"$PY" -B "$SUPPORT/SDKContract.py" verify --sdk "$SDK_ROOT" --configuration Release \
    || { echo "SDK_VERIFICATION_FAILED" >&2; exit 1; }

CATALOG="$SDK_ROOT/resources/mappings/design-component-catalog.json"
[ -f "$CATALOG" ] || { echo "Authoritative design component catalog missing: $CATALOG" >&2; exit 1; }
EXPECTED_COMPONENTS="$("$PY" -B -c "import json;print(json.load(open('$CATALOG'))['componentCount'])")"
EXPECTED_SETS="$("$PY" -B -c "import json;print(json.load(open('$CATALOG'))['statistics']['figmaComponentSetCount'])")"
EXPECTED_VARIANTS="$("$PY" -B -c "import json;d=json.load(open('$CATALOG'));print(sum(len(s['variants']) for p in d['figmaPages'] for s in p['componentSets']))")"

export LD_LIBRARY_PATH="$SDK_ROOT/lib${LD_LIBRARY_PATH:+:$LD_LIBRARY_PATH}"
export QT_QPA_PLATFORM=offscreen
export QT_OPENGL=software

"$PY" -B "$TOOLS_ROOT/CheckProject.py" --dist "$DIST" 2>&1 | tee "$REPORT_DIR/governance.log" \
    || { echo "Project governance failed" >&2; exit 1; }

for profile in standard-light wide-light standard-dark wide-dark; do
    for appearance in solid gradient-elevated; do
        SCALE=1
        if [ "$profile" = "wide-dark" ] && [ "$appearance" = "gradient-elevated" ]; then SCALE=1.5; fi
        export QT_SCALE_FACTOR="$SCALE"
        CAPTURE="$REPORT_DIR/$profile/$appearance"
        mkdir -p "$CAPTURE"
        "$DIST/component_lab" --profile "$profile" --appearance "$appearance" \
            --capture-dir "$CAPTURE" --quit-after 30000 \
            >"$CAPTURE/stdout.log" 2>"$CAPTURE/stderr.log" \
            || { echo "Runtime exit failure: $profile/$appearance" >&2; exit 1; }

        REPORT_JSON="$CAPTURE/runtime.json"
        "$PY" -B - "$REPORT_JSON" "$appearance" "$EXPECTED_COMPONENTS" "$EXPECTED_SETS" "$EXPECTED_VARIANTS" <<'PYEOF'
import json, sys
report = json.load(open(sys.argv[1], encoding="utf-8"))
appearance, expected_components, expected_sets, expected_variants = sys.argv[2:6]
expected_components = int(expected_components)
expected_sets = int(expected_sets)
expected_variants = int(expected_variants)
expected_presets = ["solid", "gradient", "inset", "gradient-inset", "outline", "elevated", "gradient-elevated"]
pages = ["172:2710", "234:2717", "339:2", "242:2720", "443:2", "363:2"]
sections = "外观,分组与状态"
assert report["screens"] == 1, report["screens"]
assert report["capturedPages"] == 13, report["capturedPages"]
assert report["designSystemComponents"] == expected_components
assert report["figmaComponentSets"] == expected_sets
assert report["figmaVariants"] == expected_variants
assert report["nativeDescriptors"] == expected_components
assert report["nativeRegistryRows"] == expected_components
assert report["presets"] == expected_presets
assert report["appearance"] == appearance
for i in range(6):
    assert report["navigation"][i]["pageId"] == pages[i]
    assert ",".join(report["navigation"][i]["sections"]) == sections
print("runtime contract PASS")
PYEOF
        for i in 0 1 2 3 4 5; do
            for j in 0 1; do
                if [ ! -s "$CAPTURE/component-$i-$j.png" ] || [ "$(stat -c%s "$CAPTURE/component-$i-$j.png")" -lt 5000 ]; then
                    echo "Empty screenshot: component-$i-$j" >&2
                    exit 1
                fi
            done
        done
        if [ ! -s "$CAPTURE/component-mapping.png" ] || [ "$(stat -c%s "$CAPTURE/component-mapping.png")" -lt 5000 ]; then
            echo "Empty mapping screenshot" >&2
            exit 1
        fi
        if grep -qE 'Unknown property|Could not parse|Cannot load|QPainter::|Startup:|Capture:' "$CAPTURE/stderr.log"; then
            echo "Runtime rendering error in $profile/$appearance" >&2
            cat "$CAPTURE/stderr.log" >&2
            exit 1
        fi
        "$PY" -B -c "
import json, sys
record = {'profile': '$profile', 'appearance': '$appearance', 'scale': $SCALE, 'capture': '$CAPTURE'}
open('$RESULTS_FILE', 'a', encoding='utf-8').write(json.dumps(record, ensure_ascii=False) + '\n')"
        echo "PASS native offline simulation: $profile / $appearance / scale=$SCALE"
    done
done

"$PY" -B - "$REPORT_DIR/verification.json" "$DIST" "$SDK_ROOT" "$RESULTS_FILE" <<'PYEOF'
import hashlib, json, sys
output, dist, sdk, results_file = sys.argv[1:5]
sha = lambda p: hashlib.sha256(open(p, "rb").read()).hexdigest()
exe = dist + "/component_lab"
results = [json.loads(line) for line in open(results_file, encoding="utf-8") if line.strip()]
value = {
    "status": "PASS", "dist": dist, "exeSha256": sha(exe),
    "sdkManifestSha256": sha(sdk + "/manifest/component-sdk.json"),
    "cleanPath": True, "invalidExternalPluginPath": True,
    "freshMachineTest": False, "compilerChecks": True,
    "results": results,
}
json.dump(value, open(output, "w", encoding="utf-8"), ensure_ascii=False, indent=2)
PYEOF

echo "EVIDENCE: $REPORT_DIR"
