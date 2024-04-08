#ifndef CONTROL_H
#define CONTROL_H

#include <stdint.h>
#include <netinet/in.h>
#include <liburing.h>

#include "events.h"

#define OPERATION_READ 1
#define OPERATION_WRITE 2

#define OBJECT_CHANNEL_1 1
#define OBJECT_CHANNEL_2 2
#define OBJECT_CHANNEL_3 3

#define PROPERTY_ENABLED 14

#define PROPERY_MIN_DURATION 42
#define PROPERTY_MAX_DURATION 43

#define PROPERTY_AMPLITUDE 170
#define PROPERTY_FREQUENCY 255
#define PROPERTY_GLITCH_CHANCE 300

struct control_message {
    uint16_t operation;
    uint16_t object;
    uint16_t property;
    uint16_t value;
};

struct control_interface {
    uint8_t *buffer;
    int buffer_length;
    int fd;
    struct sockaddr_in address;
};

int control_interface_init(struct control_interface *interface, const char *address, int port);
int control_interface_send(struct control_interface *interface, struct io_uring *ring, struct control_message *message);
int control_interface_event(struct event_context *ctx, struct event *e);

static inline int control_interface_enable(struct control_interface *ctl, struct io_uring *ring, uint16_t channel) {
    struct control_message msg = {
        .operation = OPERATION_WRITE,
        .object = channel,
        .property = PROPERTY_ENABLED,
        .value = 1
    };

    return control_interface_send(ctl, ring, &msg);
}

static inline int control_interface_disable(struct control_interface *ctl, struct io_uring *ring, uint16_t channel) {
    struct control_message msg = {
        .operation = OPERATION_WRITE,
        .object = channel,
        .property = PROPERTY_ENABLED,
        .value = 0
    };

    return control_interface_send(ctl, ring, &msg);
}

static inline void register_control_interface_events(struct event_context *ctx) {
    event_register_handler(ctx, event_control_sent, control_interface_event);
}

#endif // CONTROL_H