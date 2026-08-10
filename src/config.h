#pragma once

/* nano-mimiclaw 编译期常量。
 * 对应 MimiClaw 的 mimi_config.h，砍掉了 OTA / cron / skills / 多通道等工程细节。 */

#define MIMI_AGENT_MAX_TOOL_ITER  10     /* ReAct 循环迭代上限 */
#define MIMI_MAX_TOOL_CALLS       4      /* 一轮最多执行的工具数 */
#define MIMI_SESSION_MAX_MSGS     20     /* session 保留最近消息数 */

#define MIMI_LLM_MAX_TOKENS       4096
#define MIMI_LLM_DEFAULT_MODEL    "claude-sonnet-4-5"
#define MIMI_LLM_API_URL          "https://api.anthropic.com/v1/messages"
#define MIMI_LLM_API_VERSION      "2023-06-01"

#define MIMI_BUS_QUEUE_LEN        8      /* 入站/出站队列容量 */
#define MIMI_MSG_MAX_LEN          4096
#define MIMI_RESPONSE_MAX_LEN     (1 * 1024 * 1024)  /* HTTP 响应体上限 */

/* 默认人格，对应 MimiClaw 的 SOUL.md。
 * 真机从 SPIFFS 读文件，nano 版内置常量，可被 data 目录下的 SOUL.md 覆盖。 */
#define MIMI_SOUL_DEFAULT \
  "你是 MimiClaw，一个跑在 $5 芯片上的 AI 助手。\n" \
  "你能通过工具感知世界：查询时间、搜索网络、写入长期记忆。\n" \
  "能用工具就先调用工具，拿到结果后再简洁地回答。"
