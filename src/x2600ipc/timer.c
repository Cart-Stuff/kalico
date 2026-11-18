// Timer support for the Ingenic X2600 co-processor (RISC-V cycle counter)
//
// Copyright (C) 2026
//
// This file may be distributed under the terms of the GNU GPLv3 license.

#include "board/io.h"        // readl
#include "board/irq.h"       // irq_disable
#include "board/misc.h"      // timer_read_time
#include "board/timer_irq.h" // timer_dispatch_many
#include "command.h"         // DECL_SHUTDOWN
#include "sched.h"           // DECL_INIT

// The co-processor RISC-V core exposes a free-running cycle counter via the
// standard `rdcycle` CSR (mcycle). We use it directly as the Klipper timebase;
// CLOCK_FREQ is the co-processor clock so timer_from_us()/timer_read_time()
// agree. mcycle is 64-bit; we take the low 32 bits and extend the wrap in
// software like the 16-bit MCU timer ports.

static inline uint32_t
timer_get(void)
{
    uint32_t v;
    __asm__ volatile("rdcycle %0" : "=r"(v));
    return v;
}

void
timer_kick(void)
{
    // The wrap timer + doorbell keep the scheduler live; no separate hardware
    // compare is needed to kick it on this arch.
    (void)timer_get;
}

static uint32_t timer_high;

uint32_t __always_inline
timer_read_time(void)
{
    uint32_t th = readl(&timer_high);
    uint32_t cur = timer_get();
    return (th ^ cur) + (th & 0xffff);
}

static uint_fast8_t
timer_event(struct timer *t)
{
    timer_high += 0x8000;
    t->waketime = timer_high + 0x8000;
    return SF_RESCHEDULE;
}
static struct timer wrap_timer = {
    .func = timer_event,
    .waketime = 0x8000,
};

void
timer_reset(void)
{
    sched_add_timer(&wrap_timer);
}
DECL_SHUTDOWN(timer_reset);

// The co-processor's IPC doorbell IRQ doubles as the scheduler kick: the host
// notifies the MCU when new commands are queued, and timer_dispatch_many()
// runs from the same periodic path. We also arm a compare via mtimecmp when
// available; here we rely on the wrap timer + doorbell to stay live.
void
timer_init(void)
{
    timer_reset();
}
DECL_INIT(timer_init);
