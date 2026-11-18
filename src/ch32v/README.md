# WCH CH32V (RISC-V) support — Creality K1 nozzle & bed MCUs

This adds a Kalico MCU architecture port for the **WCH CH32V** family (QingKe
RISC-V core) used on the **RISC-V revision of the Creality K1 / K1C / K1 Max**
nozzle (toolhead) and bed-leveling MCUs.

## Background

The RISC-V K1 uses three motion MCUs (identified by disassembling the stock
`ext-root/lib/firmware/klipper_*.bin` images):

| Stock image | Chip | Transport |
|---|---|---|
| `klipper_main_ipc.bin` | Ingenic X2600-embedded RISC-V | RPMsg via `soc_mcu.ko` |
| `klipper_nozzle_serial_app.bin` | **WCH CH32V** (QingKe) | UART `/dev/ttyS3` @ 230400, IAP dtype `0xA1` |
| `klipper_leveling_serial_app.bin` | **WCH CH32V** (QingKe) | UART `/dev/ttyS4` @ 230400, IAP dtype `0xA3` |

The CH32V peripherals (RCC, GPIO, AFIO, USART, TIM, ADC) are **register-compatible
with the STM32F1** map; the CPU core is QingKe RISC-V with a **PFIC** interrupt
controller (global interrupt enable is custom CSR `0x800`, not the ARM NVIC).

This port covers the **nozzle and bed MCUs** (the two serial-attached CH32V
chips). The X2600-IPC main MCU is a separate, larger effort (RPMsg transport +
prtouch/ldc1612 sensor glue) and is **not** included here.

## Files

- `src/ch32v/Kconfig` — arch + chip selection (`MACH_CH32V103` / `MACH_CH32V203`)
- `src/ch32v/Makefile` — `riscv-none-elf-` toolchain, `rv32imac_zicsr` flags
- `src/ch32v/startup.S` — vector table, reset handler, PFIC enable helper
- `src/ch32v/main.c` — entry point + dynamic-memory pool
- `src/ch32v/irq.c` — PFIC global interrupt save/restore (CSR `0x800`)
- `src/ch32v/clock.c` — RCC HSE→PLL (72 MHz V103 / 144 MHz V203)
- `src/ch32v/timer.c` — TIM2 free-running timer (16→32-bit software extension)
- `src/ch32v/gpio.c` / `gpio.h` — STM32F1-map GPIO
- `src/ch32v/serial.c` — USART1 console (PA9 TX / PA10 RX) via `serial_irq.c`
- `src/ch32v/adc.c` — ADC1 (thermistors + internal temp sensor)
- `src/ch32v/internal.h` — register map + QingKe core helpers
- `src/ch32v/ch32v_link.lds.S` — RISC-V linker script
- `build-ch32v.sh`, `.config.ch32v` — reproducible build
- `fw/K1-RISCV/` — staged nozzle/bed firmware images

Not yet implemented (software fallbacks available): hardware SPI
(`HAVE_GPIO_SPI`) and hardware PWM (`HAVE_GPIO_HARD_PWM`). Fans/heaters use
software PWM; software SPI is available via `CONFIG_WANT_SOFTWARE_SPI`.

## Build

```sh
# xPack RISC-V toolchain
export PATH=$HOME/xpack-riscv-none-elf-gcc-13.2.0-2/bin:$PATH
./build-ch32v.sh
```

Or manually:

```sh
cp .config.ch32v .config
make olddefconfig
make -j$(nproc)
cp out/klipper.bin fw/K1-RISCV/klipper_nozzle_serial_app.bin
cp out/klipper.bin fw/K1-RISCV/klipper_leveling_serial_app.bin
```

The nozzle and bed MCUs run the **same image**; they are distinguished only by
the IAP device-type flag used during flashing.

## Flash (on the K1, via the creality repo)

`pellcorp/creality`'s `k1/services/S13mcu_update` detects the RISC-V K1
(presence of `cmd_iap` + `/dev/ttyS3` + `/dev/ttyS4`) and flashes the staged
images through the stock `cmd_iap` bootloader:

```sh
cmd_iap --device /dev/ttyS3 --baudrate 230400 --dtype 0xA1 --upgrade \
        --firmware fw/K1-RISCV/klipper_nozzle_serial_app.bin
cmd_iap --device /dev/ttyS4 --baudrate 230400 --dtype 0xA3 --upgrade \
        --firmware fw/K1-RISCV/klipper_leveling_serial_app.bin
```

The service checksums the staged images and only re-flashes on change, so it is
safe to run at every boot.

## Status / verification

- Compiles and links cleanly into a valid RISC-V image (`out/klipper.bin`,
  ~40 KB, fits CH32V103's 64 KB flash; BSS ~10 KB fits 20 KB RAM).
- Reset vector, stack top (`0x20005000`), and `.data`/`.bss` init verified by
  disassembly.
- **Not yet run on hardware.** Needs a RISC-V K1 unit to validate boot, USART
  console, steppers, and ADC. The CH32V103/203 register map was confirmed by
  disassembling the stock firmware, but PFIC vector numbering (TIM2 = IRQ 28,
  USART1 = IRQ 37) and the PLL multiplier fields should be confirmed against the
  WCH reference manual before first flash.
