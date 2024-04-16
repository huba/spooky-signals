#include "events.h"
#include "log.h"

#define QUEUE_DEPTH 10

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

void _handle_events(struct event_context *ctx) {
    struct io_uring_cqe *cqe;
    struct event *e;
    unsigned head;
    unsigned i = 0;

    io_uring_for_each_cqe(&ctx->ring, head, cqe) {
        if (cqe->res < 0 && cqe->res != -ETIME) {
            log_warning("Error in CQE: %d\n", cqe->res);
            io_uring_cqe_seen(&ctx->ring, cqe);
            continue;
        }

        e = (struct event *) cqe->user_data;

        if (e->type < 0 && e->type >= event_type_count) {
            log_warning("Invalid event type.");
        } else if (ctx->event_handlers[e->type] == NULL) {
            log_warning("Event type has no registered handler function.");
        } else {
            int res = ctx->event_handlers[e->type](ctx, e);
            log_info("Event %d processed with result %d\n", e->type, res);
        }

        i++;
    }

    log_info("Event loop handled %u events.\n", i);

    io_uring_cq_advance(&ctx->ring, i);
}

int event_run_loop(struct event_context *ctx) {
    int submitted;
    struct event delay_event = { .type = event_timeout, .data = NULL };

    struct io_uring_sqe *sqe = io_uring_get_sqe(&ctx->ring);
    io_uring_prep_timeout(sqe, &ctx->loop_delay, 0, IORING_TIMEOUT_MULTISHOT);
    io_uring_sqe_set_data(sqe, &delay_event);

    ctx->running = true;
    
    while(ctx->running) {
        submitted = io_uring_submit_and_wait(&ctx->ring, 1);
        if (submitted < 0) {
            log_warning("Could not wait for CQE\n");
            return -1;
        }

        log_info("Submitted %d events.\n", submitted);

        _handle_events(ctx);
    }

    log_info("Exitting main loop.\n");
    return 0;
}