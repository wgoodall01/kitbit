# KitBit

Smart pet tracking for improved animal–human communication.

## What's Inside

This repo has two main parts:
 
- `KitBit`: Arduino sketch, samples the IMU and transmits data over Bluetooth LE.
- `kitbit-host`: Data logger, connects over Bluetooth LE and receives data from the Arduino.


## Building

To upload the Arduino sketch, use the commands in the Makefile:

```
# Download the Arduino CLI, `arduino:mbed_nano` core, and libraries
$ make setup

# Compile the KitBit firmware, and upload it to the device at `$KITBIT_PORT`.
$ export KITBIT_PORT="/dev/$device" make upload
```

To run the host program to collect data, build it with Cargo:

```
# (if necessary) Install a Rust toolchain with Rustup
$ curl --proto '=https' --tlsv1.2 -sSf https://sh.rustup.rs | sh

# Build and run kitbit-host
$ cd kitbit-host && cargo run
```
