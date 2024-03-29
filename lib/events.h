#ifndef EVENTS_H
#define EVENTS_H

enum event_type {
    event_type_none,
    event_channel_connected,
    event_channel_read,
    event_timeout
};

struct event {
    enum event_type type;
    void *data;
};

#endif // EVENTS_H