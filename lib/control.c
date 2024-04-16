#include "control.h"

#include <stdlib.h>
#include <strings.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>

#include "log.h"

int control_interface_init(struct control_interface *interface, const char *address, int port) {
    interface->buffer = NULL;
    interface->buffer_length = 0;

    bzero(&interface->address, sizeof(interface->address));
    interface->address.sin_family = AF_INET;
    interface->address.sin_port = htons(port);
    interface->address.sin_addr.s_addr = inet_addr(address);

    interface->fd = socket(AF_INET, SOCK_DGRAM, 0);
    if (interface->fd < 0) {
        log_error("Failed to create socket for control interface.");
        return 1;
    }

    return 0;
}

struct _dynamic_event {
    struct event e;
    struct msghdr msg;
    struct iovec iov;
    struct control_message n_message;
};

int control_interface_send(struct control_interface *interface, struct io_uring *ring, struct control_message *message) {
    size_t total_sz = sizeof(struct _dynamic_event);
    struct _dynamic_event *d_event = malloc(total_sz);

    d_event->e.type = event_control_sent;
    d_event->e.data = d_event;

    d_event->iov.iov_base = &d_event->n_message;
    if (message->operation == OPERATION_READ) {
        d_event->iov.iov_len = 6;
    } else {
        d_event->iov.iov_len = 8;
    }

    d_event->msg.msg_name = &interface->address;
    d_event->msg.msg_namelen = sizeof(interface->address);
    d_event->msg.msg_iov = &d_event->iov;
    d_event->msg.msg_iovlen = 1;

    d_event->n_message.operation = htons(message->operation);
    d_event->n_message.object = htons(message->object);
    d_event->n_message.property = htons(message->property);
    d_event->n_message.value = htons(message->value);

    struct io_uring_sqe *sqe = io_uring_get_sqe(ring);
    if (!sqe) {
        log_error("Can't get SQE to queue sending.");
        return 1;
    }

    io_uring_prep_sendmsg(sqe, interface->fd, &d_event->msg, 0);
    io_uring_sqe_set_data(sqe, &d_event->e);

    return 0;
}

int control_interface_event(struct event_context *ctx, struct event *e) {
    if (e->type != event_control_sent) return 1;
    free(e->data);
    return 0;
}