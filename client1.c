#include <stdio.h>
#include <strings.h>

#include <sys/socket.h>
#include <arpa/inet.h>

#include <unistd.h>

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

    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        perror("Could not create socket");
        return 1;
    }

    {
        struct sockaddr_in addr;
        addr.sin_family = AF_INET;
        addr.sin_port = htons(port);
        addr.sin_addr.s_addr = inet_addr("127.0.0.1");

        if (connect(sock, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
            perror("Could not connect to signal source");
            return 1;
        }
    }

    printf("Connected to signal source on port %d\n", port);
    char buffer[1024];
    bzero(buffer, sizeof(buffer));

    while (1) {
        int n = read(sock, buffer, sizeof(buffer) - 1);
        if (n < 0) {
            perror("Could not read from socket");
            return 1;
        }

        if (n == 0) continue; // No data received

        if (buffer[n - 1] == '\n') n--; // Remove trailing newline

        buffer[n] = 0;
        printf("Received: %s\n", buffer);
        bzero(buffer, n);
    }

    return 0;
}