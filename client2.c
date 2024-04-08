#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

#include "lib/events.h"
#include "lib/channel.h"
#include "lib/log.h"
#include "lib/control.h"

struct control_interface ctl;
struct channel channels[3];
uint16_t enabled = 0;

int loop(struct event_context *ctx, struct event *e) {
    // struct control_message msg = {
    //     .operation = OPERATION_WRITE,
    //     .object = OBJECT_CHANNEL_1,
    //     .property = PROPERTY_ENABLED,
    //     .value = (enabled++) % 2
    // };

    // control_interface_send(&ctl, &ctx->ring, &msg);

    format_channels(stdout, 3, channels, channels+1, channels+2);
    for (int i=0; i < 3; i++)
        channel_clear(channels+i);
    
    return 0;
}

void on_rising_edge(struct channel *channel, double new_value) {
    printf("rising edge\n");
}

void on_falling_edge(struct channel *channel, double new_value) {
    printf("falling edge\n");
}

int main(int argc, char *argv[]) {
    get_log_env();

    struct event_context ctx;

    if (event_init(&ctx) != 0) {
        log_error("Could not initialize event loop.");
        exit(1);
    }

    control_interface_init(&ctl, "127.0.0.1", 4000);

    register_control_interface_events(&ctx);
    register_channel_event_handlers(&ctx);

    event_set_timeout(&ctx, 100, loop);
    init_3_channels(&ctx, channels);

    channel_watch_falling_edge(channels+2, on_falling_edge, 3.0);
    channel_watch_rising_edge(channels+2, on_rising_edge, 3.0);

    event_run_loop(&ctx);

    return EXIT_SUCCESS;
}