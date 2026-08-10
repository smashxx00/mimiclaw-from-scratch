# 第三章：跑起来看看

前面两章把代码讲完了，这一章实际跑一遍，看数据怎么流。环境需要 gcc 和 libcurl 开发库，Linux 上 `sudo apt install libcurl4-openssl-dev`，macOS 上 `brew install curl`。

<!-- checkpoint: ch3-all -->

## 构建

```bash
make
```

产出 `nano-mimiclaw` 一个可执行文件。纯 C，没有其他运行时依赖。

## 不花钱的玩法：本地 mock

没有 Anthropic API key 也能跑。仓库自带一个 mock LLM 服务器，行为是写死的：问时间就返回 get_time 的 tool_use，说"记住"就返回 memory_write 的 tool_use，说"错误"就返回 500。

```bash
python3 test/mock_llm.py 18099 &

printf '现在几点？\n记住我喜欢的颜色是蓝色。\nexit\n' | \
  MIMI_API_KEY=test \
  MIMI_LLM_URL=http://127.0.0.1:18099 \
  MIMI_DATA_DIR=/tmp/mimiclaw-demo \
  ./nano-mimiclaw
```

终端输出大致长这样：

```
mimi> 我来处理这个请求。
[tool: get_time] {}
[result: get_time] 2026-08-10 14:30:00 CST
现在是 2026-08-10 14:30:00。
```

`[tool: ...]` 和 `[result: ...]` 就是 ui 回调打印的事件，agent 本身不知道终端的存在。

## 真实 API

有 Anthropic key 的话，把 mock 换成真服务：

```bash
MIMI_API_KEY=sk-ant-xxxx \
MIMI_MODEL=claude-sonnet-4-5 \
MIMI_SEARCH_KEY=brave-key \
./nano-mimiclaw
```

环境变量覆盖构建期默认值，这就是 nano 版的"两层配置"：build-time 默认值在 config.h，运行时用环境变量覆盖，对应真机 NVS 覆盖 mimi_secrets.h 的玩法。

## 看落盘

跑完看一眼 `/tmp/mimiclaw-demo`：

```bash
cat /tmp/mimiclaw-demo/MEMORY.md
cat /tmp/mimiclaw-demo/sessions/local.jsonl
```

MEMORY.md 里是模型通过 memory_write 写入的长期记忆。session 文件每行一条消息，role、content、ts 三件套，这就是下一轮启动时的 Context 来源。

## 生成 trace

教学网站的断点调试数据由真实运行生成：

```bash
make trace
```

脚本用 mock LLM 跑三个典型 case，把 trace 事件转换成 `web/app/trace-data.generated.ts`。打开教学网站，切到 TraceLab，就能逐行打断点看代码执行流。

## 测试

```bash
make test
```

单元测试覆盖总线、记忆、工具，端到端测试起 mock 服务器喂四条输入，断言回复、session 结构、tool_result 回传。想加新工具或者改 loop，跑一遍测试就知道有没有打破约定。
