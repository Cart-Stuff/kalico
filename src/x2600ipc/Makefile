# Build rules for the Ingenic X2600 co-processor (RISC-V, IPC transport)

CROSS_PREFIX=riscv-none-elf-

dirs-y += src/x2600ipc src/generic lib/fast-hash

CFLAGS += -march=rv32imac_zicsr -mabi=ilp32
CFLAGS += -mcmodel=medany -msmall-data-limit=0
CFLAGS += --specs=nano.specs
CFLAGS += -Ilib/cmsis-core -Ilib/fast-hash

CFLAGS_klipper.elf += -nostdlib -lgcc -lc_nano
CFLAGS_klipper.elf += -T $(OUT)src/x2600ipc/x2600ipc_link.ld
$(OUT)klipper.elf: $(OUT)src/x2600ipc/x2600ipc_link.ld

# Core: reset/irq + timer + IPC console + Klipper generic code.
src-y += x2600ipc/main.c x2600ipc/irq.c x2600ipc/timer.c
src-y += x2600ipc/ipc.c x2600ipc/console.c
src-y += x2600ipc/gpio.c
src-y += generic/crc16_ccitt.c generic/timer_irq.c

src-$(CONFIG_WANT_ADC) += x2600ipc/adc.c

OBJS_klipper.elf += $(OUT)src/x2600ipc/startup.o

target-y += $(OUT)klipper.bin

$(OUT)klipper.bin: $(OUT)klipper.elf
	@echo "  Creating bin file $@"
	$(Q)$(OBJCOPY) -O binary $< $@

$(OUT)src/x2600ipc/startup.o: src/x2600ipc/startup.S
	@echo "  Assembling $@"
	$(Q)$(CC) $(CFLAGS) -c $< -o $@
