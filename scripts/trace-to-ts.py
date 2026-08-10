#!/usr/bin/env python3
"""把跑出来的 trace JSONL 转成 web/app/trace-data.generated.ts。"""
import json
import os
import sys

WORK = sys.argv[1]
OUT = sys.argv[2]

CASES = [
    ("plain", "01", "没有 tool_call", "模型只返回文本，ReAct 循环一轮结束。",
     "一次 LLM 请求，end_turn 收尾。"),
    ("time", "02", "一次工具往返", "模型调用 get_time，结果放回 Context，再回答。",
     "两轮 LLM 请求，一轮工具执行。"),
    ("memory", "03", "写入长期记忆", "模型调用 memory_write，把记忆写进 MEMORY.md。",
     "两轮 LLM 请求，MEMORY.md 落盘。"),
]

LABELS = {
    "input": ("用户消息进入 Context", "input",
              "main 把输入推入消息总线，agent 弹出后追加到 Context（session 历史 + 当前消息）。"),
    "llm_request": ("请求 LLM", "request",
                    "把 Context 和工具定义 POST 给 Anthropic Messages API（非流式）。"),
    "llm_response": ("模型回复", "response",
                     "解析 content blocks：text 与 tool_use。stop_reason 决定下一轮动作。"),
    "llm_error": ("请求失败", "error", "HTTP 或网络错误，agent 把错误交给上层并结束本轮。"),
    "tool_call": ("模型调用工具", "tool", "从注册表找到工具，把参数 JSON 交给 execute。"),
    "tool_result": ("工具返回结果", "tool", "结果作为 tool_result block 放回 Context，进入下一轮。"),
    "turn_end": ("本轮结束", "end", "没有新的 tool_use，agent 把 user + 最终回复写入 session。"),
}


def build_case(case_id, number, title, summary, outcome):
    with open(os.path.join(WORK, case_id, "case.txt"), encoding="utf-8") as f:
        lines = f.read().strip().splitlines()
    prompt = lines[0]
    steps = []
    for idx, raw in enumerate(lines[1:]):
        ev = json.loads(raw)
        label, kind, detail = LABELS.get(
            ev["type"],
            (ev["type"], "request", "trace 事件"),
        )
        if ev["type"] == "llm_request":
            label = "第 %d 次请求 LLM" % int(ev["iteration"])
        elif ev["type"] == "llm_response":
            label = "第 %d 次回复：%s" % (int(ev["iteration"]), ev["stop_reason"])
        elif ev["type"] == "tool_call":
            label = "模型调用 %s" % ev["name"]
        elif ev["type"] == "tool_result":
            label = "%s 返回结果" % ev["name"]
        elif ev["type"] == "turn_end":
            label = "本轮结束（%d 次迭代）" % int(ev["iterations"])

        event = {k: v for k, v in ev.items() if k not in ("src", "file", "line", "type")}
        context = {}
        if "messages" in event:
            context["messages"] = event.pop("messages")

        steps.append({
            "id": "s%d" % (idx + 1),
            "label": label,
            "detail": detail,
            "kind": kind,
            "source": {"file": ev["file"], "line": ev["line"]},
            "event": event,
            "context": context,
        })

    return {
        "id": case_id,
        "number": number,
        "title": title,
        "summary": summary,
        "prompt": prompt,
        "outcome": outcome,
        "steps": steps,
    }


cases = [build_case(*c) for c in CASES]

ts = (
    "// Generated from real runs against test/mock_llm.py. Do not edit by hand.\n"
    "// 重新生成：make trace\n\n"
    'import type { TraceCase } from "./trace-types";\n\n'
    "export const traceCases: TraceCase[] = "
    + json.dumps(cases, ensure_ascii=False, indent=2)
    + ";\n"
)

with open(OUT, "w", encoding="utf-8") as f:
    f.write(ts)
print("written", OUT)
