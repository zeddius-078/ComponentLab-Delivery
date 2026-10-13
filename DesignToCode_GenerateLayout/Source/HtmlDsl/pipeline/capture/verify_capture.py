"""Verify saved MCP response bytes and verbatim DSL extraction."""
# 离线检查已保存的第 04 步响应与 DSL，执行时不连接 Pixso。
# 检查通过表示本地文件相互一致；服务身份、所属设计文件和图片来源另需证据。
# 当前检查使用 assert；命令见 docs/mainSkill/generate-layout-pipeline/references/设计到布局生成说明.md，避免用 -O/-OO 关闭断言。
# 命令从 DesignToCode 执行：
# python -B HtmlDsl/pipeline/capture/verify_capture.py HtmlDsl/pipeline/design-inputs/node-12-382-full
# -B 仅关闭 Python 字节码缓存写入，校验断言仍会执行。
import argparse
import hashlib
import json
from pathlib import Path
from capture_mcp import decode_rpc


# 输入为含 canonical.raw.json 的采集目录，输出为校验结果字典。
# 此函数只读取文件；失败时抛出异常。报告写入由下方命令行入口负责。
# directory 使用 pathlib.Path，目录必须包含下方读取的四个文件。
# 适用输入为完整采集的单文本块 DSL；多文本块和图片导出需各自的检查工具。
def verify(directory):
    # 检查范围固定为 04-call，未覆盖 01~03 响应及 design.png。
    request = json.loads((directory / '04-call.request.json').read_bytes())
    body = (directory / '04-call.response.body').read_bytes()
    meta = json.loads((directory / '04-call.response-meta.json').read_bytes())
    # 用响应正文重算哈希和长度，与采集时保存在本地的元数据比较。
    assert hashlib.sha256(body).hexdigest() == meta['sha256']
    assert len(body) == meta['bytes']
    # 复用采集器的解包规则，按请求 ID 取结果；适用于单文本块 DSL 采集。
    # 这里检查首个文本块，调用方需保证采集类型正确；此函数未遍历其他内容块。
    response = decode_rpc(body, request['id'])
    raw = (directory / 'canonical.raw.json').read_bytes()
    # 比较原始文本字节而非 JSON 对象，空白、字段顺序等改动也会使校验失败。
    assert raw == response['content'][0]['text'].encode('utf-8')
    data = json.loads(raw)
    args = request['params']['arguments']
    # 完整树形 DSL 同时要求保留组件实例，并核对返回根节点与请求 GUID。
    # simplify=true 时只执行上面的响应和文本一致性检查。
    if not args['simplify']:
        assert args['isDetachInstance'] is False
        assert data['pixTreeNodes'][0]['guid'] == args['guid']
    # receivedAtUtc 沿用采集时间；两种哈希分别对应 DSL 文本和外层响应正文。
    # verbatimText 表示逐字节相等；topLevelFields 只列字段名，不验证设计字段语义。
    # PASS 仅覆盖上述一致性检查，实际服务身份及设计文件归属需额外来源记录。
    return {'status': 'PASS', 'directory': directory.name,
            'arguments': args, 'receivedAtUtc': meta['receivedAtUtc'],
            'rawBytes': len(raw), 'rawSha256': hashlib.sha256(raw).hexdigest(),
            'responseSha256': meta['sha256'], 'verbatimText': True,
            'topLevelFields': list(data)}


# 被其他检查脚本导入时只提供 verify 函数；直接运行才解析命令并保存报告。
if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('directory', type=Path)
    args = parser.parse_args()
    result = verify(args.directory)
    # 全部检查通过后才写报告，并覆盖同名旧报告；失败时旧报告可能仍在。
    # 判断本次结果应同时查看命令退出状态，避免把旧 PASS 当作本次通过。
    (args.directory / 'verification.json').write_text(
        json.dumps(result, indent=2, ensure_ascii=False), encoding='utf-8')
    print(json.dumps(result, ensure_ascii=False))
