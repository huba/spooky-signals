#!/usr/bin/env python

import matplotlib.pyplot as plt

import subprocess
import signal
import io
import json
import sys

from dataclasses import dataclass, field

def progress(count: int|float, total: int|float, task: str = "Working...", bar_length: int = 60):
    ratio = count / total
    filled_length = int(round(ratio * bar_length))
    bar = ("=" * filled_length) + ("-" * (bar_length - filled_length))

    sys.stdout.write(f"[{bar}] {round(100 * ratio, 1)}% - {task}\r")
    sys.stdout.flush()

@dataclass
class Channel:
    name: str
    times: list[int] = field(default_factory=list)
    values: list[float] = field(default_factory=list)

    @property
    def datapoints(self):
        return len(self.times)

    @property
    def last(self):
        return (self.times[-1], self.values[-1])


def collect_data(data_channels: list[Channel], sync_channel: int = 0, time_limit: int =10_000):
    start_time = None
    started = False
    prev_value = None
    sync_channel_name = data_channels[sync_channel].name

    print(f"Collecting data for {time_limit / 1000} seconds.")
    progress(0, time_limit, f"Syncing to {sync_channel_name}...")

    proc = subprocess.Popen("./client2", stdout=subprocess.PIPE)

    for line in io.TextIOWrapper(proc.stdout, encoding="utf-8"):
        output = json.loads(line)

        if (not started) and (output[sync_channel_name] != "--"):
            this_value = float(output[sync_channel_name])

            if prev_value != None:
                # whenever we cross 0 while rising
                if (prev_value < 0 and this_value >= 0):
                    started = True
                    start_time = output["timestamp"]
            
            prev_value = this_value

        if started:
            time = output["timestamp"] - start_time

            for channel in data_channels:
                if output[channel.name] != "--":
                    channel.times.append(time)
                    channel.values.append(float(output[channel.name])) # big assumption here
            
            progress(time, time_limit, "Collecting data...")
            
            if time > time_limit:
                break

    progress(time_limit, time_limit, "Collecting data...")
    print("")

    proc.send_signal(signal.SIGINT) # stop that!

    for channel in data_channels:
        print(f'Collected {channel.datapoints} datapoints for {channel.name}.')



if __name__ == "__main__":
    data_channels = [Channel("out1"), Channel("out2"), Channel("out3")]
    time = 10_000

    collect_data(data_channels, 1, time)

    fig, ax = plt.subplots(len(data_channels), 1, layout="constrained")

    for i, channel in enumerate(data_channels):
        ax[i].stem(channel.times, channel.values, label=channel.name)
        ax[i].set_xlim(0, time)
        ax[i].set_ylim(-8, 8)
        ax[i].set_ylabel(channel.name)
    
    ax[-1].set_xlabel("Time [ms]")

    plt.show()