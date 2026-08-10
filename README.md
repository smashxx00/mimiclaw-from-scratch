# nano-mimiclaw from Scratch

从零手写一个跑在 $5 芯片上的 AI agent。

项目沿着 [MimiClaw](https://github.com/memovai/mimiclaw) 的数据流拆解，需要什么、我们造什么，所有组件都是符合直觉的。

删除 MimiClaw 的工程细节（WiFi、Telegram 长轮询、Feishu、OTA、cron、skills、GPIO），留下核心思想：消息总线、agent loop、LLM 代理、工具注册表、记忆与会话。

放轻松，这是一篇文章，不是一本书，你会很容易看懂。

网站把文章和源码放在一起。阅读推进时，右侧编辑器会逐步补全代码，当你看完的时候，nano-mimiclaw 的代码也会全部呈现在编辑器中。

同时设计了一个 Trace 跟踪，可以打断点逐行过代码，trace 数据由真实运行生成，跑的是同一个二进制。

## 运行 nano-mimiclaw

需要 gcc 和 libcurl 开发库。

```bash
make
export MIMI_API_KEY=your-api-key
./nano-mimiclaw
```

可选环境变量：

- `MIMI_MODEL`：模型名，默认 `claude-sonnet-4-5`
- `MIMI_LLM_URL`：Anthropic Messages API 地址，默认 `https://api.anthropic.com/v1/messages`
- `MIMI_SEARCH_KEY`：Brave Search API key，`web_search` 工具用
- `MIMI_DATA_DIR`：数据目录，默认 `~/.mimiclaw`
- `MIMI_TRACE`：非空时把 trace 事件写入该文件

没有 API key 也能玩，仓库自带本地 mock LLM：

```bash
python3 test/mock_llm.py 18099 &
MIMI_API_KEY=test MIMI_LLM_URL=http://127.0.0.1:18099 ./nano-mimiclaw
```

## 本地运行教学网站

```bash
cd web
npm install
npm run dev
```

## 测试

```bash
make test
cd web && npm test
```

## 生成 trace

```bash
make trace
```

## Thanks

- [MimiClaw](https://github.com/memovai/mimiclaw)
- [pi-from-scratch](https://github.com/SaladDay/pi-from-scratch)
- LINUX DO 社区

## License

[MIT](LICENSE)
