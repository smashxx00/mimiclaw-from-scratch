#pragma once

#include <stddef.h>

#include "cJSON.h"

/* LLM 代理。对应 MimiClaw 的 llm/llm_proxy.c。
 * 只做一件事：把 messages 数组 POST 给 Anthropic Messages API（非流式），
 * 把响应解析成 content block 数组 + stop_reason。上层不碰 HTTP 细节。 */

typedef struct {
    const char *api_key;
    const char *model;
    const char *base_url;   /* 默认 MIMI_LLM_API_URL，可用 MIMI_LLM_URL 覆盖 */
    int max_tokens;
} llm_config_t;

typedef enum { BLOCK_TEXT, BLOCK_TOOL_USE } block_kind_t;

typedef struct {
    block_kind_t kind;
    char *text;          /* BLOCK_TEXT 的文本 */
    char *tool_use_id;   /* BLOCK_TOOL_USE */
    char *tool_name;
    char *tool_input;    /* 原始 JSON 参数串 */
} block_t;

typedef struct {
    char *text;          /* 所有 text block 拼接，便于直接当回复用 */
    block_t *blocks;
    int block_count;
    char stop_reason[16];  /* "end_turn" | "tool_use" | "max_tokens" | "" */
} llm_response_t;

/* 调用一次 Messages API。messages/tools 只读，不修改。
 * 返回 0 成功；-1 失败并把错误写进 err。 */
int llm_chat(const llm_config_t *cfg, const char *system, cJSON *messages,
             cJSON *tools, llm_response_t *out, char *err, size_t errlen);
void llm_response_free(llm_response_t *resp);

/* 消息构造辅助：返回新分配的 cJSON，所有权归调用方。 */
cJSON *msg_text(const char *role, const char *text);
cJSON *msg_blocks(const char *role, cJSON *blocks);
cJSON *block_text(const char *text);
cJSON *block_tool_use(const char *id, const char *name, cJSON *input);
cJSON *block_tool_result(const char *tool_use_id, const char *content);
