# 数据目录

nano-mimiclaw 不使用数据库，持久化数据是文件，运行时放在数据目录（默认 `~/.mimiclaw`，或用 `MIMI_DATA_DIR` 指定）：

```
<数据目录>/
├── MEMORY.md            # 长期记忆
└── sessions/*.jsonl     # 每个 chat_id 一个对话历史文件
```

本目录内容：

- `example/`：mock 演示产生的示例数据，用于查看数据结构和测试恢复流程
- `export-*/`：用 `scripts/export-data.sh` 导出的迁移快照，提交前请检查隐私内容

迁移说明见 `../docs/MIGRATION.md`。
