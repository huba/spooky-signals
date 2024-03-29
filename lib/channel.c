#include "channel.h"

#include <string.h>
#include <stdio.h>
#include <sys/socket.h>
#include <arpa/inet.h>

#include <unistd.h>

int channel_init(struct channel *c, const char *name) {
    strncpy(c->name, name, sizeof(c->name) - 1);
    c->name[sizeof(c->name) - 1] = 0;

    bzero(c->state, sizeof(c->state));
    channel_clear(c);

    c->socket_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (c->socket_fd < 0) {
        perror("Could not create socket");
        return 1;
    }

    c->event.type = event_type_none;
    c->event.data = c;

    return 0;
}

void channel_clear(struct channel *c) {
    strcpy(c->state, "--");
}

int channel_connect_sync(struct channel *c, const char *host, int port) {
    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = inet_addr(host);

    if (connect(c->socket_fd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        perror("Could not connect to signal source");
        return 1;
    }

    return 0;
}

int channel_read_sync(struct channel *c) {
    static char buffer[1024];
    bzero(buffer, sizeof(buffer));

    int n = read(c->socket_fd, buffer, sizeof(buffer) - 1);
    if (n < 0) {
        perror("Could not read from socket");
        return 1;
    }

    if (n == 0) return 0; // No data received

    if (buffer[n - 1] == '\n') n--; // Remove trailing newline

    buffer[n] = 0;
    strncpy(c->state, buffer, sizeof(c->state) - 1);
    c->state[sizeof(c->state) - 1] = 0;

    return 0;
}

int _is_channel_busy(struct channel *c) {
    return c->event.type != event_type_none;
}

int channel_event(struct channel *c) {
    if (!_is_channel_busy(c)) {
        fprintf(stderr, "Channel is not currently expecting an event\n");
        return 1;
    }

    switch (c->event.type) {
        case event_channel_read:
            char *nl = strchr(c->state, '\n');
            if (nl) *nl = 0; // Remove trailing newline
            break;
        default:
            break;
    }

    c->event.type = event_type_none;
    return 0;
}

int channel_connect_async(struct channel *c, struct io_uring *ring, const char *host, int port) {
    if (_is_channel_busy(c)) {
        fprintf(stderr, "Channel is already busy\n");
        return 1;
    }

    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = inet_addr(host);

    struct io_uring_sqe *sqe = io_uring_get_sqe(ring);
    if (!sqe) {
        fprintf(stderr, "Could not get sqe");
        return 1;
    }

    io_uring_prep_connect(sqe, c->socket_fd, (struct sockaddr *)&addr, sizeof(addr));

    c->event.type = event_channel_connected;

    io_uring_sqe_set_data(sqe, &c->event);
    io_uring_submit(ring);

    return 0;
}

int channel_read_async(struct channel *c, struct io_uring *ring) {
    if (_is_channel_busy(c)) {
        fprintf(stderr, "Channel is already busy\n");
        return 1;
    }

    struct io_uring_sqe *sqe = io_uring_get_sqe(ring);
    if (!sqe) {
        fprintf(stderr, "Could not get sqe");
        return 1;
    }

    io_uring_prep_read(sqe, c->socket_fd, c->state, sizeof(c->state) - 1, 0);

    c->event.type = event_channel_read;

    io_uring_sqe_set_data(sqe, &c->event);
    io_uring_submit(ring);

    return 0;
}