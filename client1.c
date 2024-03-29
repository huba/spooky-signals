#include <stdio.h>
#include <strings.h>

#include "lib/channel.h"

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <port>\n", argv[0]);
        return 1;
    }

    int port;
    if (sscanf(argv[1], "%d", &port) != 1) {
        perror("Invalid port number");
        return 1;
    }

    struct channel c;

    if (channel_init(&c, "client1") != 0) return 1;
    if (channel_connect_sync(&c, "127.0.0.1", port) != 0) return 1;

    while (1) {
        if (channel_read_sync(&c) != 0) break;
        printf("Received: %s\n", c.state);
    }

    return 0;
}