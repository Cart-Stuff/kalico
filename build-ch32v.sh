#!/bin/sh
# Build the WCH CH32V (RISC-V) Klipper firmware for the Creality K1 nozzle and
# bed-leveling MCUs, and stage the images into fw/K1-RISCV/.
#
# Requires a RISC-V embedded GCC on PATH (xPack riscv-none-elf-gcc, or
# riscv64-unknown-elf-gcc). Example:
#   export PATH=$HOME/xpack-riscv-none-elf-gcc-13.2.0-2/bin:$PATH
#   ./build-ch32v.sh
set -e
cd "$(dirname "$0")"

if ! command -v riscv-none-elf-gcc >/dev/null 2>&1; then
    echo "error: riscv-none-elf-gcc not on PATH" >&2
    echo "Install the xPack RISC-V toolchain and add its bin/ to PATH." >&2
    exit 1
fi

cp .config.ch32v .config
make olddefconfig
make -j"$(nproc)"

mkdir -p fw/K1-RISCV
cp out/klipper.bin fw/K1-RISCV/klipper_nozzle_serial_app.bin
cp out/klipper.bin fw/K1-RISCV/klipper_leveling_serial_app.bin
echo "Staged fw/K1-RISCV/klipper_{nozzle,leveling}_serial_app.bin"
