// Main entry point for the Ingenic X2600 co-processor
//
// Copyright (C) 2026
//
// This file may be distributed under the terms of the GNU GPLv3 license.

#include <stdint.h>
#include "autoconf.h"     // CONFIG_MCU
#include "board/misc.h"   // dynmem_start
#include "command.h"      // DECL_CONSTANT_STR
#include "compiler.h"     // __visible
#include "internal.h"     // HOST_MSG_REG, HOST_MSG_ACK
#include "sched.h"        // sched_main

DECL_CONSTANT_STR("MCU", CONFIG_MCU);

static char dynmem_pool[8 * 1024];

void *
dynmem_start(void)
{
    return dynmem_pool;
}

void *
dynmem_end(void)
{
    return &dynmem_pool[sizeof(dynmem_pool)];
}

extern void console_setup(void);
extern void console_notify(void);

// The host's doorbell (write of 1 to 0xB2A00008) raises the co-processor IRQ.
// The startup mtvec points at DefaultHandler; route the IPC interrupt here.
__visible void
ipc_irq_handler(void)
{
    // Acknowledge the host->MCU doorbell and wake the console to drain the
    // command ring.
    HOST_MSG_ACK = 0;
    console_notify();
}

__visible void
x2600_main(void)
{
    console_setup();
    sched_main();
    for (;;)
        irq_wait();
}
