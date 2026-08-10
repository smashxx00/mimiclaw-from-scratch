#pragma once

#include <stdio.h>

#include "bus.h"
#include "llm.h"
#include "tools.h"

/* Agent Loop。对应 MimiClaw 的 agent/agent_loop.c。
 *
 * 核心是一个循环：问 LLM → 模型说要调工具就调 → 结果塞回 Context → 再问，
 * 直到 stop_reason 不是 tool_use。UI 与 agent 解耦：
 * agent 只通过 ui 回调把过程播出去，main 负责消费（对应 nanopi 的 AgentEvent/TUI）。 */

typedef struct {
    void (*on_assistant_text)(const char *text, void *userdata);
    void (*on_tool_call)(const char *name, const char *args_json, void *userdata);
    void (*on_tool_result)(const char *name, const char *result, void *userdata);
    void *userdata;
} agent_ui_t;

typedef struct {
    llm_config_t llm;
    tool_registry_t *tools;
    const char *data_dir;
    FILE *trace;        /* NULL = 关闭 trace 输出 */
    agent_ui_t ui;
} agent_env_t;

/* 处理一条入站消息，回复写入 *reply（malloc，调用方 free）。
 * 返回 0 成功，-1 失败（错误写进 err）。 */
int agent_run(agent_env_t *env, const mimi_msg_t *msg, char **reply, char *err, size_t errlen);

/* trace 发射：把事件写进 env->trace（JSONL）。payload 所有权转移给本函数。 */
void agent_trace_event(agent_env_t *env, const char *src_file, int src_line,
                       const char *type, cJSON *payload);
