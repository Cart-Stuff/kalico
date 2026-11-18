// Interrupt control for the Ingenic X2600 co-processor (standard RISC-V)
//
// Copyright (C) 2026
//
// This file may be distributed under the terms of the GNU GPLv3 license.

#include "board/irq.h"  // irqstatus_t

// Standard RISC-V machine-mode global interrupt enable is mstatus.MIE (bit 3).
#define MSTATUS_MIE (1u << 3)

void
irq_disable(void)
{
    __asm__ volatile("csrrc zero, mstatus, %0" :: "r"(MSTATUS_MIE) : "memory");
}

void
irq_enable(void)
{
    __asm__ volatile("csrrs zero, mstatus, %0" :: "r"(MSTATUS_MIE) : "memory");
}

irqstatus_t
irq_save(void)
{
    irqstatus_t old;
    __asm__ volatile("csrrc %0, mstatus, %1"
                     : "=r"(old) : "r"(MSTATUS_MIE) : "memory");
    return old & MSTATUS_MIE;
}

void
irq_restore(irqstatus_t flag)
{
    if (flag)
        irq_enable();
}

void
irq_wait(void)
{
    __asm__ volatile("wfi" ::: "memory");
}

void
irq_poll(void)
{
}
