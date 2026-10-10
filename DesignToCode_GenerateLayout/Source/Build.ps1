# V1 构建入口：使用本机 MSVC、Qt5 和 Ninja，构建缓存写入 D 盘。
# 加 -Test 会运行组件与布局测试；本脚本不部署 Offline 目录。
param([switch]$Test)
$ErrorActionPreference = 'Stop'
. "$PSScriptRoot/NativeTools.ps1"
Initialize-NativeTools
$build = "$D2CProjectRoot/build/cache/V1"
# 每个原生命令后检查退出码，配置或编译失败时立即停止后续步骤。
& $NativeCMake -S $PSScriptRoot -B $build -G Ninja '-DCMAKE_BUILD_TYPE=Release' "-DCMAKE_PREFIX_PATH=$env:QT_ROOT"
# 配置失败意味着构建目录或依赖尚未就绪，立即停止，避免继续使用旧的生成配置。
if ($LASTEXITCODE -ne 0) { throw 'Configure failed' }
& $NativeCMake --build $build
# 编译失败时停止后续步骤，防止旧 EXE 被当作本轮新产物使用。
if ($LASTEXITCODE -ne 0) { throw 'Build failed' }
# 仅在显式请求测试时运行测试目标，使普通构建与验证构建的职责可区分。
if ($Test) {
    # 测试目标的 offscreen 环境和报告位置由 CMakeLists.txt 定义。
    & $NativeCTest --test-dir $build --output-on-failure
    # 测试未通过时向调用方抛错，阻止上层打包流程继续部署。
    if ($LASTEXITCODE -ne 0) { throw 'Tests failed' }
}
