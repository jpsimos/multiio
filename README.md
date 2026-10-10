# multiio

Custom bare-metal firmware and software for the **Microphase Z7-Lite** (Xilinx Zynq-7000, `xc7z010clg400-1`).

This repository is a modular, no-OS (standalone) application developed for **Vivado / Vitis 2024.2**. It serves as a hardware bring-up and feature-validation platform before porting to PetaLinux or Buildroot. The design emphasizes clean component isolation through a lightweight Middleware framework, interrupt-driven peripherals, and dual-core capable memory layout.

> **Important**: Code and platform files are written for the 2024.2 tool chain. Older tool versions are not supported.

## Branch: `can`

This branch provides a working multi-peripheral test platform centered on the Processing System CAN controllers, with UART console, RTC, and USB support integrated through the Middleware system.

### Currently Active Subsystems (Core0)

`Core0.cpp` creates, configures, and registers:

- `UartController` (PS UART0) — console / command interface
- `HwClock` — RTC (AXI IIC + GPIO interrupt, address `0x68`)
- `CanController` × 2 (PS CAN0 and CAN1) — 500 kbit/s, normal mode
- `UsbController` (PS USB0) — ULPI transceiver bring-up

The main loop simply calls `Middleware::LoopRegisteredMiddlewares()`.

### CAN Features

- Interrupt-driven transmit and receive with double-buffered `LiteQueue`
- Baud-rate configuration (defaults to 500000 in `XCANPS_MODE_NORMAL`)
- Standard and extended identifiers, variable-length data
- Statistics (`status`) and transmit test (`test`) commands
- Periodic self-test in `Loop()` that alternates between the two controllers

Supported commands:

```
can <deviceId> status
can <deviceId> test
```

### Other Command Interfaces

UART lines are parsed and dispatched to all registered middlewares.

| Prefix   | Commands (examples)                  | Status                  |
|----------|--------------------------------------|-------------------------|
| `can`    | `status`, `test`                     | Active (both controllers) |
| `usb`    | `test` (ULPI VID/PID + scratch)      | Active                  |
| `sd`     | `status`, `mount`, `unmount`, `ls`   | Code present, not yet registered in Core0 |
| (UART)   | line-based command entry             | Active console          |

### Architecture

| Component              | Role |
|------------------------|------|
| `Middleware`           | Base class for all functional modules. Provides `Loop()` and `ProcessCommand()` plus registration/priority ordering. |
| `CanController`        | Wrapper around `XCanPs` with ISR handlers, queues, baud-rate calculation, and command support. |
| `UartController`       | PS UART console that reads lines and dispatches commands. |
| `UsbController`        | PS USB device controller with ULPI register access and self-test. |
| `SdCardController`     | PS SD card + FatFs mount/status/list (present but not yet started). |
| `InterruptManager`     | Centralized SCU GIC setup, attach/detach, priority, and enable/disable. |
| `HwClock`              | RTC driver (AXI IIC + GPIO interrupt). |
| `LiteQueue`            | Fixed-capacity, cache-friendly queue template used for ISR-to-task communication. |
| `Helpers`              | Time (`Stopwatch`), text/ANSI, random, CRC, logging macros, and other utilities. |
| `Core0`                | CPU0 entry point: initializes subsystems, starts CPU1, then runs the Middleware loop. |
| `usb/`                 | Supporting Xilinx USB PS device-mode sources. |

### Dual-Core & Memory Map

The custom linker script (`lscript.ld`) partitions DDR:

- `cpu0_ddr` / `cpu1_ddr` — separate application regions
- Dedicated DMA and LiteQueue regions
- OCM regions

`Core0` writes the CPU1 start address and issues a `sev` to bring up the second core. OCM attributes are configured at startup.

## Building (Vitis 2024.2)

1. Create a Vivado 2024.2 hardware platform for the Microphase Z7-Lite that includes the PS CAN (both instances), PS UART, PS USB, AXI IIC (RTC), AXI GPIO (RTC interrupt), and PS SD if you enable the SD controller.
2. Export the platform / XSA and create a Vitis bare-metal application project targeting the Cortex-A9.
3. Replace the default linker script with `lscript.ld` and the startup specification with `Xilinx.spec`.
4. Add all source files from this branch (including the `usb/` directory).
5. Ensure `xparameters.h` matches the hardware platform (especially CAN device/interrupt IDs, UART, USB, IIC, and GPIO).

The application is self-contained and does not require an OS.

## Project Structure

```
.
├── CanController.cpp / .h      # PS CAN (both instances)
├── Core0.cpp                   # CPU0 main + subsystem registration
├── UartController.cpp / .h     # Console / command dispatcher
├── UsbController.cpp / .h      # PS USB + ULPI
├── SdCardController.cpp / .h   # PS SD + FatFs (not yet started)
├── Middleware.cpp / .h         # Component framework
├── InterruptManager.cpp / .h
├── HwClock.cpp / .h            # RTC
├── LiteQueue.cpp / .hpp
├── Helpers.cpp / .h
├── usb/                        # USB PS supporting sources
├── lscript.ld                  # Dual-core linker script
└── Xilinx.spec
```

## Status

- Dual CAN controllers running in normal mode at 500 kbit/s with status and test commands.
- UART console dispatches commands to the Middleware system.
- RTC and interrupt infrastructure are active.
- USB ULPI self-test is available.
- Dual-core startup and OCM configuration are implemented.
- SD card controller sources are present but not yet instantiated in `Core0`.

## License

See repository license (if present). All original code is authored by Jacob A. Psimos.
