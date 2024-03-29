#ifndef CHANNEL_H
#define CHANNEL_H

struct channel {
    int socket_fd;
    char name[20];
    char state[20];
};

int channel_init(struct channel *c, const char *name);

void channel_clear(struct channel *c);

int channel_connect_sync(struct channel *c, const char *ip, int port);
int channel_read_sync(struct channel *c);

#endif // CHANNEL_H
