#pragma once

#include <stddef.h>

#include "cJSON.h"

/* 工具注册表。对应 MimiClaw 的 tools/tool_registry.c。
 * 每个工具是纯函数：吃原始 JSON 参数串，吐 malloc 的字符串结果。
 * 工具不知道 agent 的存在，不知道 Context 是什么，可以被单独测试、单独替换。 */

typedef char *(*tool_exec_fn)(const char *args_json, void *userdata);

typedef struct {
    const char *name;
    const char *description;
    const char *parameters_json;  /* JSON Schema 字符串 */
    tool_exec_fn execute;
    void *userdata;
} tool_t;

#define TOOL_REGISTRY_MAX 8

typedef struct {
    tool_t items[TOOL_REGISTRY_MAX];
    int count;
} tool_registry_t;

void tool_registry_init(tool_registry_t *reg);
int  tool_registry_add(tool_registry_t *reg, const char *name, const char *description,
                       const char *parameters_json, tool_exec_fn execute, void *userdata);
const tool_t *tool_registry_find(const tool_registry_t *reg, const char *name);

/* 构建 Anthropic tools 数组（name/description/input_schema），调用方负责 cJSON_Delete */
cJSON *tool_registry_to_json(const tool_registry_t *reg);

/* 注册三个内置工具 */
void tools_builtin(tool_registry_t *reg, const char *data_dir, const char *search_key);

/* 工具实现，暴露给单元测试 */
char *tool_get_time(const char *args_json, void *userdata);
char *tool_web_search(const char *args_json, void *userdata);
char *tool_memory_write(const char *args_json, void *userdata);
