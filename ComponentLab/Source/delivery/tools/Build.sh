#!/usr/bin/env bash
# ComponentLab 的 Linux 构建入口：编译展示程序与回归测试、暂存受控工具、
# 验收并发布到 build/tools/gallery。
# 用法: bash Build.sh [--sdk <SDK目录>] [--build-directory <目录>] [--skip-tests]
set -euo pipefail

TOOLS_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
SOURCE_ROOT="$(dirname "$(dirname "$TOOLS_ROOT")")"
PROJECT_ROOT="$(dirname "${SOURCE_ROOT}")"
WORKSPACE="$(dirname "${PROJECT_ROOT}")"
SUPPORT="${D2C_SUPPORT_ROOT:-$WORKSPACE/Source}"
PY="${D2C_PYTHON_EXE:-python3.9}"
QT_ROOT="${QT_ROOT:-/usr/lib/x86_64-linux-gnu}"
CMAKE_EXE="${CMAKE_EXE:-cmake}"
SDK_ROOT=""
BUILD_DIRECTORY=""
SKIP_TESTS=0
while [ $# -gt 0 ]; do
    case "$1" in
        --sdk) SDK_ROOT="$2"; shift 2 ;;
        --build-directory) BUILD_DIRECTORY="$2"; shift 2 ;;
        --skip-tests) SKIP_TESTS=1; shift ;;
        *) echo "Unknown argument: $1" >&2; exit 2 ;;
    esac
done
[ -n "$SDK_ROOT" ] || SDK_ROOT="${GENERATED_COMPONENT_SDK_ROOT:-$WORKSPACE/DesignToCode_GenerateComponent/build/sdk}"

export D2C_SUPPORT_ROOT="$SUPPORT"
export D2C_PYTHON_EXE="$PY"
export QT_ROOT="$QT_ROOT"
export PYTHONDONTWRITEBYTECODE=1
export GENERATED_COMPONENT_SDK_ROOT="$SDK_ROOT"

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

[ -n "$BUILD_DIRECTORY" ] || BUILD_DIRECTORY="$PROJECT_ROOT/output/work/cache/gallery-release"
RUN_ID="$(date +%Y%m%d-%H%M%S)-$(od -An -N4 -tx1 /dev/urandom | tr -d ' \n')"
LOGS="$PROJECT_ROOT/output/logs/build-$RUN_ID"
STAGE="$PROJECT_ROOT/output/work/stage/gallery-$RUN_ID"
mkdir -p "$LOGS" "$STAGE" "$LOGS/tests"

if [ -f "$BUILD_DIRECTORY/CMakeCache.txt" ]; then
    CACHED_SDK="$(grep -E '^(D2C_CONFIGURED_SDK_ROOT:INTERNAL|GENERATED_COMPONENT_SDK_ROOT:PATH)=' "$BUILD_DIRECTORY/CMakeCache.txt" | head -1 | cut -d= -f2- || true)"
    if [ -n "$CACHED_SDK" ] && [ "$(cd "$CACHED_SDK" 2>/dev/null && pwd)" != "$(cd "$SDK_ROOT" && pwd)" ]; then
        fail "SDK_CACHE_CONFLICT: use a fresh output/work cache for a different SDK."
    fi
fi

"$CMAKE_EXE" -S "$SOURCE_ROOT" -B "$BUILD_DIRECTORY" -G Ninja \
    -DCMAKE_BUILD_TYPE=Release \
    -DGENERATED_COMPONENT_SDK_ROOT="$SDK_ROOT" \
    -DCMAKE_PREFIX_PATH="$QT_ROOT" \
    -DPython3_EXECUTABLE="$PY" \
    -DTEST_OUTPUT_DIR="$LOGS/tests" \
    -DCOMPONENTLAB_BUILD_DEMO=ON \
    -DBUILD_TESTING=ON 2>&1 | tee "$LOGS/configure.log" || fail "Configure failed"
"$CMAKE_EXE" --build "$BUILD_DIRECTORY" --parallel 4 2>&1 | tee "$LOGS/compile.log" || fail "Compile failed"
"$CMAKE_EXE" --install "$BUILD_DIRECTORY" --prefix "$STAGE" || fail "Install stage failed"
rm -f "$STAGE/qt.conf"
cat > "$STAGE/run.sh" <<'EOF'
#!/usr/bin/env bash
# 受控展示程序的 Linux 运行入口。需要 libGeneratedComponentSDK.so 在库搜索路径。
set -euo pipefail
DIST="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
export QT_QPA_PLATFORM="${QT_QPA_PLATFORM:-offscreen}"
export QT_OPENGL=software
export LD_LIBRARY_PATH="${GENERATED_COMPONENT_SDK_ROOT:+$GENERATED_COMPONENT_SDK_ROOT/lib:}$LD_LIBRARY_PATH"
exec "$DIST/component_lab" "$@"
EOF
chmod +x "$STAGE/run.sh"

export LD_LIBRARY_PATH="$SDK_ROOT/lib${LD_LIBRARY_PATH:+:$LD_LIBRARY_PATH}"
export QT_OPENGL=software

"$PY" -B "$TOOLS_ROOT/CheckProject.py" --dist "$STAGE" --snapshot "$LOGS/validation-inputs.json" \
    || fail "Project governance failed"

if [ "$SKIP_TESTS" = "0" ]; then
    ctest --test-dir "$BUILD_DIRECTORY" -j 1 --output-on-failure \
        --output-junit "$LOGS/ctest.xml" || fail "ctest failed"
    export QT_QPA_PLATFORM=offscreen
    mkdir -p "$LOGS/token"
    for profile in standard-light wide-light standard-dark wide-dark; do
        mkdir -p "$LOGS/token/$profile"
        "$BUILD_DIRECTORY/token_probe" --token-dir "$SDK_ROOT/resources/tokens/default" \
            --profile "$profile" --output "$LOGS/token/$profile" \
            || fail "token probe failed: $profile"
    done
    unset QT_QPA_PLATFORM
fi

bash "$TOOLS_ROOT/Verify.sh" --dist "$STAGE" --report "$LOGS/native" --sdk "$SDK_ROOT" \
    || fail "Native staged validation failed"

if [ "$(sha256sum "$SDK_ROOT/manifest/component-sdk.json" | cut -d' ' -f1)" != "$SDK_MANIFEST_HASH" ]; then
    fail "SDK_CHANGED_DURING_BUILD: do not combine new mappings with old binary."
fi

SUCCESS="$PROJECT_ROOT/output/logs/latest-success/gallery-build.json"
mkdir -p "$(dirname "$SUCCESS")"
exec 9>&-
"$PY" -B "$SUPPORT/ArtifactLifecycle.py" receipt --project "$PROJECT_ROOT" --stage "$STAGE" \
    --sdk "$SDK_ROOT" --source-fingerprint "$SOURCE_FINGERPRINT" \
    --checks ctest,token,native --output "$SUCCESS" || fail "GALLERY_ACCEPTANCE_FAILED"
"$PY" -B "$SUPPORT/ArtifactLifecycle.py" publish-tools --project "$PROJECT_ROOT" \
    --stage "$STAGE" --name gallery --receipt "$SUCCESS" || fail "GALLERY_PUBLICATION_FAILED"

echo "STABLE GALLERY: $PROJECT_ROOT/build/tools/gallery"
