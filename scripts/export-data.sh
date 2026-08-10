#!/usr/bin/env bash
# 导出 nano-mimiclaw 数据目录到 data/export-<时间戳>/，方便迁移到其他机器。
# 用法：MIMI_DATA_DIR=<数据目录> ./scripts/export-data.sh
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
DATA_DIR="${MIMI_DATA_DIR:-$HOME/.mimiclaw}"

if [ ! -d "$DATA_DIR" ]; then
    echo "数据目录不存在: $DATA_DIR"
    exit 1
fi

STAMP="$(date +%Y%m%d-%H%M%S)"
OUT="$ROOT/data/export-$STAMP"
mkdir -p "$OUT/sessions"

if [ -f "$DATA_DIR/MEMORY.md" ]; then
    cp "$DATA_DIR/MEMORY.md" "$OUT/"
fi
if [ -d "$DATA_DIR/sessions" ]; then
    cp "$DATA_DIR"/sessions/*.jsonl "$OUT/sessions/" 2>/dev/null || true
fi

python3 - "$OUT" "$DATA_DIR" "$(date -Is)" > "$OUT/manifest.json" <<'PY'
import json
import os
import sys

out, src, ts = sys.argv[1], sys.argv[2], sys.argv[3]
files = []
for root, _, names in os.walk(src):
    for name in names:
        path = os.path.join(root, name)
        files.append({
            "path": os.path.relpath(path, src),
            "bytes": os.path.getsize(path),
        })
print(json.dumps({
    "project": "mimiclaw-from-scratch",
    "exported_at": ts,
    "source_dir": src,
    "files": files,
}, ensure_ascii=False, indent=2))
PY

echo "已导出到 $OUT"
echo "提交前请检查内容（对话历史、个人记忆可能涉及隐私）："
echo "  git add data/export-$STAMP"
echo "  git commit -m \"data: 迁移数据快照\""
echo "新环境恢复："
echo "  ./scripts/restore-data.sh data/export-$STAMP"
