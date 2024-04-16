#ifndef CHANNEL_H
#define CHANNEL_H

#include <stdio.h>
#include <stdbool.h>
#include <liburing.h>
#include <arpa/inet.h>
#include "events.h"

#define CHANNEL_E_ALREADY_BUSY 1
#define CHANNEL_E_NOT_EXPECTING_EVENT 2
#define CHANNEL_E_COULD_NOT_GET_SQE 3
#define CHANNEL_E_COULD_NOT_CREATE_SOCKET 4
#define CHANNEL_E_UNKNOWN_EVENT 5

struct channel {
    struct sockaddr_in addr;
    int socket_fd;
    char name[20];
    char state[20];
    double value;
    double rising_threshold;
    void (*on_rising_edge)(struct channel*, double);
    double falling_threshold;
    void (*on_falling_edge)(struct channel*, double);
    char buffer[20];
    bool continous_read; // when true it will immediately schedule a read event after successful read or connection.
    struct event event;
};

int channel_init(struct channel *c, const char *name, bool continous_read);
void channel_init_multiple(struct channel *channels, struct event_context *ctx, int count, int starting_port);

void channel_clear(struct channel *c);

int channel_event(struct event_context *ctx, struct event *e);
int channel_connect_async(struct channel *c, struct io_uring *ring, const char *ip, int port);
int channel_read_async(struct channel *c, struct io_uring *ring);

void channel_watch_rising_edge(struct channel *c, void (*cb)(struct channel*, double), double threshold);
void channel_watch_falling_edge(struct channel *c, void (*cb)(struct channel*, double), double threshold);

void format_channels(FILE *fd, int n, ...);

static inline void register_channel_event_handlers(struct event_context *ctx) {
    event_register_handler(ctx, event_channel_connected, channel_event);
    event_register_handler(ctx, event_channel_read, channel_event);
}

#endif // CHANNEL_H
