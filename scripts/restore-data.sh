#!/usr/bin/env bash
# 从 data/export-<时间戳>/ 或 data/example/ 恢复数据到数据目录。
# 用法：./scripts/restore-data.sh <导出目录>
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
SRC="${1:?用法: restore-data.sh <导出目录>}"
DATA_DIR="${MIMI_DATA_DIR:-$HOME/.mimiclaw}"

if [ ! -f "$SRC/MEMORY.md" ] && [ ! -d "$SRC/sessions" ]; then
    echo "目录里没有可恢复的数据: $SRC"
    exit 1
fi

mkdir -p "$DATA_DIR/sessions"
if [ -f "$SRC/MEMORY.md" ]; then
    cp "$SRC/MEMORY.md" "$DATA_DIR/"
fi
if [ -d "$SRC/sessions" ]; then
    cp "$SRC"/sessions/*.jsonl "$DATA_DIR/sessions/" 2>/dev/null || true
fi

echo "已恢复: $SRC -> $DATA_DIR"
