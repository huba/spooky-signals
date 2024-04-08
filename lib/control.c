#include "control.h"

#include <strings.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>

#include "log.h"

int control_interface_init(struct control_interface *interface, const char *address, int port) {
    bzero(interface->buffer, sizeof(interface->buffer));
    interface->buffer_length = 0;
    
    interface->event.type = event_type_none;
    interface->event.data = interface;

    bzero(&interface->address, sizeof(interface->address));
    interface->address.sin_family = AF_INET;
    interface->address.sin_port = htons(port);
    interface->address.sin_addr.s_addr = inet_addr(address);

    interface->iov.iov_base = interface->buffer;
    interface->iov.iov_len = interface->buffer_length;

    interface->msg.msg_name = &interface->address;
    interface->msg.msg_namelen = sizeof(interface->address);
    interface->msg.msg_iov = &interface->iov;
    interface->msg.msg_iovlen = 1;

    interface->fd = socket(AF_INET, SOCK_DGRAM, 0);
    if (interface->fd < 0) {
        log_error("Failed to create socket for control interface.");
        return 1;
    }

    return 0;
}

int _is_control_busy(struct control_interface *ctl) {
    return ctl->event.type != event_type_none;
}

int control_interface_send(struct control_interface *interface, struct io_uring *ring, struct control_message *message) {
    if (_is_control_busy(interface)) return 1;

    if (message->operation == OPERATION_READ) {
        interface->buffer_length = 6;
    } else {
        interface->buffer_length = 8;
    }

    {
        struct control_message network_message = {
            .operation = htons(message->operation),
            .object = htons(message->object),
            .property = htons(message->property),
            .value = htons(message->value)
        };

        memcpy(interface->buffer, &network_message, interface->buffer_length);
    }

    struct io_uring_sqe *sqe = io_uring_get_sqe(ring);
    if (!sqe) {
        log_error("Can't get SQE to queue sending.");
        return 1;
    }

    interface->iov.iov_len = interface->buffer_length;

    io_uring_prep_sendmsg(sqe, interface->fd, &interface->msg, 0);
    interface->event.type = event_control_sent;
    io_uring_sqe_set_data(sqe, &interface->event);

    return io_uring_submit(ring);
}

int control_interface_event(struct event_context *ctx, struct event *e) {
    struct control_interface *ctl = e->data;

    if (e->type != event_control_sent) return 1;

    ctl->event.type = event_type_none;
    return 0;
}