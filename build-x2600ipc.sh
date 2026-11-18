#!/bin/sh
# Build the Ingenic X2600 co-processor (RISC-V) Klipper firmware for the
# Creality K1 main motion MCU, and stage it into fw/K1-RISCV/.
#
# Requires a RISC-V embedded GCC on PATH (xPack riscv-none-elf-gcc).
#   export PATH=$HOME/xpack-riscv-none-elf-gcc-13.2.0-2/bin:$PATH
#   ./build-x2600ipc.sh
set -e
cd "$(dirname "$0")"

if ! command -v riscv-none-elf-gcc >/dev/null 2>&1; then
    echo "error: riscv-none-elf-gcc not on PATH" >&2
    exit 1
fi

cp .config.x2600ipc .config
make olddefconfig
make -j"$(nproc)"

mkdir -p fw/K1-RISCV
cp out/klipper.bin fw/K1-RISCV/klipper_main_ipc.bin
echo "Staged fw/K1-RISCV/klipper_main_ipc.bin"
