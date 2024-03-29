#ifndef CHANNEL_H
#define CHANNEL_H

#include <stdbool.h>
#include <liburing.h>
#include "events.h"

struct channel {
    int socket_fd;
    char name[20];
    char state[20];
    struct event event;
};

int channel_init(struct channel *c, const char *name);

void channel_clear(struct channel *c);

int channel_connect_sync(struct channel *c, const char *ip, int port);
int channel_read_sync(struct channel *c);

int channel_event(struct channel *c);
int channel_connect_async(struct channel *c, struct io_uring *ring, const char *ip, int port);
int channel_read_async(struct channel *c, struct io_uring *ring);

#endif // CHANNEL_H
