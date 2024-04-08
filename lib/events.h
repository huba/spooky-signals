#ifndef EVENTS_H
#define EVENTS_H

#include <liburing.h>
#include <stdbool.h>

enum event_type {
    event_type_none = 0,
    event_channel_connected,
    event_channel_read,
    event_timeout,

    event_type_count,
    event_type_max = event_timeout
};

struct event {
    enum event_type type;
    void *data;
};

struct event_context;
typedef int (*event_handler_function)(struct event_context*, struct event*);

struct event_context {
    bool running;
    event_handler_function event_handlers[event_type_count];
    struct io_uring ring;
    struct __kernel_timespec loop_delay;
};

int event_init(struct event_context *ctx);
void event_set_timeout(struct event_context *ctx, long long timeout_ms, event_handler_function loop_handler);
void event_register_handler(struct event_context *ctx, enum event_type e, event_handler_function handler);
int event_run_loop(struct event_context *ctx);

#endif // EVENTS_H