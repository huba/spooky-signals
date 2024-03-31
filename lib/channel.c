#include "channel.h"

#include <string.h>
#include <stdio.h>
#include <stdarg.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <arpa/inet.h>

#include <unistd.h>

#include "log.h"

int channel_init(struct channel *c, const char *name, bool continous_read) {
    c->continous_read = continous_read;

    strncpy(c->name, name, sizeof(c->name) - 1);
    c->name[sizeof(c->name) - 1] = 0;

    bzero(c->state, sizeof(c->state));
    bzero(c->buffer, sizeof(c->buffer));
    channel_clear(c);

    c->socket_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (c->socket_fd < 0) CHANNEL_E_COULD_NOT_CREATE_SOCKET;

    c->event.type = event_type_none;
    c->event.data = c;

    return 0;
}

void channel_clear(struct channel *c) {
    strcpy(c->state, "--");
}

int _is_channel_busy(struct channel *c) {
    return c->event.type != event_type_none;
}

int channel_event(struct event *e, struct io_uring *ring) {
    struct channel *c = e->data;

    if (!_is_channel_busy(c)) return CHANNEL_E_NOT_EXPECTING_EVENT;

    switch (c->event.type) {
        case event_channel_read:
            char *nl = strchr(c->buffer, '\n');
            if (nl != NULL) {
                strncpy(c->state, c->buffer, nl - c->buffer);
            }
            log_info("%s reads %s\n", c->name, c->state);
        case event_channel_connected: // FALLTHROUGH
            if (c->continous_read) channel_read_async(c, ring);
            break;
        default:
            return CHANNEL_E_UNKNOWN_EVENT;
    }

    c->event.type = event_type_none;
    return 0;
}

int channel_connect_async(struct channel *c, struct io_uring *ring, const char *host, int port) {
    if (_is_channel_busy(c)) return CHANNEL_E_ALREADY_BUSY;

    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = inet_addr(host);

    struct io_uring_sqe *sqe = io_uring_get_sqe(ring);
    if (!sqe) return CHANNEL_E_COULD_NOT_GET_SQE;

    log_info("%s attempting to connect to %s on port %d\n", c->name, host, port);
    c->event.type = event_channel_connected;

    io_uring_prep_connect(sqe, c->socket_fd, (struct sockaddr *)&addr, sizeof(addr));
    io_uring_sqe_set_data(sqe, &c->event);
    io_uring_submit(ring);

    return 0;
}

int channel_read_async(struct channel *c, struct io_uring *ring) {
    if (_is_channel_busy(c)) return CHANNEL_E_ALREADY_BUSY;

    struct io_uring_sqe *sqe = io_uring_get_sqe(ring);
    if (!sqe) return CHANNEL_E_COULD_NOT_GET_SQE;

    log_info("%s waiting for data\n", c->name);
    c->event.type = event_channel_read;

    io_uring_prep_read(sqe, c->socket_fd, c->buffer, sizeof(c->buffer) - 1, 0);
    io_uring_sqe_set_data(sqe, &c->event);
    io_uring_submit(ring);

    return 0;
}

void format_channels(FILE *fd, int n, ...) {
    struct timeval timer;
    gettimeofday(&timer, NULL);
    unsigned long long millis = 
        ((unsigned long long)timer.tv_sec * 1000) + 
        ((unsigned long long)timer.tv_usec / 1000);

    fprintf(fd, "{\"timestamp\": %llu", millis);

    va_list p;
    va_start(p, n);

    for (int i = 0; i<n; i++) {
        struct channel *c = va_arg(p, struct channel *);
        fprintf(fd, ", \"%s\": \"%s\"", c->name, c->state);
    }

    fprintf(fd, "}\n");
}