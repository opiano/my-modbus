# Multi-Device Modbus Master & Slave

This repository contains a concurrent Modbus Master and Slave implementation written in C, utilizing `libmodbus` and POSIX threads (`pthreads`). It is designed to support both Modbus RTU and Modbus TCP protocols, enabling simultaneous communication with multiple devices.

## Features

- **Concurrent Multi-Device Communication**: Implements a parallel, thread-based architecture to handle multiple Modbus connections (TCP and RTU) concurrently without blocking.
- **Modbus Master and Slave**: Provides separate binaries for Modbus Master (`piModbusMaster`) and Modbus Slave (`piModbusSlave`).
- **Flexible Configuration (INI Format)**: Uses an easy-to-read INI configuration file to define multiple masters (`[TCPMaster]`, `[RTUMaster]`) and their respective polling actions (`[Action]`).
- **Enhanced Debugging**: Provides clear console output, tagging register values with their source IP addresses or serial ports to easily identify incoming data.

## Prerequisites

To build and run this project, you will need:
- GCC (or a compatible C99 compiler)
- `libmodbus` development library
- `pthread` library

On Debian/Ubuntu-based systems, you can install the dependencies via:
```bash
sudo apt-get update
sudo apt-get install build-essential libmodbus-dev
```

## Building the Project

A `Makefile` is provided to compile both the Master and Slave applications.

To build the project, simply run:
```bash
make
```

This will generate two executables in the root directory:
- `piModbusMaster`
- `piModbusSlave`

To clean the compiled binaries and object files, run:
```bash
make clean
```

## Configuration

The master application uses an INI-style configuration file to define the devices it should connect to and the actions it should perform. Below is an example configuration:

```ini
# ===================================================
# 1. First Port : Modbus RTU Device
# ===================================================
[RTUMaster]
Device=/dev/ttyUSB0
Baudrate=19200
Parity=N
Databits=8
Stopbits=1

# Data for Slave 1 on /dev/ttyUSB0
[Action]
ActionSlaveAddress=1
ActionStartRegister=1
ActionRegisterCount=5
ActionInterval_ms=1000

# Data for Slave 2 on /dev/ttyUSB0
[Action]
ActionSlaveAddress=2
ActionStartRegister=10
ActionRegisterCount=2
ActionInterval_ms=1000

# ===================================================
# 2. Second Port : Modbus TCP Device
# ===================================================
[TCPMaster]
IP=192.168.219.251
Port=502

[Action]
ActionSlaveAddress=1
ActionStartRegister=1
ActionRegisterCount=5
```

## Running the Application

Run the master application by passing the configuration file as an argument (or it may default to a specific path depending on the implementation):

```bash
./piModbusMaster
```

*(Note: If the application requires root privileges for accessing serial ports, run it with `sudo`).*
