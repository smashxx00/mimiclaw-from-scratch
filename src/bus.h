#pragma once

#include "config.h"

/* 消息总线。对应 MimiClaw 的 bus/message_bus.c（FreeRTOS 双队列）。
 *
 * nano 版是单线程环形队列，语义与真机保持一致：
 * content 是堆上字符串，push 时所有权随消息转移，pop 方负责 bus_msg_free()。
 * 真机上入站/出站是两条 xQueue，由不同任务生产消费；
 * nano 版只有一条总线、一个线程，因为桌面版不需要并发。
 */

typedef struct {
    char channel[16];   /* "cli" | "telegram" | "websocket" */
    char chat_id[32];   /* "local" 或聊天 ID */
    char *content;      /* 堆上文本，所有权随 push 转移 */
} mimi_msg_t;

typedef struct {
    mimi_msg_t items[MIMI_BUS_QUEUE_LEN];
    int head;
    int tail;
    int count;
} message_bus_t;

void bus_init(message_bus_t *bus);
int  bus_push(message_bus_t *bus, const char *channel, const char *chat_id, char *content);
int  bus_pop(message_bus_t *bus, mimi_msg_t *out);
void bus_msg_free(mimi_msg_t *msg);
