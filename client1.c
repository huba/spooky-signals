#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

#include "lib/events.h"
#include "lib/channel.h"
#include "lib/log.h"



struct channel channels[3];

int loop(struct event_context *ctx, struct event *e) {
    format_channels(stdout, 3, channels, channels+1, channels+2);

    for (int i=0; i < 3; i++)
        channel_clear(channels+i);
    
    return 0;
}

int main(int argc, char *argv[]) {
    get_log_env();

    struct event_context ctx;

    if (event_init(&ctx) != 0) {
        log_error("Could not initialize event loop.");
        exit(1);
    }

    register_channel_event_handlers(&ctx);
    event_set_timeout(&ctx, 100, loop);
    channel_init_multiple(channels, &ctx, 3, 4001);

    event_run_loop(&ctx);

    return EXIT_SUCCESS;
}