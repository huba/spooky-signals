#include "channel.h"

#include <string.h>
#include <stdio.h>
#include <sys/socket.h>
#include <arpa/inet.h>

#include <unistd.h>

int channel_init(struct channel *c, const char *name) {
    strncpy(c->name, name, sizeof(c->name) - 1);
    c->name[sizeof(c->name) - 1] = 0;

    channel_clear(c);

    c->socket_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (c->socket_fd < 0) {
        perror("Could not create socket");
        return 1;
    }

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