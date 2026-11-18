# Additional WCH CH32V (QingKe RISC-V) build rules

# Toolchain: riscv-none-elf (xPack) or riscv64-unknown-elf GCC targeting rv32imac.
CROSS_PREFIX=riscv-none-elf-

dirs-y += src/ch32v src/generic lib/fast-hash

# CH32V103 = QingKe V3 (rv32imac + zicsr, no hardware float).
# CH32V203 = QingKe V4 (rv32imac + zicsr, optional single-precision FPU on V4F).
CFLAGS-$(CONFIG_MACH_CH32V103) += -march=rv32imac_zicsr -mabi=ilp32
CFLAGS-$(CONFIG_MACH_CH32V203) += -march=rv32imac_zicsr -mabi=ilp32
CFLAGS += $(CFLAGS-y) -mcmodel=medlow -msmall-data-limit=8
CFLAGS += --specs=nano.specs
CFLAGS += -Ilib/cmsis-core -Ilib/fast-hash

CFLAGS_klipper.elf += -nostdlib -lgcc -lc_nano
CFLAGS_klipper.elf += -T $(OUT)src/ch32v/ch32v_link.ld
$(OUT)klipper.elf: $(OUT)src/ch32v/ch32v_link.ld

# Core: RISC-V reset/interrupt/PFIC + generic command/stepper code.
src-y += ch32v/main.c ch32v/irq.c ch32v/timer.c ch32v/clock.c
src-y += ch32v/gpio.c
src-y += generic/crc16_ccitt.c generic/timer_irq.c

src-$(CONFIG_WANT_ADC) += ch32v/adc.c
src-$(CONFIG_SERIAL) += ch32v/serial.c generic/serial_irq.c

# Startup assembly (vector table + reset + BSS/data init).
OBJS_klipper.elf += $(OUT)src/ch32v/startup.o

# Binary output (raw image consumed by cmd_iap / mcu_iap_upgrade.sh).
target-y += $(OUT)klipper.bin

$(OUT)klipper.bin: $(OUT)klipper.elf
	@echo "  Creating bin file $@"
	$(Q)$(OBJCOPY) -O binary $< $@

$(OUT)src/ch32v/startup.o: src/ch32v/startup.S
	@echo "  Assembling $@"
	$(Q)$(CC) $(CFLAGS) -c $< -o $@
