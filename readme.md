# Spooky Signal

Some utilities for processing the spooky signals. If the contents of this repository seem like nonsense, it is because this is a technical screening task for a job application. I've been given a Docker image that runs a server that produces data on three TCP ports. The server also operates a "control channel" by listening for messages on a UDP port. For obvious reasons, that's all the context I will provide.

## Building & running

The main deliverables are the executables `client1` and `client2`. To build them, run `make client1` and `make client2` respectively. The executables will be placed in the project root.

Additional tools are provided for analysing the data produced by `client1`. These can be found in the `analysis` directory.


## Design decisions

### Main loop & I/O scheduling

I decided to use the (relatively new-ish) `io_uring` interface for asynchronous I/O, mostly because I wanted to learn more about it. Not only is the solution using the interface to schedule read operations from the network sockets, but also to pace the main loop of the program. This is done by scheduling a multishot `IORING_OP_TIMEOUT` operation. This way, the main loop of the program is not spinning, but rather waiting for the next event to happen.


### Logging

Both `client1` and `client2` use the same logging system. The logging system is a simple wrapper around `fprintf` that writes to `stderr`. Control it by setting the `LOG_LEVEL` environment variable. The levels are:

- `none`: No logging
- `error`: Only log errors, which are fatal. This is the default.
- `warning`: Log errors and warnings.
- `info`: Log errors, warnings and info messages.

Only the specified data output will be written to `stdout`, thus keeping data and log messages separate.


### Modular design

The program is divided into modules so that parts can be reused between `client1` and `client2`. Both headers and implementations are located in the `lib` directory. The source files for the executables are located in the in the project root.


## Analysis

### Output at First Glance

I made a simple python script to capture the output of `client1`. It can be found under `analysis/plot_data.py`. It uses
`matplotlib` to plot the data. The script just generates stem plots of the data. Here is 5 seconds of data on all 3 channels:

![5 seconds of data](analysis/5_second_capture.png)

This plot shows that `out1` is a 1/2 Hz sine wave with an amplitude of 5.0. `out2` is a 1/4 Hz triangle wave with an amplitude that is also 5.0. `out1` and `out2`.

`out3` appears to be a square wave of some sort, on a longer time scale it seems like there is some kind of digital information imprinted on it:

![1 minute of out3](analysis/1_minute_capture.png)

Sometomes there are missing values, or delayed values. For example when `out2` misses the sample period it usually sends a value in and it is delayed by one sample period.

### Control Protocol

I fuzzed the control protocol and documented the different properties objects have in `lib/control.h`. All channels can be enabled and disabled, and the frequency, amplitude, and glitch chance of `out1` and `out2` can be set. For `out3` it is possible to set the max duration and min duration of the high state.

The glitch chance is controls how likely a value is skipped as mentioned above.

