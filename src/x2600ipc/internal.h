#ifndef __X2600IPC_INTERNAL_H
#define __X2600IPC_INTERNAL_H
// Ingenic X2600 co-processor: IPC registers, TPEL descriptor, ring buffers.
//
// Layout recovered from the stock klipper_main_ipc.bin and soc_mcu.ko:
//   - The firmware image begins with a TPEL boot header at 0x0FF20000.
//   - The bootloader points register 0xB2A00004 at this header.
//   - header+0x18 = command-ring descriptor ptr (host -> MCU)
//   - header+0x1C = response-ring descriptor ptr (MCU -> host)
//   - Each ring descriptor: { data_ptr, size, w_pos, r_pos } (head/tail ring).
//   - Doorbell: MCU sets bit 8 of 0x13650014 to notify the host; the host
//     writes 1 to 0xB2A00008 to notify the MCU (cleared by the IRQ handler).

#include <stdint.h>

// --- TPEL boot header (placed at the very start of the image) ---
#define TPEL_MAGIC      0x4C455054u   // "TPEL" little-endian

// --- IPC / doorbell registers (X2600 co-processor IPC block) ---
#define IPC_STATUS      (*(volatile uint32_t *)0x13650000UL)
#define IPC_DOORBELL    (*(volatile uint32_t *)0x13650014UL)
#define IPC_DB_HOST_BIT (1u << 8)     // MCU -> host notification

// Host->MCU doorbell (written by the host, read/acked by the MCU).
#define HOST_MSG_REG    (*(volatile uint32_t *)0xB2A00008UL)
#define HOST_MSG_ACK    (*(volatile uint32_t *)0xB2A0000CUL)
#define HOST_HEADER_REG (*(volatile uint32_t *)0xB2A00004UL)

// --- Ring descriptor (shared-memory circular buffer) ---
struct ipc_ring {
    volatile uint8_t  *data;   // +0x00 ring data buffer (virtual/physical)
    volatile uint32_t  size;   // +0x04 capacity in bytes
    volatile uint32_t  w_pos;  // +0x08 producer index
    volatile uint32_t  r_pos;  // +0x0C consumer index
};

// TPEL header as published at the start of the firmware image.
struct tpel_header {
    uint32_t reset_insn;       // +0x00 jal to ResetHandler
    uint32_t magic;            // +0x04 TPEL_MAGIC
    uint32_t rsv[4];           // +0x08 .. +0x14
    struct ipc_ring *cmd_ring; // +0x18 host -> MCU
    struct ipc_ring *resp_ring;// +0x1C MCU -> host
    uint32_t data_base;        // +0x20 shared data region base
    uint32_t data_size;        // +0x24
};

// Co-processor global interrupt enable CSR (Ingenic RISC-V uses mstatus.MIE).
void clock_init(void);

// Pin-number encoding (matches the generic Klipper convention).
#define GPIO(PORT, NUM) (((PORT)-'A') * 32 + (NUM))
#define GPIO2PORT(PIN) ((PIN) / 32)
#define GPIO2BIT(PIN) (1u<<((PIN) % 32))

#endif // __X2600IPC_INTERNAL_H
