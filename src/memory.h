#pragma once

#include <stddef.h>

#include "cJSON.h"

/* 记忆与会话。对应 MimiClaw 的 memory/memory_store.c + memory/session_mgr.c。
 * MEMORY.md 是长期记忆，读进 system prompt；
 * sessions/<chat_id>.jsonl 是逐聊天的对话历史，每行一条 JSON 消息。 */

char *memory_load(const char *data_dir);      /* malloc，文件不存在返回空串 */
int   memory_save(const char *data_dir, const char *content, char *err, size_t errlen);

/* 加载最近 max_msgs 条消息为 cJSON 数组（role/content 都是字符串），调用方负责删除 */
cJSON *session_load(const char *data_dir, const char *chat_id, int max_msgs);
int    session_append(const char *data_dir, const char *chat_id,
                      const char *role, const char *content);
