// Interrupt control for WCH CH32V (QingKe RISC-V PFIC)
//
// Copyright (C) 2026
//
// This file may be distributed under the terms of the GNU GPLv3 license.

#include "board/irq.h"  // irqstatus_t
#include "internal.h"   // CSR_GINTENR

void
irq_disable(void)
{
    __asm__ volatile("csrrc zero, %0, %1"
                     :: "i"(CSR_GINTENR), "r"(CH32V_GIE_BIT) : "memory");
}

void
irq_enable(void)
{
    __asm__ volatile("csrrs zero, %0, %1"
                     :: "i"(CSR_GINTENR), "r"(CH32V_GIE_BIT) : "memory");
}

irqstatus_t
irq_save(void)
{
    irqstatus_t old;
    __asm__ volatile("csrrc %0, %1, %2"
                     : "=r"(old) : "i"(CSR_GINTENR), "r"(CH32V_GIE_BIT)
                     : "memory");
    return old & CH32V_GIE_BIT;
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
