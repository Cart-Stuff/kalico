# Ingenic X2600 co-processor (RISC-V) — Creality K1 main motion MCU

This adds a Kalico MCU architecture port for the **Ingenic X2600 co-processor**
(the RISC-V real-time core inside the X2600 SoC), which is the **main motion
MCU** on the RISC-V revision of the Creality K1 / K1C / K1 Max.

## Background

The RISC-V K1's main motion firmware (`klipper_main_ipc.bin`) runs on the
X2600's RISC-V co-processor, **not** a standalone UART MCU. The host (X2600
XBurst MIPS core) loads the firmware into the co-processor's SRAM via the
bootloader and talks to it over **shared-memory ring buffers + a doorbell IRQ**
(the `soc_mcu.ko` kernel module). Console messages use the **standard Klipper
`0x7E`/CRC16 framing** carried over those rings — the same wire protocol as
Klipper serial, just transported via IPC instead of UART.

The layout was recovered by disassembling the stock `klipper_main_ipc.bin` and
`soc_mcu.ko`:

- The image begins with a **TPEL boot header** at `0x0FF20000`:
  - `+0x00`: `jal ResetHandler`
  - `+0x04`: `"TPEL"` magic (`0x4C455054`)
  - `+0x18`: command-ring descriptor pointer (host → MCU)
  - `+0x1C`: response-ring descriptor pointer (MCU → host)
  - `+0x24`: ring size (`0x1000` = 4096)
- The bootloader points register `0xB2A00004` at this header; the host's
  `soc_mcu.ko` reads the two ring pointers from it.
- Each ring descriptor is `{ data_ptr, size, w_pos, r_pos }` (producer/consumer).
- **Doorbell:** MCU sets bit 8 of `0x13650014` to notify the host; the host
  writes `1` to `0xB2A00008` to notify the MCU (acked via `0xB2A0000C`).

## Files

- `src/x2600ipc/Kconfig` — arch + memory map (text `0x0FF20000`, RAM `0x12B24000`)
- `src/x2600ipc/Makefile` — `riscv-none-elf-` toolchain, `rv32imac_zicsr`
- `src/x2600ipc/startup.S` — **TPEL header** + reset handler
- `src/x2600ipc/main.c` — entry point + dynamic-memory pool
- `src/x2600ipc/irq.c` — standard RISC-V `mstatus.MIE` save/restore
- `src/x2600ipc/timer.c` — `rdcycle` free-running timer (32→64-bit extension)
- `src/x2600ipc/ipc.c` — shared-memory ring read/write + doorbell
- `src/x2600ipc/console.c` — Klipper command framing over the rings
- `src/x2600ipc/gpio.c` / `gpio.h` — Ingenic-standard GPIO (PXDAT/PXDIR)
- `src/x2600ipc/adc.c` — SAR ADC
- `src/x2600ipc/internal.h` — TPEL/ring/IPC register definitions
- `src/x2600ipc/x2600ipc_link.lds.S` — RISC-V linker script
- `build-x2600ipc.sh`, `.config.x2600ipc` — reproducible build
- `fw/K1-RISCV/klipper_main_ipc.bin` — staged main-MCU image

## Build

```sh
export PATH=$HOME/xpack-riscv-none-elf-gcc-13.2.0-2/bin:$PATH
./build-x2600ipc.sh
```

## Deploy (on the K1)

The co-processor firmware is loaded by the bootloader, **not** by `cmd_iap`
(the nozzle/bed CH32V MCUs use `cmd_iap`). To use a custom `klipper_main_ipc.bin`,
replace the stock image at `/lib/firmware/klipper_main_ipc.bin` on the K1 and
reload the co-processor (the `soc_mcu` module / remoteproc), or power-cycle.

The host `soc_mcu.ko` reads the TPEL header from the loaded image and connects
to the rings, so no host-side change is needed as long as the header layout is
preserved (this port reproduces it exactly).

## Status / verification

- Compiles and links into a valid RISC-V image (`out/klipper.bin`, ~41 KB).
- **TPEL header verified byte-for-byte** against the stock layout (reset jump,
  magic, ring pointers, ring size).
- Ring descriptor field order `{ data, size, w_pos, r_pos }` confirmed against
  `soc_mcu.ko`'s `mcu_write_data`/`mcu_read_data`.
- **Not yet run on hardware.** The IPC transport (ring + doorbell) is the novel
  part and matches the kernel module's contract. The GPIO base (`0x10010000`,
  Ingenic-standard) and SAR ADC base (`0x10070000`) follow the Ingenic family
  register model but should be confirmed against the X2600 Programmer's Manual
  on first hardware test. The X2600 also has a dedicated TPC stepper engine
  (used by the stock firmware via the vendor HAL); this port drives steppers
  through generic GPIO, which is simpler but lower-throughput than the TPC.
