#include "events.h"
#include "log.h"

#define QUEUE_DEPTH 4

int event_init(struct event_context *ctx) {
    ctx->running = false;

    for (int i=0; i < event_type_count; i++)
        ctx->event_handlers[i] = NULL;

    ctx->loop_delay.tv_nsec = 0;
    ctx->loop_delay.tv_sec = 0;
    
    if (io_uring_queue_init(QUEUE_DEPTH, &ctx->ring, 0) != 0) {
        log_error("Could not initialize I/O ring\n");
        return -1;
    }

    return 0;
}

void event_set_timeout(struct event_context *ctx, long long timeout_ms, event_handler_function loop_handler) {
    ctx->loop_delay.tv_sec = timeout_ms / 1000;
    ctx->loop_delay.tv_nsec = (timeout_ms % 1000) * 1000000;

    ctx->event_handlers[event_timeout] = loop_handler;
}

void event_register_handler(struct event_context *ctx, enum event_type e, event_handler_function handler) {
    ctx->event_handlers[e] = handler;
}

int event_run_loop(struct event_context *ctx) {
    struct event delay_event = { .type = event_timeout, .data = NULL };

    struct io_uring_sqe *sqe = io_uring_get_sqe(&ctx->ring);
    io_uring_prep_timeout(sqe, &ctx->loop_delay, 0, IORING_TIMEOUT_MULTISHOT);
    io_uring_sqe_set_data(sqe, &delay_event);
    io_uring_submit(&ctx->ring);

    ctx->running = true;
    struct io_uring_cqe *cqe;
    
    while(ctx->running) {
        if (io_uring_wait_cqe(&ctx->ring, &cqe) != 0) {
            log_warning("Could not wait for CQE\n");
            return -1;
        }

        // Ignore ETIME here, since we use a timeout to pace the output loop
        if (cqe->res < 0 && cqe->res != -ETIME) {
            log_warning("Error in CQE: %d\n", cqe->res);
            io_uring_cqe_seen(&ctx->ring, cqe);
            continue;
        }

        struct event *e = (struct event *) cqe->user_data;

        if (e->type < 0 && e->type >= event_type_count) {
            log_warning("Invalid event type.");
        } else if (ctx->event_handlers[e->type] == NULL) {
            log_warning("Event type has no registered handler function.");
        } else {
            ctx->event_handlers[e->type](ctx, e);
        }

        io_uring_cqe_seen(&ctx->ring, cqe);
    }

    log_info("Exitting main loop.\n");
    return 0;
}