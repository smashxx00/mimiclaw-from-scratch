#!/usr/bin/env python3
"""mock_llm.py: 本地假 Anthropic Messages API。

用于测试和 trace 生成，行为由输入内容决定：
  - 提到时间 -> 返回 get_time 的 tool_use，下一轮返回最终时间文本
  - 提到记住 -> 返回 memory_write 的 tool_use，下一轮返回确认文本
  - 提到错误 -> 返回 HTTP 500
  - 其他     -> 直接返回纯文本回复

用法: python3 mock_llm.py <port> [request_log]
"""
import json
import sys
from http.server import BaseHTTPRequestHandler, HTTPServer

PORT = int(sys.argv[1]) if len(sys.argv) > 1 else 18080
LOG = sys.argv[2] if len(sys.argv) > 2 else ""


def latest_user_text(messages):
    for msg in reversed(messages):
        content = msg.get("content")
        if isinstance(content, str) and msg.get("role") == "user":
            return content
    return ""


def has_tool_result(messages):
    for msg in messages:
        content = msg.get("content")
        if isinstance(content, list):
            for block in content:
                if isinstance(block, dict) and block.get("type") == "tool_result":
                    return True
    return False


def last_tool_used(messages):
    for msg in reversed(messages):
        content = msg.get("content")
        if isinstance(content, list):
            for block in content:
                if isinstance(block, dict) and block.get("type") == "tool_use":
                    return block.get("name", "")
    return ""


def tool_use(name, args):
    return {
        "type": "message",
        "role": "assistant",
        "content": [
            {"type": "text", "text": "我来处理这个请求。"},
            {"type": "tool_use", "id": "toolu_mock_01", "name": name, "input": args},
        ],
        "stop_reason": "tool_use",
    }


def plain(text):
    return {
        "type": "message",
        "role": "assistant",
        "content": [{"type": "text", "text": text}],
        "stop_reason": "end_turn",
    }


def respond(messages):
    text = latest_user_text(messages)
    if has_tool_result(messages):
        tool = last_tool_used(messages)
        if tool == "get_time":
            return plain("现在是 2026-08-10 14:30:00。")
        if tool == "memory_write":
            return plain("记住了：你的喜好已经写入 MEMORY.md。")
        return plain("好的，处理完毕。")
    if "错误" in text:
        return None
    if "几点" in text or "时间" in text:
        return tool_use("get_time", {})
    if "记住" in text:
        return tool_use("memory_write", {"content": "用户喜欢的颜色是蓝色。"})
    return plain("这是一条纯文本回复，没有调用任何工具。")


class Handler(BaseHTTPRequestHandler):
    def do_POST(self):
        length = int(self.headers.get("content-length", 0))
        body = json.loads(self.rfile.read(length).decode("utf-8"))

        if LOG:
            with open(LOG, "a") as f:
                f.write(json.dumps(body, ensure_ascii=False) + "\n")

        messages = body.get("messages", [])
        if not isinstance(messages, list) or not isinstance(body.get("system"), str):
            self.send_json(400, {"type": "error", "error": {"message": "bad request"}})
            return

        resp = respond(messages)
        if resp is None:
            self.send_json(500, {"type": "error", "error": {"message": "mock_500"}})
            return
        self.send_json(200, resp)

    def send_json(self, code, obj):
        payload = json.dumps(obj, ensure_ascii=False).encode("utf-8")
        self.send_response(code)
        self.send_header("content-type", "application/json")
        self.send_header("content-length", str(len(payload)))
        self.end_headers()
        self.wfile.write(payload)

    def log_message(self, *args):
        pass


HTTPServer(("127.0.0.1", PORT), Handler).serve_forever()
