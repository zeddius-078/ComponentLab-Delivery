"""Record explicit MCP calls without transforming design fields."""
# 构建期采集工具：通过 HTTP 调用 MCP，保存请求、响应正文和提取的文本。
# 设计文件由 Pixso 客户端当前打开的文档决定，节点由 --arguments 中的 guid 指定。
# 本工具不执行 Qt 组件映射或 Token 求值；来源边界见交付包 docs/DesignMapping/Guide.md。
# 文件分工：request 是本地请求，response.body 是服务正文，meta 是本地接收记录。
# canonical.raw.json 的内容来自响应中的文本块；它与外层响应文件的字节不同。
import argparse
import hashlib
import json
from datetime import datetime, timezone
from pathlib import Path
from urllib.request import Request, urlopen


# 输入：响应正文字节和本次请求 ID；输出：匹配消息的 result 对象。
# 此函数只解释传输包装，调用方保存的原始 body 字节保持不变。
# body 为 bytes；request_id 应与发送的 JSON-RPC id 一致，按原类型比较。
# 空正文返回 None；编码、JSON 格式、匹配数量或 RPC 错误直接向调用方抛出。
def decode_rpc(body, request_id):
    text = body.decode("utf-8")
    # 初始化完成通知可能返回空正文，无需解包。
    if not text.strip():
        return None
    # 按正文起始符区分普通 JSON 与 SSE，保证两种服务传输格式都能提取同一类 RPC 消息。
    if text.lstrip().startswith("{"):
        messages = [json.loads(text)]
    else:
        messages = []
        # SSE 以空行分隔事件；同一事件的多条 data: 行合并后解析为 JSON。
        # 换行标准化仅用于这里的解析，不回写原始响应文件。
        for event in text.replace("\r\n", "\n").split("\n\n"):
            data = "\n".join(line[5:].lstrip(" ") for line in event.splitlines() if line.startswith("data:"))
            # 只解析有 data 内容的事件，跳过 SSE 心跳或空事件，避免将空字符串当作 JSON。
            if data:
                messages.append(json.loads(data))
    # 只接受一个 ID 匹配的结果，避免把通知或其他请求的结果当成本次返回。
    matches = [message for message in messages if message.get("id") == request_id]
    # 要求唯一匹配当前请求 ID；零条表示缺失，多条表示歧义，两者都不能作为可靠工具结果。
    if len(matches) != 1:
        raise ValueError("Expected one matching MCP response")
    # 匹配到请求后先检查 RPC 错误，阻止错误对象进入后续设计数据提取流程。
    if "error" in matches[0]:
        raise ValueError(matches[0]["error"])
    return matches[0]["result"]


# 一次采集会话的传输记录器，维护 HTTP 头、请求序号和输出目录。
# directory 由入口预先创建；该类只保存通信内容，不判断设计字段的含义。
class Recorder:
    # endpoint 是完整 HTTP 地址；directory 是已存在的 pathlib.Path 目录。
    # 目录唯一性由 main 保证，直接使用 Recorder 时须由调用方保证文件名无冲突。
    def __init__(self, endpoint, directory):
        self.endpoint, self.directory = endpoint, directory
        self.headers = {"Content-Type": "application/json", "Accept": "application/json, text/event-stream"}
        self.sequence = 0

    # label 决定三类文件的共同前缀：request.json、response.body、response-meta.json。
    # method 是 RPC 方法名，params 是可序列化的参数；label 使用固定的本地文件名前缀。
    # 普通调用返回解包后的 result；通知仍等待 HTTP 返回，但不按 RPC id 查找结果。
    def rpc(self, label, method, params, notification=False):
        # 通知也占用一个本地序号，所以四步流程的目标工具请求 ID 为 4。
        self.sequence += 1
        payload = {"jsonrpc": "2.0", "method": method, "params": params}
        # 普通请求需要 id 才能匹配响应；通知省略 id，遵守 MCP 的 JSON-RPC 通知格式。
        if not notification:
            payload["id"] = self.sequence
        request = json.dumps(payload, ensure_ascii=False).encode("utf-8")
        # 先保存再发送同一份字节，网络调用失败时仍可检查请求参数。
        (self.directory / (label + ".request.json")).write_bytes(request)
        with urlopen(Request(self.endpoint, request, self.headers), timeout=60) as response:
            body = response.read()
            # 服务若分配会话 ID，后续请求沿用该值；当前元数据未保存此响应头。
            if response.headers.get("Mcp-Session-Id"):
                self.headers["Mcp-Session-Id"] = response.headers["Mcp-Session-Id"]
            # 接收时间来自本机时钟，哈希由本地计算，作用是核对文件一致性。
            # 当前记录未包含实际端点、设计文件身份或服务端签名。
            metadata = {"status": response.status, "contentType": response.headers.get("Content-Type"),
                        "receivedAtUtc": datetime.now(timezone.utc).isoformat(),
                        "sha256": hashlib.sha256(body).hexdigest(), "bytes": len(body)}
        # 解包前先保存原始正文；JSON-RPC 错误仍可从该文件追溯。
        # urlopen 阶段的网络/HTTP 异常会直接向外抛出，可能只留下请求文件。
        (self.directory / (label + ".response.body")).write_bytes(body)
        (self.directory / (label + ".response-meta.json")).write_text(json.dumps(metadata, indent=2), encoding="utf-8")
        return None if notification else decode_rpc(body, self.sequence)


# 命令行入口：参数检查 → 新建采集目录 → 初始化 → 查询工具 → 调用 → 提取文本。
def main():
    parser = argparse.ArgumentParser(description=__doc__)
    # --endpoint 选择服务地址，--tool 选择工具，--arguments 原样传递该工具的参数。
    # --output 是本次采集的独立目录；采集与核对方法统一在上述指南维护。
    parser.add_argument("--endpoint", default="http://127.0.0.1:3667/mcp")
    parser.add_argument("--tool", choices=("fetch_context", "get_node_dsl", "design_to_code", "get_top_level_frames", "eval_script"), required=True)
    parser.add_argument("--arguments", required=True)
    parser.add_argument("--output", type=Path, required=True)
    args = parser.parse_args()
    # --arguments 是工具参数的 JSON 字符串；格式错误时在创建输出目录前停止。
    arguments = json.loads(args.arguments)
    # 每次采集使用新目录，已有目录会报错，防止覆盖之前保存的来源文件。
    args.output.mkdir(parents=True, exist_ok=False)
    recorder = Recorder(args.endpoint, args.output)
    # 01/02 完成 MCP 初始化，03 保存服务当次公开的工具名称及参数结构。
    recorder.rpc("01-initialize", "initialize", {"protocolVersion": "2024-11-05", "capabilities": {},
                 "clientInfo": {"name": "raw-dsl-recorder", "version": "1"}})
    recorder.rpc("02-initialized", "notifications/initialized", {}, notification=True)
    tools = recorder.rpc("03-tools", "tools/list", {})
    # 本地只核对工具名称；参数字段是否符合 inputSchema 由目标服务处理。
    if not any(t["name"] == args.tool for t in tools["tools"]):
        raise ValueError("Requested tool unavailable")
    # 04 才是目标工具调用。eval_script 会执行传入脚本，其读写行为取决于参数。
    result = recorder.rpc("04-call", "tools/call", {"name": args.tool, "arguments": arguments})
    # 工具级失败与 HTTP 成功可以同时出现；在提取前终止，原始响应已由记录器保存。
    if result.get("isError"):
        raise ValueError("MCP tool error; original response retained")
    blocks = result.get("content", [])
    # 按服务返回顺序处理内容块，用原始索引命名文件，使多块响应可以逐项追溯。
    for index, block in enumerate(blocks):
        # 仅提取文本块；图片等非文本内容仍保留在响应正文中，不在这里导出图片。
        if block["type"] == "text":
            text = block["text"]
            # 解析 JSON 只用于确定扩展名和终端摘要，写文件时仍使用原始 text。
            try:
                value = json.loads(text)
                suffix = ".json"
                summary = list(value)[:20] if isinstance(value, dict) else type(value).__name__
            except json.JSONDecodeError:
                suffix, summary = ".txt", text[:300]
            # 单文本块 DSL 使用固定输入名；其他工具或多块返回按内容块索引命名。
            # 例如第二个内容块为文本时使用 content-01；索引也计入前面的非文本块。
            name = "canonical.raw" if args.tool == "get_node_dsl" and len(blocks) == 1 else f"content-{index:02d}"
            # 按 UTF-8 逐字保存已解包文本，保留空白，不重排或补写设计字段。
            (args.output / (name + suffix)).write_bytes(text.encode("utf-8"))
            print(json.dumps({"file": name+suffix, "bytes": len(text.encode('utf-8')), "summary": summary}, ensure_ascii=False))
            # 脚本错误可能封装在文本 JSON 内；先保留返回内容，再终止本次执行。
            if args.tool == "eval_script" and suffix == ".json" and isinstance(value, dict) and value.get("error"):
                raise RuntimeError("Pixso script failed; original response retained: " + str(value["error"]))
    print(str(args.output.resolve()))


# 直接运行脚本才发起采集；verify_capture 导入 decode_rpc 时不会连接服务。
if __name__ == "__main__":
    main()
