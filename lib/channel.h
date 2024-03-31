#ifndef CHANNEL_H
#define CHANNEL_H

#include <stdio.h>
#include <stdbool.h>
#include <liburing.h>
#include "events.h"

struct channel {
    int socket_fd;
    char name[20];
    char state[20];
    char buffer[20];
    bool continous_read; // when true it will immediately schedule a read event after successful read or connection.
    struct event event;
};

int channel_init(struct channel *c, const char *name, bool continous_read);

void channel_clear(struct channel *c);

int channel_event(struct event *e, struct io_uring *ring);
int channel_connect_async(struct channel *c, struct io_uring *ring, const char *ip, int port);
int channel_read_async(struct channel *c, struct io_uring *ring);

void format_channels(FILE *fd, int n, ...);

#endif // CHANNEL_H
