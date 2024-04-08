#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

#include "lib/events.h"
#include "lib/channel.h"
#include "lib/log.h"
#include "lib/control.h"

struct control_interface ctl;
uint16_t enabled = 0;

int loop(struct event_context *ctx, struct event *e) {
    struct control_message msg = {
        .operation = OPERATION_WRITE,
        .object = OBJECT_CHANNEL_1,
        .property = PROPERTY_ENABLED,
        .value = (enabled++) % 2
    };

    control_interface_send(&ctl, &ctx->ring, &msg);
    
    return 0;
}

int main(int argc, char *argv[]) {
    get_log_env();

    struct event_context ctx;

    if (event_init(&ctx) != 0) {
        log_error("Could not initialize event loop.");
        exit(1);
    }

    control_interface_init(&ctl, "127.0.0.1", 4000);

    // struct control_message msg = {
    //     .operation = OPERATION_READ,
    //     .object = 1,
    //     .property = property++,
    // };
    // control_interface_send(&ctl, &ctx.ring, &msg);

    register_control_interface_events(&ctx);
    event_set_timeout(&ctx, 1000, loop);

    event_run_loop(&ctx);

    return EXIT_SUCCESS;
}