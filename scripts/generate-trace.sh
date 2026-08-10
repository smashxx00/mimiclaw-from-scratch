#!/usr/bin/env bash
# 用真实二进制 + mock LLM 跑出三个教学 case 的 trace，生成 web 用的静态数据。
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
WEB_APP="$ROOT/web/app"
mkdir -p "$WEB_APP"

WORK="$(mktemp -d)"
PORT=18100
python3 "$ROOT/test/mock_llm.py" "$PORT" >/dev/null 2>&1 &
SERVER_PID=$!
trap 'kill $SERVER_PID 2>/dev/null || true; wait $SERVER_PID 2>/dev/null || true; rm -rf "$WORK"' EXIT
sleep 0.5

run_case() {
    local id="$1"
    local input="$2"
    local dir="$WORK/$id"
    mkdir -p "$dir"
    printf '%s\nexit\n' "$input" | \
        MIMI_API_KEY=test \
        MIMI_LLM_URL="http://127.0.0.1:$PORT" \
        MIMI_DATA_DIR="$dir" \
        MIMI_TRACE="$dir/trace.jsonl" \
        "$ROOT/nano-mimiclaw" >/dev/null 2>&1
    { echo "$input"; cat "$dir/trace.jsonl"; } > "$dir/case.txt"
}

run_case plain "回我一句纯文本。"
run_case time "现在几点？"
run_case memory "记住：我喜欢的颜色是蓝色。"

python3 "$ROOT/scripts/trace-to-ts.py" "$WORK" "$WEB_APP/trace-data.generated.ts"
echo "trace 已生成: $WEB_APP/trace-data.generated.ts"
