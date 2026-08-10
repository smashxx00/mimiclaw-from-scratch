#include "bus.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void bus_init(message_bus_t *bus) {
    memset(bus, 0, sizeof(*bus));
}

/* 队列满返回 0。content 所有权转移给队列，失败时调用方仍需自行释放。 */
int bus_push(message_bus_t *bus, const char *channel, const char *chat_id, char *content) {
    if (bus->count >= MIMI_BUS_QUEUE_LEN) return 0;

    mimi_msg_t *slot = &bus->items[bus->tail];
    memset(slot, 0, sizeof(*slot));
    snprintf(slot->channel, sizeof(slot->channel), "%s", channel);
    snprintf(slot->chat_id, sizeof(slot->chat_id), "%s", chat_id);
    slot->content = content;

    bus->tail = (bus->tail + 1) % MIMI_BUS_QUEUE_LEN;
    bus->count++;
    return 1;
}

int bus_pop(message_bus_t *bus, mimi_msg_t *out) {
    if (bus->count == 0) return 0;

    *out = bus->items[bus->head];
    memset(&bus->items[bus->head], 0, sizeof(bus->items[bus->head]));
    bus->head = (bus->head + 1) % MIMI_BUS_QUEUE_LEN;
    bus->count--;
    return 1;
}

void bus_msg_free(mimi_msg_t *msg) {
    free(msg->content);
    msg->content = NULL;
}
