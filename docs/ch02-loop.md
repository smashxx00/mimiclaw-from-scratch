# 第二章：从一个 while 循环开始

第一章看了地图，知道六个文件各管什么。这一章跟着数据流一步步把它们填起来，需要什么，就写什么。

<!-- checkpoint: ch2-start -->

## agent 到底是什么

一个普通聊天程序，你问一句它答一句。agent 多了一个能力：它可以调用工具。它觉得需要知道当前时间，就发出一个 tool_use，程序帮它执行完，把结果喂回去，它再接着想。反复这个过程，直到它觉得事情做完了，直接回复你。

先不管细节，只把循环的位置摆出来，伪代码不超过十行：

```c
while (迭代次数 < 10) {
    问 LLM（带上 Context 和工具描述）
    if (模型说 tool_use) {
        执行每个工具
        把结果作为 tool_result 放回 Context
        continue
    }
    break  /* end_turn，拿文本收尾 */
}
```

就这些。后面所有代码都是在给这十行补细节。

<!-- checkpoint: ch2-bus -->

## 消息从哪来

agent 不直接读 stdin，也不直接连 Telegram。真机上消息从各种通道进来，通道可能随时加新的，agent 不该认识每一个通道。所以中间隔一条总线：通道往里面推，agent 从里面取，回复再由另一个任务按 channel 分发回去。

nano 版把"任务"退化成普通函数调用，总线保留。消息结构只有三个字段：

```c
typedef struct {
    char channel[16];   /* "cli" | "telegram" | "websocket" */
    char chat_id[32];
    char *content;      /* 堆上文本，所有权随 push 转移 */
} mimi_msg_t;
```

content 是堆指针，push 时所有权转移，pop 方负责 bus_msg_free。为什么强调所有权？真机上两个任务隔着队列交换数据，谁释放、什么时候释放必须有一条明确的规矩，不然就是泄漏或者 double free。

> **所有权**：谁最后持有这个指针，谁负责释放。C 没有 GC，数据跨模块传递时必须说清楚这条。

<!-- checkpoint: ch2-prompt -->

## 先拼系统提示词

agent 手里的第一个问题是：模型怎么知道自己是干嘛的、记得哪些事？MimiClaw 的方案是把人格、记忆、工作方式拼成一个 system prompt，每次请求带上。

`build_system_prompt()` 干的就是这件事：内置的 MIMI_SOUL_DEFAULT 加上 MEMORY.md 的内容，再补一段"工作方式"说明。拼好以后，这一轮要发的东西就是 Context：

```c
system prompt（字符串）
messages（cJSON 数组：session 最近 20 条 + 当前用户消息）
```

> **Context**：这一轮喂给模型的所有东西，纯 JSON，可以直接落盘，下次读回来接着聊。agent 的全部状态就在这里，没有别的隐藏状态。

<!-- checkpoint: ch2-llm -->

## 怎么跟 LLM 说话

Context 有了，下一步把它变成 HTTP 请求。Anthropic Messages API 的请求体长这样：

```json
{
  "model": "claude-sonnet-4-5",
  "max_tokens": 4096,
  "system": "...",
  "tools": [...],
  "messages": [...]
}
```

`llm_chat()` 用 libcurl POST 过去，带三个头：content-type、x-api-key、anthropic-version。响应是完整 JSON，不是流式。MimiClaw 真机也是非流式，芯片上做 SSE 解析不值得，一次性拿完整回复更省事。

响应里最关键的字段是 content 数组和 stop_reason。content 数组的每个元素是一个 block，可能是 text 也可能是 tool_use。llm.c 把它们拆成 block_t 数组：

```c
typedef struct {
    block_kind_t kind;   /* BLOCK_TEXT 或 BLOCK_TOOL_USE */
    char *text;
    char *tool_use_id;
    char *tool_name;
    char *tool_input;    /* 原始 JSON 参数串 */
} block_t;
```

tool_use 的 input 是嵌套 JSON，比如 {"query":"天气"}。llm.c 不解析它，原样保留成字符串，因为解析是工具自己的事。stop_reason 决定 agent 下一步干嘛：tool_use 继续循环，end_turn 收尾。

<!-- checkpoint: ch2-loop -->

## 把循环补完整

骨架有了，llm 能说话了，现在把循环写成真代码。agent_run() 的核心就是前面那段伪代码：

```c
for (; iter < MIMI_AGENT_MAX_TOOL_ITER; iter++) {
    cJSON *tools_json = tool_registry_to_json(env->tools);

    llm_response_t resp;
    if (llm_chat(&env->llm, system_prompt, messages, tools_json,
                 &resp, lerr, sizeof(lerr)) != 0) {
        /* 请求失败：报错，结束本轮 */
    }

    /* 模型回复塞回 Context */
    cJSON_AddItemToArray(messages, msg_blocks("assistant", blocks));

    if (strcmp(resp.stop_reason, "tool_use") == 0) {
        /* 逐个执行工具，结果作为 tool_result 放回 Context */
        cJSON_AddItemToArray(messages, msg_blocks("user", result_blocks));
        continue;
    }

    /* end_turn：拿到最终文本，退出循环 */
    final_text = strdup(resp.text);
    break;
}
```

每次循环都往 messages 里塞一条 assistant 消息，有工具就再塞一条带 tool_result 的 user 消息。Context 就是这么一条一条长起来的，整个对话历史、包括工具调用和结果，全在这个 JSON 数组里。

跑一遍具体例子。用户说"现在几点？"。循环开始前 messages 只有一条用户消息。第一轮模型回 text 加 tool_use(get_time)，assistant 消息入 Context。执行 get_time，返回时间字符串。tool_result 入 Context。第二轮模型看到结果，回"现在是 2026-08-10 14:30:00。"，没有 tool_use。end_turn，收尾。

[图：一轮 Agent Loop 的内部数据流]

<!-- checkpoint: ch2-tools -->

## 工具从哪来

模型说"调用 get_time"，参数是个 JSON 串。agent 拿着名字去注册表里查，找到就调 execute，找不到就造一条错误结果。工具描述数组是注册表生成的，LLM 永远看不到 execute 的实现，它只认识 name、description、input_schema。

工具为什么要设计成纯函数？两个原因。第一，可测试，喂参数返回字符串，没有状态没有副作用依赖。第二，可替换，web_search 想换供应商，改一个文件的事，agent 和 LLM 都不受影响。

三个工具是怎么选的。get_time 不依赖任何外部服务，离线也能跑，是最容易验证 loop 的工具。web_search 是"联网"的代表，agent 最有价值的工具之一，没配 key 时返回错误字符串而不是崩溃，错误也是可以继续读的 tool_result。memory_write 让 agent 真正"记住"东西，把长期记忆写进 MEMORY.md，重启不丢。

工具输出有个隐藏约束：必须是字符串。因为结果要作为 tool_result 塞回 Context 再发给模型，文本是唯一不会出问题的载体。MimiClaw 里 GPIO 工具返回的也是格式化后的文本，不是二进制。

<!-- checkpoint: ch2-memory -->

## 记忆和会话

agent 循环跑完，这一轮的对话得存下来，不然重启就失忆了。nano 版有两个落盘位置。

MEMORY.md。长期记忆，每次启动读进 system prompt，模型通过 memory_write 更新它。这正是 MimiClaw 宣传的"Loyal，记得住事"。

sessions 目录下每个 chat_id 一个 JSONL 文件，一行一条消息，带时间戳。为什么用 JSONL 不用一个大 JSON？追加方便，读坏一行不影响其他行，进程崩溃最多丢半行。加载时只保留最近 20 条，Context 撑不爆，这也对应真机的 MIMI_SESSION_MAX_MSGS。

<!-- checkpoint: ch2-edge -->

## 边界情况

真实 API 不会每次都乖乖 end_turn，nano 版处理三个常见情况。

工具不存在。模型幻觉出一个没注册的名字，tool_registry_find 返回 NULL，agent 构造一条 "error: tool not found" 当 tool_result 放回去。模型看到错误通常就明白了，会换个工具或者直接回答。

请求失败。网络断了或者 API 报 500，llm_chat 返回 -1，agent 把错误交给 main 打印，结束本轮。这里不能重试，否则可能一直报同一个错。

迭代上限。模型陷入 tool_use 死循环，最多跑 10 轮，超了直接结束，防止芯片被一个请求拖死。真机还有 max_tokens 截断的问题，nano 版把截断当 end_turn 处理，文本可能不完整，但至少不会拿半截 JSON 参数去执行工具。

<!-- checkpoint: ch2-main -->

## 粘起来

main() 是唯一认识所有模块的胶水。造 Model、注册工具、初始化总线、进 REPL。每读一行输入，走一遍总线：

```c
char *content = strdup(line);
bus_push(&bus, "cli", "local", content);   /* 入站 */
mimi_msg_t msg;
bus_pop(&bus, &msg);                        /* agent 从出站取 */
agent_run(&env, &msg, &reply, ...);
```

ui 回调是 agent 和界面的协议。agent 调 on_tool_call、on_tool_result，main 负责打印 [tool: ...] 和 [result: ...]。这层解耦是刻意做的：agent 一行代码没变，把回调换成别的消费者，就是另一种"前端"。MimiClaw 真机上，agent 同样不知道 Telegram 和 WebSocket 的存在，它只跟总线说话。

每个 checkpoint 之后的代码段，右侧编辑器已经帮你补进 src/ 了。读到这里，nano-mimiclaw 的源码就齐了。去第三章把它跑起来。
