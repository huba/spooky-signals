#include <stdio.h>
#include <strings.h>

#include <liburing.h>

#include "lib/channel.h"
#include "lib/log.h"

#define QUEUE_DEPTH 4
#define DELAY 100000000 // 100ms

int main(int argc, char *argv[]) {
    set_log_level(LOG_LEVEL_ERROR);

    struct io_uring ring;

    if (io_uring_queue_init(QUEUE_DEPTH, &ring, 0) != 0) {
        log_error("Could not initialize I/O ring\n");
        return 1;
    }

    struct channel c1;
    struct channel c2;
    struct channel c3;

    if (channel_init(&c1, "out1", true) != 0) return 1;
    if (channel_init(&c2, "out2", true) != 0) return 1;
    if (channel_init(&c3, "out3", true) != 0) return 1;

    channel_connect_async(&c1, &ring, "127.0.0.1", 4001);
    channel_connect_async(&c2, &ring, "127.0.0.1", 4002);
    channel_connect_async(&c3, &ring, "127.0.0.1", 4003);

    // Set up a timer
    struct __kernel_timespec delay = { .tv_sec = 0, .tv_nsec = DELAY};
    struct event delay_event = { .type = event_timeout, .data = NULL };

    {
        struct io_uring_sqe *sqe = io_uring_get_sqe(&ring);
        io_uring_prep_timeout(sqe, &delay, 0, IORING_TIMEOUT_MULTISHOT);
        io_uring_sqe_set_data(sqe, &delay_event);
        io_uring_submit(&ring);
    }
    
    // Enter the event loop
    struct io_uring_cqe *cqe;
    while (1) {
        if (io_uring_wait_cqe(&ring, &cqe) != 0) {
            log_error("Could not wait for CQE");
            return 1;
        }

        // Ignore ETIME here, since we use a timeout to pace the output loop
        if (cqe->res < 0 && cqe->res != -ETIME) {
            log_warning("Error in CQE: %d\n", cqe->res);
            io_uring_cqe_seen(&ring, cqe);
            continue;
        }

        struct event *e = (struct event *) cqe->user_data;

        switch (e->type) {
            case event_channel_connected:
            case event_channel_read:
                channel_event(e, &ring);
                break;
            case event_timeout:
                format_channels(stdout, 3, &c1, &c2, &c3);

                channel_clear(&c1);
                channel_clear(&c2);
                channel_clear(&c3);
                break;
            default:
                log_warning("Unknown event type\n");
                break;
        }

        io_uring_cqe_seen(&ring, cqe);
    }

    return 0;
}