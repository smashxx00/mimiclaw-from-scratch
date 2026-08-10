/* 单元测试：bus / memory / tools，不依赖网络。 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#include "bus.h"
#include "memory.h"
#include "tools.h"

static int failures = 0;

#define CHECK(cond, msg)                                                       \
    do {                                                                       \
        if (!(cond)) {                                                         \
            fprintf(stderr, "FAIL %s:%d: %s\n", __FILE__, __LINE__, msg);      \
            failures++;                                                        \
        }                                                                      \
    } while (0)

static void test_bus(void) {
    message_bus_t bus;
    bus_init(&bus);
    CHECK(bus.count == 0, "bus 初始为空");

    /* push 后所有权转移 */
    CHECK(bus_push(&bus, "cli", "local", strdup("hello")) == 1, "push 成功");
    CHECK(bus_push(&bus, "cli", "local", strdup("world")) == 1, "push 成功");
    CHECK(bus.count == 2, "bus 计数");

    mimi_msg_t msg;
    CHECK(bus_pop(&bus, &msg) == 1, "pop 成功");
    CHECK(strcmp(msg.content, "hello") == 0, "pop 内容");
    CHECK(strcmp(msg.channel, "cli") == 0, "pop channel");
    bus_msg_free(&msg);

    CHECK(bus_pop(&bus, &msg) == 1, "pop 成功");
    CHECK(strcmp(msg.content, "world") == 0, "pop 内容");
    bus_msg_free(&msg);

    /* 环形复用：放满 */
    for (int i = 0; i < MIMI_BUS_QUEUE_LEN; i++) {
        CHECK(bus_push(&bus, "cli", "local", strdup("x")) == 1, "push 到满");
    }
    CHECK(bus_push(&bus, "cli", "local", strdup("y")) == 0, "满时 push 失败");
    CHECK(bus_pop(&bus, &msg) == 1, "pop 成功");
    bus_msg_free(&msg);
    CHECK(bus_push(&bus, "cli", "local", strdup("z")) == 1, "腾出位置后可 push");
}

static void test_memory(void) {
    char dir[256];
    snprintf(dir, sizeof(dir), "/tmp/mimiclaw-unit-%d", (int)getpid());
    mkdir(dir, 0755);

    /* MEMORY.md 读写 */
    char *m = memory_load(dir);
    CHECK(strcmp(m, "") == 0, "无记忆文件时返回空串");
    free(m);
    char err[128] = "";
    CHECK(memory_save(dir, "喜欢蓝色", err, sizeof(err)) == 0, "memory_save 成功");
    m = memory_load(dir);
    CHECK(strcmp(m, "喜欢蓝色") == 0, "memory_load 读回");
    free(m);

    /* session 追加与加载 */
    CHECK(session_append(dir, "local", "user", "你好") == 0, "session_append 成功");
    CHECK(session_append(dir, "local", "assistant", "你好呀") == 0, "session_append 成功");
    CHECK(session_append(dir, "local", "user", "再见") == 0, "session_append 成功");

    cJSON *msgs = session_load(dir, "local", 20);
    CHECK(cJSON_GetArraySize(msgs) == 3, "session 加载 3 条");
    cJSON *first = cJSON_GetArrayItem(msgs, 0);
    CHECK(strcmp(cJSON_GetStringValue(cJSON_GetObjectItemCaseSensitive(first, "role")),
                 "user") == 0, "第一条是 user");
    cJSON_Delete(msgs);

    /* 环状保留最近 N 条 */
    for (int i = 0; i < 30; i++) {
        char buf[16];
        snprintf(buf, sizeof(buf), "msg-%d", i);
        session_append(dir, "local", "user", buf);
    }
    msgs = session_load(dir, "local", 20);
    CHECK(cJSON_GetArraySize(msgs) == 20, "只保留最近 20 条");
    cJSON_Delete(msgs);

    /* 坏行容错 */
    FILE *f;
    char path[512];
    snprintf(path, sizeof(path), "%s/sessions/local.jsonl", dir);
    f = fopen(path, "a");
    fputs("this is not json\n", f);
    fclose(f);
    msgs = session_load(dir, "local", 20);
    CHECK(cJSON_GetArraySize(msgs) == 20, "坏行被跳过，其余保留");
    cJSON_Delete(msgs);
}

static void test_tools(void) {
    /* get_time 返回合法时间串 */
    char *t = tool_get_time("{}", NULL);
    CHECK(t != NULL && strlen(t) >= 19, "get_time 输出长度");
    CHECK(strchr(t, ':') != NULL, "get_time 含冒号");
    free(t);

    /* memory_write 写入 data dir */
    char dir[256];
    snprintf(dir, sizeof(dir), "/tmp/mimiclaw-unit-%d", (int)getpid());
    char *r = tool_memory_write("{\"content\":\"测试记忆\"}", dir);
    CHECK(r && strncmp(r, "memory updated", 14) == 0, "memory_write 返回成功");
    free(r);
    char *m = memory_load(dir);
    CHECK(strcmp(m, "测试记忆") == 0, "memory_write 已落盘");
    free(m);

    /* web_search 无 key 时报错而不是崩 */
    r = tool_web_search("{\"query\":\"hello\"}", "");
    CHECK(r && strstr(r, "not configured") != NULL, "web_search 无 key 报错");
    free(r);

    /* 工具注册表 */
    tool_registry_t reg;
    tools_builtin(&reg, dir, "");
    CHECK(reg.count == 3, "注册 3 个工具");
    CHECK(tool_registry_find(&reg, "get_time") != NULL, "能找到 get_time");
    CHECK(tool_registry_find(&reg, "nope") == NULL, "找不到不存在的工具");
    cJSON *json = tool_registry_to_json(&reg);
    CHECK(cJSON_GetArraySize(json) == 3, "tools JSON 有 3 项");
    cJSON_Delete(json);
}

int main(void) {
    test_bus();
    test_memory();
    test_tools();
    if (failures) {
        fprintf(stderr, "%d 个测试失败\n", failures);
        return 1;
    }
    printf("unit tests: all passed\n");
    return 0;
}
