# multiio

Custom bare-metal firmware and software for the **Microphase Z7-Lite** (Xilinx Zynq-7000, `xc7z010clg400-1`).

This repository is a modular, no-OS (standalone) application developed for **Vivado / Vitis 2024.2**. It serves as a hardware bring-up and feature-validation platform before porting to PetaLinux or Buildroot. The design emphasizes clean component isolation through a lightweight Middleware framework, interrupt-driven peripherals, and dual-core capable memory layout.

> **Important**: Code and platform files are written for the 2024.2 tool chain. Older tool versions are not supported.

## Branch: `can`

This branch adds full support for the Processing System CAN controllers (`XCanPs`).

### CAN Features

- Interrupt-driven transmit and receive with double-buffered `LiteQueue` (64 frames)
- Configurable baud rate (brings up at 33 333 baud in loopback mode by default)
- Standard and extended identifiers, DLC, and 8-byte data frames
- Statistics and simple test command interface
- Integrated into the Middleware command system

Supported commands (via the shared command processor):

```
can <deviceId> status
can <deviceId> test
```

## Architecture

| Component              | Role |
|------------------------|------|
| `Middleware`           | Base class for all functional modules. Provides `Loop()` and `ProcessCommand()` plus registration/priority ordering. |
| `CanController`        | Wrapper around `XCanPs` with ISR handlers, queues, baud-rate calculation, and command support. |
| `InterruptManager`     | Centralized SCU GIC setup, attach/detach, priority, and enable/disable. |
| `HwClock`              | RTC driver (AXI IIC + GPIO interrupt, typical address `0x68`). |
| `LiteQueue`            | Fixed-capacity, cache-friendly queue template used for ISR-to-task communication. |
| `Helpers`              | Time (`Stopwatch`), text/ANSI, random, CRC, logging macros, and other utilities. |
| `Core0`                | CPU0 entry point: initializes subsystems, starts CPU1, then runs the main loop. |
| `usb/`                 | Xilinx USB PS device-mode mass-storage example sources. |

### Dual-Core & Memory Map

The custom linker script (`lscript.ld`) partitions DDR:

- `cpu0_ddr` / `cpu1_ddr` — separate application regions
- Dedicated DMA and LiteQueue regions
- OCM regions

`Core0` writes the CPU1 start address and issues a `sev` to bring up the second core.

## Building (Vitis 2024.2)

1. Create a Vivado 2024.2 hardware platform for the Microphase Z7-Lite that includes the PS CAN, AXI IIC (RTC), AXI GPIO (RTC interrupt), and any other peripherals used by the design.
2. Export the platform / XSA and create a Vitis bare-metal application project targeting the Cortex-A9.
3. Replace the default linker script with `lscript.ld` and the startup specification with `Xilinx.spec`.
4. Add all source files from this branch (and the `usb/` directory if USB is enabled).
5. Ensure `xparameters.h` matches the hardware platform (especially `XPAR_XCANPS_NUM_INSTANCES`, CAN clock frequency, IIC and GPIO device/interrupt IDs).

The application is self-contained and does not require an OS.

## Project Structure

```
.
├── CanController.cpp / .h      # PS CAN controller
├── Core0.cpp                   # CPU0 main
├── Middleware.cpp / .h         # Component framework
├── InterruptManager.cpp / .h
├── HwClock.cpp / .h            # RTC
├── LiteQueue.cpp / .hpp
├── Helpers.cpp / .h
├── usb/                        # USB PS examples
├── lscript.ld                  # Dual-core linker script
└── Xilinx.spec
```

## Status

- CAN controller is functional in loopback mode with status and test commands.
- RTC and interrupt infrastructure are active.
- Dual-core startup is implemented.
- USB sources are included but not yet integrated into the Middleware framework on this branch.

## License

See repository license (if present). All original code is authored by Jacob A. Psimos.
