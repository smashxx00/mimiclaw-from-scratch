# nano-mimiclaw 迁移清单

本文件记录把「mimiclaw-from-scratch」项目从当前环境迁移到其他机器或账号时需要带走的东西，按重要性排列，迁移完可以逐项打勾。

## 0. 一句话结论

项目**不使用数据库**。所有持久化数据都是文件：`MEMORY.md`（长期记忆）和 `sessions/*.jsonl`（逐聊天历史），默认在 `~/.mimiclaw`（或 `MIMI_DATA_DIR` 指定的目录）。迁移 = 拷贝代码 + 拷贝数据目录 + 重新配置密钥。

## 1. 代码仓库（已就绪）

- GitHub 仓库：https://github.com/smashxx00/mimiclaw-from-scratch（公开，main 分支）
- 新环境拉取：`git clone https://github.com/smashxx00/mimiclaw-from-scratch.git`
- 后续迁移只需要 `git pull` 保持最新

## 2. 数据（文件持久化，无数据库）

数据目录布局：

```
~/.mimiclaw/
├── MEMORY.md            # 长期记忆，模型通过 memory_write 工具写入
└── sessions/
    └── local.jsonl      # 每个 chat_id 一个 JSONL，一行一条消息
```

推荐用仓库里的脚本迁移：

```bash
# 在旧环境导出
MIMI_DATA_DIR=~/.mimiclaw ./scripts/export-data.sh
# 产物在 data/export-<时间戳>/，含 manifest.json
# 检查内容无敏感信息后提交到仓库：
git add data/export-<时间戳>
git commit -m "data: 迁移数据快照"
git push

# 在新环境恢复
./scripts/restore-data.sh data/export-<时间戳>
```

仓库里已带示例数据 `data/example/`（mock 演示产生），用来查看数据结构和测试恢复流程。真实数据导出前请检查是否包含隐私内容（对话历史、个人记忆等），确认无误再提交。

> 注意：`data/export-*` 没有被 .gitignore 忽略，提交前自行确认内容。

## 3. 密钥与配置（不提交到仓库）

迁移后必须重新配置的环境变量（建议写进新机器的 shell profile 或 .env，不要提交）：

| 变量 | 用途 | 示例 |
| --- | --- | --- |
| `MIMI_API_KEY` | Anthropic API key（必填） | `sk-ant-xxx` |
| `MIMI_MODEL` | 模型名（可选） | `claude-sonnet-4-5` |
| `MIMI_LLM_URL` | API 地址（可选，本地 mock 用） | `http://127.0.0.1:18099` |
| `MIMI_SEARCH_KEY` | Brave Search API key（web_search 用） | `BS-xxx` |
| `MIMI_DATA_DIR` | 数据目录（可选） | `/data/mimiclaw` |

构建期默认值模板在 `src/secrets.h.example`，复制为 `src/secrets.h` 可写死默认值，运行时环境变量优先级更高。

## 4. 本地构建环境（新机器需要装）

- gcc（或 clang）和 make
- libcurl 开发库：Ubuntu/Debian `sudo apt install libcurl4-openssl-dev`，macOS `brew install curl`
- Python 3（跑 mock LLM 与测试脚本）
- Node.js 22+ 与 npm（仅教学网站需要）

验证：

```bash
make test        # C 端：单元测试 + 端到端（mock LLM）
cd web && npm test
make trace       # 重新生成教学网站 trace 数据
```

## 5. 教学网站部署（Vercel）

当前配置（Vercel 项目 `mimiclaw-from-scratch`）：

- Root Directory：`web`
- Framework：Next.js
- Git 集成：连接 GitHub 仓库 `smashxx00/mimiclaw-from-scratch`，push 自动部署
- 线上地址：https://mimiclaw-from-scratch.vercel.app

若迁移到新的 Vercel 账号，新账号 `vercel login` 后：

```bash
cd web
vercel link --yes --project mimiclaw-from-scratch --scope <你的团队>
vercel --prod
```

或在 Vercel Dashboard 用 GitHub 仓库新建项目，并把 Root Directory 设置为 `web`。

## 6. 迁移核对清单

- [ ] 拉取 / 推送最新代码
- [ ] 导出数据 `./scripts/export-data.sh`，检查隐私后提交到仓库
- [ ] 新机器安装 gcc、libcurl、Python、Node.js 22+
- [ ] 配置环境变量（API key 等）
- [ ] `make test` 通过
- [ ] 恢复数据 `./scripts/restore-data.sh data/export-<时间戳>`
- [ ] 确认 MEMORY.md 与会话历史可读
- [ ] （网站）Vercel Root Directory=web、Framework=nextjs、Git 集成正常
- [ ] `cd web && npm test` 通过
