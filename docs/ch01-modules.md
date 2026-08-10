# nano-mimiclaw from Scratch

> "What I cannot create, I do not understand." Richard Feynman 去世时，黑板上留着这句话。

[MimiClaw](https://github.com/memovai/mimiclaw) 是一个跑在 ESP32-S3 上的嵌入式 AI agent。芯片上没有任何操作系统，没有 Node.js，纯 C 加 FreeRTOS，一块 5 美元的小板子，接上 USB 供电，就能在 Telegram 里跟它聊天。它内部是一个完整的 agent：LLM 思考，调用工具，读写记忆，然后把答案发回来。

这个仓库是它的教学版 nano-mimiclaw，一千行上下的 C 代码，在桌面上就能编译运行。我们删除工程细节，留下核心思想：消息总线、agent loop、LLM 代理、工具注册表、记忆与会话。

食用方式跟在 [pi-from-scratch](https://github.com/SaladDay/pi-from-scratch) 一样，跟着数据流走，需要什么就写什么。右侧编辑器会随阅读推进补全代码，读完全文，nano-mimiclaw 的源码就完整了。

> BTW，MimiClaw 还有很多 nano 版没覆盖的东西：WiFi 和 Telegram 长轮询、Feishu 集成、OTA 升级、cron、heartbeat、skills、GPIO 工具。这些词现在都不用管，只是预告。

## 开始之前

nano-mimiclaw 整个项目就六个 C 文件。在跟着数据流造代码之前，先花两分钟记住这六个文件做什么、不做什么、对外暴露什么。

<!-- checkpoint: ch1-bus -->

## bus.c

消息总线。真机上这是两条 FreeRTOS 队列，一条入站一条出站。Telegram 轮询任务把收到的消息推进入站队列，agent 任务从里面取，处理完把回复推进出站队列，再由出站任务按 channel 分发回去。通道和 agent 之间隔着队列，谁也不认识谁，加新通道不用改 agent 的代码。

nano 版没有并发，只有一条总线一个线程，队列退化成环形数组。但语义保持原样：`mimi_msg_t` 里有个 `content`，是堆上的字符串，push 时所有权转移，pop 方负责释放。谁持有指针谁负责，这条规矩让数据跨模块传递不会泄漏。

对外暴露 `bus_init`、`bus_push`、`bus_pop`、`bus_msg_free` 四个函数。它不知道消息内容是什么，不知道 chat_id 是干嘛的，就是个搬运工。

<!-- checkpoint: ch1-llm -->

## llm.c

LLM 代理。吃进去一个 cJSON 消息数组，POST 给 Anthropic Messages API，吐出来一个 `llm_response_t`：content blocks 数组加一个 stop_reason。HTTP 怎么发、JSON 怎么拼、block 怎么拆，全封在这个文件里，上层不用管。

MimiClaw 支持 Anthropic 和 OpenAI 两家，nano 只做 Anthropic。原因不是 OpenAI 不好，是 Anthropic 的协议更贴近 agent 循环的形状：`system` 是顶层字段，`tool_use` 和 `tool_result` 都是 content block，天然就是"模型回复 + 工具结果"来回喂的结构。

> **content block**：消息内容的原子单元。一条 assistant 回复可以是"一段文字加一个工具调用"的组合，每块都是一个独立 JSON 对象。

对外暴露 `llm_chat()` 和几个消息构造辅助函数。它不知道 agent 的存在，也不知道工具怎么执行。

<!-- checkpoint: ch1-tools -->

## tools.c

工具注册表加三个内置工具。每个工具是纯函数：吃原始 JSON 参数串，返回 malloc 的字符串结果。`get_time` 查本地时间，`web_search` 调 Brave Search API，`memory_write` 把一条记忆写进 MEMORY.md。

工具不碰 agent 状态，不知道 Context 是什么，甚至不知道自己是被 agent 调用的。这样设计的好处是工具可以单独测试、单独替换，加一个新工具也不需要改 agent 的任何代码。

对外暴露 `tools_builtin()`（注册全部工具）和 `tool_registry_to_json()`（生成喂给 LLM 的工具描述）。LLM 只看到 name、description、input_schema，看不到 execute 的实现。

<!-- checkpoint: ch1-memory -->

## memory.c

记忆与会话。MEMORY.md 是长期记忆，每次启动读进 system prompt，agent 可以通过 memory_write 工具更新它。sessions 目录下每个 chat_id 一个 JSONL 文件，一行一条消息：`{"role":"user","content":"...","ts":...}`。加载时只保留最近 20 条，写坏的行直接跳过。

它管"数据怎么落盘"，不管"数据该怎么用"。是 agent 在读它、写它，它自己不做任何决定。

<!-- checkpoint: ch1-agent -->

## agent.c

agent 的循环，整个项目的灵魂。调用 `llm_chat()` 问模型，模型说 tool_use 就执行工具，把结果塞回 Context 再问，直到 stop_reason 变成 end_turn。

对外暴露 `agent_run()`。它不碰屏幕，只通过 ui 回调把过程播出去：模型说了什么、调了什么工具、工具返回什么。谁想消费这些事件都可以，终端、网页、trace 文件，随你。

<!-- checkpoint: ch1-main -->

## main.c

胶水。读环境变量造 Model，注册工具，初始化总线和 agent，然后进入一个 REPL：读一行输入推入总线，弹出后交给 agent，把回复打印出来。

MimiClaw 里对应 mimi.c 加 serial_cli.c。真机的 mimi.c 要初始化 NVS、SPIFFS、WiFi、一堆组件，nano 版把这些全砍了，剩一个 while 循环。

<!-- checkpoint: ch1-collab -->

## 它们怎么协作

六个模块拼起来，数据流长这样。

用户输入从 main 出发，推入总线。agent 从总线弹出消息，把 session 历史和当前消息组成 Context，调 llm 问模型。模型回 text 和 tool_use，agent 执行工具，把 tool_result 放回 Context，再问。stop_reason 变成 end_turn 时，agent 把 user 和最终回复写进 session，回复交回 main 打印。

[图：nano-mimiclaw 一轮完整的数据流]

依赖关系是单向的。llm 不知道 agent，tools 不知道 Context，memory 只管落盘，agent 不碰屏幕。main 是唯一认识所有人的那个，它的工作就是把它们粘起来。换掉任何一层，上下游都不用改。

[图：六模块依赖关系]

好了，现在已经知道有哪些模块了，去第二章看它们怎么被一步步写出来。
