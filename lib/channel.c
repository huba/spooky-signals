#include "channel.h"

#include <string.h>
#include <stdio.h>
#include <stdarg.h>
#include <sys/socket.h>
#include <sys/time.h>

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
    if (c->socket_fd < 0) return CHANNEL_E_COULD_NOT_CREATE_SOCKET;

    c->event.type = event_type_none;
    c->event.data = c;

    c->value = 0.0;
    c->rising_threshold = 0.0;
    c->on_rising_edge = NULL;
    c->falling_threshold = 0.0;
    c->on_falling_edge = NULL;

    return 0;
}

void channel_init_multiple(struct channel *channels, struct event_context *ctx, int count, int starting_port) {
    char name[7];

    for (int i=0; i < count; i++) {
        snprintf(name, 7, "out%d", i+1);
        channel_init(&channels[i], name, true);
        channel_connect_async(&channels[i], &ctx->ring, "127.0.0.1", starting_port + i);
    }
}

void channel_clear(struct channel *c) {
    strcpy(c->state, "--");
}

int _is_channel_busy(struct channel *c) {
    return c->event.type != event_type_none;
}

void _update_val(struct channel *c) {
    double new_value;

    int ret = sscanf(c->state, "%lf", &new_value);

    if (ret != 1) return;

    if (new_value >= c->rising_threshold && c->value < c->rising_threshold) {
        if (c->on_rising_edge != NULL) c->on_rising_edge(c, new_value);
    } else if (new_value < c->falling_threshold && c->value >= c->falling_threshold) {
        if (c->on_falling_edge != NULL) c->on_falling_edge(c, new_value);
    }

    c->value = new_value;
}

int channel_event(struct event_context *ctx, struct event *e) {
    struct channel *c = e->data;
    char *nl;

    if (!_is_channel_busy(c)) return CHANNEL_E_NOT_EXPECTING_EVENT;

    switch (c->event.type) {
        case event_channel_read:
            nl = strchr(c->buffer, '\n');
            if (nl != NULL) {
                strncpy(c->state, c->buffer, nl - c->buffer);
            }

            _update_val(c);

            log_info("%s updated to %s\n", c->name, c->state);
            c->event.type = event_type_none;
            if (c->continous_read) return channel_read_async(c, &ctx->ring);
            break;
        case event_channel_connected:
            log_info("%s connected successfully\n", c->name);
            c->event.type = event_type_none;
            if (c->continous_read) return channel_read_async(c, &ctx->ring);
            break;
        default:
            return CHANNEL_E_UNKNOWN_EVENT;
    }

    return 0;
}

int channel_connect_async(struct channel *c, struct io_uring *ring, const char *host, int port) {
    if (_is_channel_busy(c)) return CHANNEL_E_ALREADY_BUSY;

    c->addr.sin_family = AF_INET;
    c->addr.sin_port = htons(port);
    c->addr.sin_addr.s_addr = inet_addr(host);

    struct io_uring_sqe *sqe = io_uring_get_sqe(ring);
    if (!sqe) return CHANNEL_E_COULD_NOT_GET_SQE;

    log_info("%s attempting to connect to %s on port %d\n", c->name, host, port);
    c->event.type = event_channel_connected;

    io_uring_prep_connect(sqe, c->socket_fd, (struct sockaddr *)&c->addr, sizeof(c->addr));
    io_uring_sqe_set_data(sqe, &c->event);

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

    return 0;
}

void channel_watch_rising_edge(struct channel *c, void (*cb)(struct channel*, double), double threshold) {
    c->rising_threshold = threshold;
    c->on_rising_edge = cb;
}

void channel_watch_falling_edge(struct channel *c, void (*cb)(struct channel*, double), double threshold) {
    c->falling_threshold = threshold;
    c->on_falling_edge = cb;
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
    fflush(fd); // ensure it gets flushed even if output is buffered
}