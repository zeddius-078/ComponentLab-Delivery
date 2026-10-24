#!/usr/bin/env bash
# DesignToCode_GenerateLayout 的 Linux 构建入口：编译 v2_ui_compiler、跑回归测试、
# 暂存受控工具并发布到 build/tools/layout。
# 用法: bash Build.sh [--sdk <SDK目录>] [--build-directory <目录>] [--compile-only]
set -euo pipefail

SOURCE_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(dirname "${SOURCE_ROOT}")"
WORKSPACE="$(dirname "${PROJECT_ROOT}")"
SUPPORT="${D2C_SUPPORT_ROOT:-$WORKSPACE/Source}"
PY="${D2C_PYTHON_EXE:-python3.9}"
QT_ROOT="${QT_ROOT:-/usr/lib/x86_64-linux-gnu}"
CMAKE_EXE="${CMAKE_EXE:-cmake}"
SDK_ROOT=""
BUILD_DIRECTORY=""
RUN_TESTS=1
PREVIEW=OFF
while [ $# -gt 0 ]; do
    case "$1" in
        --sdk) SDK_ROOT="$2"; shift 2 ;;
        --build-directory) BUILD_DIRECTORY="$2"; shift 2 ;;
        --compile-only) RUN_TESTS=0; shift ;;
        --preview) PREVIEW=ON; shift ;;
        *) echo "Unknown argument: $1" >&2; exit 2 ;;
    esac
done
[ -n "$SDK_ROOT" ] || SDK_ROOT="${GENERATED_COMPONENT_SDK_ROOT:-$WORKSPACE/DesignToCode_GenerateComponent/build/sdk}"

export D2C_SUPPORT_ROOT="$SUPPORT"
export D2C_PYTHON_EXE="$PY"
export QT_ROOT="$QT_ROOT"
export PYTHONDONTWRITEBYTECODE=1

"$PY" -B -c 'import sys; sys.exit(0 if sys.version_info[:2] == (3, 9) else 1)' \
    || { echo "PYTHON39_REQUIRED: $PY" >&2; exit 2; }

mkdir -p "$PROJECT_ROOT/build/.state" "$PROJECT_ROOT/output/work/cache"
exec 9>"$PROJECT_ROOT/build/.state/operation.lock"
flock -n 9 || { echo "PROJECT_OPERATION_LOCKED" >&2; exit 3; }

fail() { echo "FAILED: $*" >&2; exit 1; }

"$PY" -B "$SUPPORT/SDKContract.py" verify --sdk "$SDK_ROOT" --configuration Release \
    || fail "VERIFIED_SDK_REQUIRED"
SDK_MANIFEST_HASH="$(sha256sum "$SDK_ROOT/manifest/component-sdk.json" | cut -d' ' -f1)"
SOURCE_FINGERPRINT="$("$PY" -B "$SUPPORT/ArtifactLifecycle.py" fingerprint --project "$PROJECT_ROOT")"
[ -n "$SOURCE_FINGERPRINT" ] || fail "SOURCE_FINGERPRINT_FAILED"

[ -n "$BUILD_DIRECTORY" ] || BUILD_DIRECTORY="$PROJECT_ROOT/output/work/cache/V2-html-dsl"
if [ -f "$BUILD_DIRECTORY/CMakeCache.txt" ]; then
    CACHED_SDK="$(grep -E '^D2C_CONFIGURED_SDK_ROOT:INTERNAL=' "$BUILD_DIRECTORY/CMakeCache.txt" | cut -d= -f2- || true)"
    if [ -n "$CACHED_SDK" ] && [ "$(cd "$CACHED_SDK" && pwd)" != "$(cd "$SDK_ROOT" && pwd)" ]; then
        fail "SDK_CACHE_CONFLICT: use a new build directory when changing GENERATED_COMPONENT_SDK_ROOT."
    fi
fi

"$CMAKE_EXE" -S "$SOURCE_ROOT" -B "$BUILD_DIRECTORY" -G Ninja \
    -DCMAKE_BUILD_TYPE=Release \
    -DGENERATED_COMPONENT_SDK_ROOT="$SDK_ROOT" \
    -DCMAKE_PREFIX_PATH="$QT_ROOT" \
    -DD2C_BUILD_TESTS=$([ "$RUN_TESTS" = "1" ] && echo ON || echo OFF) \
    -DD2C_BUILD_PREVIEW="$PREVIEW" \
    || fail "Configure failed"
"$CMAKE_EXE" --build "$BUILD_DIRECTORY" --parallel 4 || fail "Build failed"
export LD_LIBRARY_PATH="$SDK_ROOT/lib${LD_LIBRARY_PATH:+:$LD_LIBRARY_PATH}"
if [ "$RUN_TESTS" = "1" ]; then
    ctest --test-dir "$BUILD_DIRECTORY" --output-on-failure || fail "Tests failed"
fi

STAGE="$PROJECT_ROOT/output/work/stage/layout-$(od -An -N8 -tx1 /dev/urandom | tr -d ' \n')"
mkdir -p "$STAGE/bin" "$STAGE/scripts" "$STAGE/resources"
cp "$BUILD_DIRECTORY/v2_ui_compiler" "$STAGE/bin/v2_ui_compiler"
Uic="${D2C_UIC_EXE:-}"
if [ -z "$Uic" ]; then
    for candidate in /usr/lib/qt5/bin/uic /usr/lib/x86_64-linux-gnu/qt5/bin/uic "$(command -v uic 2>/dev/null || true)"; do
        [ -n "$candidate" ] && [ -x "$candidate" ] && Uic="$candidate" && break
    done
fi
[ -n "$Uic" ] && cp "$Uic" "$STAGE/bin/uic"
cp -r "$SOURCE_ROOT/HtmlDsl/tools" "$STAGE/scripts/tools"
cp -r "$SOURCE_ROOT/HtmlDsl/pipeline/capture" "$STAGE/scripts/capture"
cp "$SOURCE_ROOT/HtmlDsl/PreviewStructure.py" "$STAGE/scripts/PreviewStructure.py"
for entry in RuntimeRun.py ArtifactLifecycle.py SDKContract.py; do
    cp "$SUPPORT/$entry" "$STAGE/scripts/$entry"
done
cat > "$STAGE/run.sh" <<'EOF'
#!/usr/bin/env bash
# 受控布局工具的 Linux 运行入口。需要 libGeneratedComponentSDK.so 在库搜索路径。
set -euo pipefail
TOOL_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
export QT_QPA_PLATFORM="${QT_QPA_PLATFORM:-offscreen}"
export LD_LIBRARY_PATH="${GENERATED_COMPONENT_SDK_ROOT:+$GENERATED_COMPONENT_SDK_ROOT/lib:}$LD_LIBRARY_PATH"
exec "$TOOL_ROOT/bin/v2_ui_compiler" "$@"
EOF
chmod +x "$STAGE/run.sh"
cp -r "$PROJECT_ROOT/docs/fixtures/layout/mapping" "$STAGE/resources/mapping"
cp "$PROJECT_ROOT/docs/examples/resources/HarmonyOS_Sans_SC_Regular.ttf" "$STAGE/resources/font.ttf"
cp "$PROJECT_ROOT/docs/examples/styles/initial-light.json" "$STAGE/resources/initial-light.json"
mkdir -p "$STAGE/licenses"
cp "$SDK_ROOT/licenses/"* "$STAGE/licenses/" 2>/dev/null || true
if [ "$(sha256sum "$SDK_ROOT/manifest/component-sdk.json" | cut -d' ' -f1)" != "$SDK_MANIFEST_HASH" ]; then
    fail "SDK_CHANGED_DURING_BUILD: do not combine new mappings with old binary."
fi

RECEIPT="$PROJECT_ROOT/output/logs/latest-success/layout-build.json"
mkdir -p "$(dirname "$RECEIPT")"
exec 9>&-
"$PY" -B "$SUPPORT/ArtifactLifecycle.py" receipt --project "$PROJECT_ROOT" --stage "$STAGE" \
    --sdk "$SDK_ROOT" --source-fingerprint "$SOURCE_FINGERPRINT" --checks ctest \
    --output "$RECEIPT" || fail "LAYOUT_ACCEPTANCE_FAILED"
"$PY" -B "$SUPPORT/ArtifactLifecycle.py" publish-tools --project "$PROJECT_ROOT" \
    --stage "$STAGE" --name layout --receipt "$RECEIPT" || fail "LAYOUT_PUBLICATION_FAILED"

echo "STABLE LAYOUT: $PROJECT_ROOT/build/tools/layout"
