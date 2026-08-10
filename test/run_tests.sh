#!/usr/bin/env bash
# 端到端测试：启动 mock LLM，喂给 nano-mimiclaw 四条输入，逐条断言输出与落盘。
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
BIN="$ROOT/nano-mimiclaw"
UNITS="$ROOT/test/unit_tests"

fail() {
    echo "FAIL: $1"
    exit 1
}

echo "== 单元测试 =="
"$UNITS"

echo "== 端到端（mock LLM）=="
WORK="$(mktemp -d)"
PORT=18099
LOG="$WORK/requests.jsonl"
python3 "$ROOT/test/mock_llm.py" "$PORT" "$LOG" &
SERVER_PID=$!
trap 'kill $SERVER_PID 2>/dev/null || true; wait $SERVER_PID 2>/dev/null || true; rm -rf "$WORK"' EXIT
sleep 0.5

printf '现在几点？\n帮我记住我喜欢的颜色是蓝色。\n回我一句纯文本。\n触发一次错误。\nexit\n' | \
  MIMI_API_KEY=test \
  MIMI_LLM_URL="http://127.0.0.1:$PORT" \
  MIMI_DATA_DIR="$WORK/data" \
  "$BIN" > "$WORK/output.txt" 2>&1

grep -q '现在是 2026-08-10 14:30:00' "$WORK/output.txt" || fail "时间回复缺失"
grep -q '记住了：你的喜好已经写入 MEMORY.md' "$WORK/output.txt" || fail "记忆回复缺失"
grep -q '纯文本回复' "$WORK/output.txt" || fail "纯文本回复缺失"
grep -q '\[error\]' "$WORK/output.txt" || fail "错误场景没有报错"
grep -q '\[tool: get_time\]' "$WORK/output.txt" || fail "工具调用事件缺失"
grep -q '\[result: get_time\]' "$WORK/output.txt" || fail "工具结果事件缺失"

test -f "$WORK/data/sessions/local.jsonl" || fail "session 文件未生成"
test -f "$WORK/data/MEMORY.md" || fail "MEMORY.md 未生成"
grep -q '蓝色' "$WORK/data/MEMORY.md" || fail "MEMORY.md 内容缺失"

python3 - "$WORK/data/sessions/local.jsonl" <<'PY'
import json, sys
lines = [json.loads(l) for l in open(sys.argv[1]) if l.strip()]
assert len(lines) >= 6, "session 应有 3 轮 x 2 条消息"
for item in lines:
    assert item["role"] in ("user", "assistant") and isinstance(item["content"], str), "session 结构错误"
    assert isinstance(item.get("ts"), (int, float)), "session 缺少 ts"
print("OK: session.jsonl 结构正确")
PY

python3 - "$LOG" <<'PY'
import json, sys
reqs = [json.loads(l) for l in open(sys.argv[1]) if l.strip()]
assert reqs, "没有请求日志"
tool_round = [
    r for r in reqs
    if any(
        isinstance(m.get("content"), list)
        and any(b.get("type") == "tool_result" for b in m["content"])
        for m in r["messages"]
    )
]
assert tool_round, "工具结果没有回传给 LLM"
last = tool_round[0]["messages"][-1]["content"][0]
assert last["type"] == "tool_result" and last["tool_use_id"], "tool_result block 结构错误"
print("OK: tool_result 已回传给 LLM")
PY

echo "== 端到端通过 =="
