// Main entry point for WCH CH32V (QingKe RISC-V)
//
// Copyright (C) 2026
//
// This file may be distributed under the terms of the GNU GPLv3 license.

#include <stdint.h>
#include "autoconf.h"     // CONFIG_MCU
#include "board/misc.h"   // dynmem_start
#include "command.h"      // DECL_CONSTANT_STR
#include "compiler.h"     // __visible
#include "internal.h"     // clock_init
#include "sched.h"        // sched_main

DECL_CONSTANT_STR("MCU", CONFIG_MCU);

// Dynamic memory pool for Klipper's alloc_chunk() (command configs, etc).
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

// The vector table and reset code live in startup.S; ResetHandler calls
// ch32v_main() after initialising .data and .bss.

__visible void
ch32v_main(void)
{
    // Configure RCC for the target clock (HSE -> PLL).
    clock_init();

    // Console (USART) + scheduler take over from here.
    sched_main();

    for (;;)
        irq_wait();
}
