#include "agent.h"

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "config.h"
#include "memory.h"

#define TRACE(env, type, payload) \
    agent_trace_event((env), __FILE__, __LINE__, (type), (payload))

/* ---- trace payload 辅助 ---- */

static cJSON *po(void) { return cJSON_CreateObject(); }

static void pstr(cJSON *o, const char *k, const char *v) {
    cJSON_AddStringToObject(o, k, v ? v : "");
}

static void pnum(cJSON *o, const char *k, double v) {
    cJSON_AddNumberToObject(o, k, v);
}

static void pmsg(cJSON *o, cJSON *messages) {
    cJSON_AddItemToObject(o, "messages", cJSON_Duplicate(messages, 1));
}

void agent_trace_event(agent_env_t *env, const char *src_file, int src_line,
                       const char *type, cJSON *payload) {
    if (!env || !env->trace) {
        cJSON_Delete(payload);
        return;
    }
    cJSON *ev = cJSON_CreateObject();
    char src[256];
    snprintf(src, sizeof(src), "%s:%d", src_file, src_line);
    pstr(ev, "src", src);
    pstr(ev, "file", src_file);
    pnum(ev, "line", src_line);
    pstr(ev, "type", type);

    if (payload) {
        while (payload->child) {
            cJSON *child = payload->child;
            cJSON_DetachItemViaPointer(payload, child);
            cJSON_AddItemToObject(ev, child->string, child);
        }
        cJSON_Delete(payload);
    }

    char *line = cJSON_PrintUnformatted(ev);
    if (line) {
        fprintf(env->trace, "%s\n", line);
        free(line);
    }
    cJSON_Delete(ev);
}

/* ---- system prompt ---- */

static char *build_system_prompt(const char *data_dir) {
    char *memory = memory_load(data_dir);
    size_t len = strlen(MIMI_SOUL_DEFAULT) + strlen(memory) + 128;
    char *prompt = (char *)malloc(len);
    snprintf(prompt, len,
             "%s\n\n# 长期记忆\n%s\n\n# 工作方式\n"
             "用工具完成任务，工具结果会以 tool_result 的形式反馈给你。",
             MIMI_SOUL_DEFAULT, memory);
    free(memory);
    return prompt;
}

/* ---- 主循环 ---- */

int agent_run(agent_env_t *env, const mimi_msg_t *msg, char **reply, char *err, size_t errlen) {
    if (errlen) err[0] = '\0';
    *reply = NULL;

    char *system_prompt = build_system_prompt(env->data_dir);

    /* Context = session 历史 + 当前消息，整个对话就是一个 cJSON 数组 */
    cJSON *messages = session_load(env->data_dir, msg->chat_id, MIMI_SESSION_MAX_MSGS);
    cJSON_AddItemToArray(messages, msg_text("user", msg->content));

    cJSON *tp = po();
    pstr(tp, "content", msg->content);
    pstr(tp, "chat_id", msg->chat_id);
    pmsg(tp, messages);
    TRACE(env, "input", tp);

    char *final_text = NULL;
    int iter = 0;
    for (; iter < MIMI_AGENT_MAX_TOOL_ITER; iter++) {
        cJSON *tools_json = tool_registry_to_json(env->tools);

        tp = po();
        pnum(tp, "iteration", iter + 1);
        pnum(tp, "message_count", cJSON_GetArraySize(messages));
        TRACE(env, "llm_request", tp);

        llm_response_t resp;
        char lerr[512];
        if (llm_chat(&env->llm, system_prompt, messages, tools_json,
                     &resp, lerr, sizeof(lerr)) != 0) {
            cJSON_Delete(tools_json);
            snprintf(err, errlen, "%s", lerr);
            tp = po();
            pstr(tp, "error", lerr);
            TRACE(env, "llm_error", tp);
            free(system_prompt);
            cJSON_Delete(messages);
            return -1;
        }
        cJSON_Delete(tools_json);

        /* 模型回复塞回 Context：assistant message，text + tool_use block 平铺 */
        cJSON *blocks = cJSON_CreateArray();
        for (int i = 0; i < resp.block_count; i++) {
            block_t *b = &resp.blocks[i];
            if (b->kind == BLOCK_TEXT) {
                cJSON_AddItemToArray(blocks, block_text(b->text));
            } else {
                cJSON *input = cJSON_Parse(b->tool_input);
                if (!input) input = cJSON_CreateObject();
                cJSON_AddItemToArray(blocks,
                    block_tool_use(b->tool_use_id, b->tool_name, input));
            }
        }
        cJSON_AddItemToArray(messages, msg_blocks("assistant", blocks));

        tp = po();
        pnum(tp, "iteration", iter + 1);
        pstr(tp, "stop_reason", resp.stop_reason);
        pstr(tp, "text", resp.text);
        TRACE(env, "llm_response", tp);

        if (env->ui.on_assistant_text && resp.text && *resp.text) {
            env->ui.on_assistant_text(resp.text, env->ui.userdata);
        }

        if (strcmp(resp.stop_reason, "tool_use") == 0) {
            /* 执行工具，结果作为 tool_result 放回 Context，然后继续循环 */
            cJSON *result_blocks = cJSON_CreateArray();
            int executed = 0;
            for (int i = 0; i < resp.block_count && executed < MIMI_MAX_TOOL_CALLS; i++) {
                block_t *b = &resp.blocks[i];
                if (b->kind != BLOCK_TOOL_USE) continue;
                executed++;

                const tool_t *tool = tool_registry_find(env->tools, b->tool_name);
                char *result = NULL;
                if (!tool) {
                    size_t len = strlen(b->tool_name) + 32;
                    result = (char *)malloc(len);
                    snprintf(result, len, "error: tool \"%s\" not found", b->tool_name);
                } else {
                    tp = po();
                    pstr(tp, "name", b->tool_name);
                    pstr(tp, "args", b->tool_input);
                    TRACE(env, "tool_call", tp);

                    if (env->ui.on_tool_call) {
                        env->ui.on_tool_call(b->tool_name, b->tool_input, env->ui.userdata);
                    }
                    result = tool->execute(b->tool_input, tool->userdata);
                    if (!result) result = strdup("error: tool execution failed");

                    tp = po();
                    pstr(tp, "name", b->tool_name);
                    pstr(tp, "result", result);
                    TRACE(env, "tool_result", tp);

                    if (env->ui.on_tool_result) {
                        env->ui.on_tool_result(b->tool_name, result, env->ui.userdata);
                    }
                }
                cJSON_AddItemToArray(result_blocks,
                    block_tool_result(b->tool_use_id, result));
                free(result);
            }

            if (executed == 0) {
                /* 畸形回复：说 tool_use 却一个工具都没给，直接收尾 */
                cJSON_Delete(result_blocks);
                final_text = strdup("error: model requested tool_use without tool calls");
                llm_response_free(&resp);
                break;
            }

            cJSON_AddItemToArray(messages, msg_blocks("user", result_blocks));
            llm_response_free(&resp);
            continue;
        }

        /* end_turn / max_tokens：拿到最终文本，退出循环 */
        final_text = strdup(resp.text && *resp.text ? resp.text : "(模型没有输出文本)");
        llm_response_free(&resp);
        break;
    }

    if (!final_text) {
        final_text = strdup("error: tool_use iteration limit exceeded");
    }

    /* session 持久化：存用户消息 + 最终回复（与 MimiClaw 一致） */
    session_append(env->data_dir, msg->chat_id, "user", msg->content);
    session_append(env->data_dir, msg->chat_id, "assistant", final_text);

    tp = po();
    pstr(tp, "reply", final_text);
    pnum(tp, "iterations", iter + 1);
    pmsg(tp, messages);
    TRACE(env, "turn_end", tp);

    *reply = final_text;
    free(system_prompt);
    cJSON_Delete(messages);
    return 0;
}
