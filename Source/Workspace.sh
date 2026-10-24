#!/usr/bin/env bash
# 工作区级 Linux 入口：doctor / build / verify / clean，协调三个项目。
# 用法: bash Source/Workspace.sh <doctor|build|verify|clean> [all|component|layout|gallery]
set -euo pipefail

ACTION="${1:-doctor}"
TARGET="${2:-all}"
case "$ACTION" in
    doctor|build|verify|clean) ;;
    *) echo "Action must be doctor|build|verify|clean" >&2; exit 2 ;;
esac
case "$TARGET" in
    all|component|layout|gallery) ;;
    *) echo "Target must be all|component|layout|gallery" >&2; exit 2 ;;
esac

SOURCE_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
WORKSPACE="$(dirname "${SOURCE_ROOT}")"
COMPONENT="$WORKSPACE/DesignToCode_GenerateComponent"
LAYOUT="$WORKSPACE/DesignToCode_GenerateLayout"
GALLERY="$WORKSPACE/ComponentLab"
PY="${D2C_PYTHON_EXE:-python3.9}"
QT_ROOT="${QT_ROOT:-/usr/lib/x86_64-linux-gnu}"
CMAKE_EXE="${CMAKE_EXE:-cmake}"
SDK_ROOT="${GENERATED_COMPONENT_SDK_ROOT:-$COMPONENT/build/sdk}"
NODE_MODULES="${D2C_NODE_MODULES:-$HOME/d2c-node-modules/node_modules}"
BROWSER_EXE="${D2C_BROWSER_EXE:-$HOME/.cache/ms-playwright/chromium_headless_shell-1248/chrome-headless-shell-linux64/chrome-headless-shell}"

export D2C_SUPPORT_ROOT="$SOURCE_ROOT"
export D2C_PYTHON_EXE="$PY"
export QT_ROOT="$QT_ROOT"
export PYTHONDONTWRITEBYTECODE=1
export GENERATED_COMPONENT_SDK_ROOT="$SDK_ROOT"

"$PY" -B -c 'import sys; sys.exit(0 if sys.version_info[:2] == (3, 9) else 1)' \
    || { echo "PYTHON39_REQUIRED: $PY" >&2; exit 2; }
"$PY" -B "$SOURCE_ROOT/AgentCapabilityProfile.py" verify >/dev/null \
    || { echo "AGENT_CAPABILITY_PROFILE_INVALID" >&2; exit 2; }

py_step() {
    "$PY" -B "$@"
}

if [ "$ACTION" = "clean" ]; then
    case "$TARGET" in
        component) PROJECTS=("$COMPONENT") ;;
        layout) PROJECTS=("$LAYOUT") ;;
        gallery) PROJECTS=("$GALLERY") ;;
        all) PROJECTS=("$COMPONENT" "$LAYOUT" "$GALLERY") ;;
    esac
    for project in "${PROJECTS[@]}"; do
        py_step "$SOURCE_ROOT/ArtifactLifecycle.py" clean --project "$project" --dry-run
        echo "Above paths would be removed from $project. Remove --dry-run to execute."
    done
    exit 0
fi

verify_sdk() {
    py_step "$SOURCE_ROOT/SDKContract.py" verify --sdk "$SDK_ROOT" --configuration Release
}

if [ "$ACTION" = "doctor" ]; then
    echo "Python: $("$PY" -B -c 'import sys;print(sys.version)')"
    echo "Qt: $(qmake -query QT_VERSION 2>/dev/null || echo missing)"
    echo "CMake: $("$CMAKE_EXE" --version | head -1)"
    echo "Ninja: $(ninja --version 2>/dev/null || echo missing)"
    echo "Node: $(node --version 2>/dev/null || echo missing)"
    echo "Execution capability baseline: DS-V4F-BUILDUNK-NT-C256K-TEXT-AGENT-PROVIDERUNK [controlled-low-autonomy]"
    if [ -f "$SDK_ROOT/manifest/component-sdk.json" ]; then
        verify_sdk
    else
        echo "SDK: $SDK_ROOT [not published; not source damage]"
    fi
    exit 0
fi

if [ "$ACTION" = "build" ]; then
    if [ "$TARGET" = "all" ] || [ "$TARGET" = "component" ]; then
        bash "$COMPONENT/Source/Publish-ComponentSDK.sh" \
            "$COMPONENT/output/work/cache/GeneratedComponentSDK"
    fi
    verify_sdk
    if [ "$TARGET" = "all" ] || [ "$TARGET" = "layout" ]; then
        bash "$LAYOUT/Source/Build.sh" --sdk "$SDK_ROOT"
    fi
    if [ "$TARGET" = "all" ] || [ "$TARGET" = "gallery" ]; then
        bash "$GALLERY/Source/delivery/tools/Build.sh" --sdk "$SDK_ROOT"
    fi
    echo "BUILD DONE: $TARGET"
    exit 0
fi

# verify
verify_sdk
if [ "$TARGET" = "all" ] || [ "$TARGET" = "component" ]; then
    py_step "$COMPONENT/output/work/generated/Source/ComponentKit/design-assets/DesignSystem/scripts/v2/tests/test_v2.py"
    py_step "$COMPONENT/Source/CheckProject.py"
fi
if [ "$TARGET" = "all" ] || [ "$TARGET" = "layout" ]; then
    py_step "$LAYOUT/Source/CheckProject.py" || true
    export D2C_NODE_MODULES="$NODE_MODULES"
    export NODE_EXE="$(command -v node)"
    export D2C_BROWSER_EXE="$BROWSER_EXE"
    py_step -m unittest discover -s "$LAYOUT/docs/validation/formal" -p 'test_*.py'
    py_step -m unittest discover -s "$LAYOUT/docs/validation" -p 'test_*.py' \
        --top-level-directory "$LAYOUT/docs/validation" || true
fi
if [ "$TARGET" = "all" ] || [ "$TARGET" = "gallery" ]; then
    py_step "$GALLERY/Source/delivery/tools/CheckProject.py"
fi
py_step -m unittest discover -s "$WORKSPACE/docs/validation" -p 'test_*.py' || true
echo "Requested development regression finished; fresh-machine and visual acceptance remain separate."
