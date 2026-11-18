// Timer support for WCH CH32V (general-purpose TIM, STM32F1-compatible)
//
// Copyright (C) 2026
//
// This file may be distributed under the terms of the GNU GPLv3 license.

#include "board/io.h"        // readl
#include "board/irq.h"       // irq_disable
#include "board/misc.h"      // timer_read_time
#include "board/timer_irq.h" // timer_dispatch_many
#include "command.h"         // DECL_SHUTDOWN
#include "compiler.h"        // __visible
#include "internal.h"        // TIM2, RCC_*
#include "sched.h"           // DECL_INIT

// CH32V TIM2 is a 16-bit up-counter at 0x40000000. We extend it to 32 bits in
// software (same approach as the STM32 16-bit timer ports).
#define TIMx            TIM2
#define TIMx_IRQn       28  // CH32V TIM2 global interrupt number (PFIC)

#define TIM_DIER_CC1IE  (1u << 1)
#define TIM_CCER_CC1E   (1u << 0)
#define TIM_CR1_CEN     (1u << 0)

static inline uint32_t
timer_get(void)
{
    return TIMx->CNT;
}

static inline void
timer_set(uint32_t next)
{
    TIMx->CCR1 = next;
    TIMx->SR = 0;
}

void
timer_kick(void)
{
    timer_set(timer_get() + 50);
}

// High 17 bits of the 32-bit software-extended counter.
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

// PFIC interrupt handler for TIM2. The startup vector table routes the TIM2
// global interrupt here.
__visible void
TIM2_IRQHandler(void)
{
    irq_disable();
    uint32_t next = timer_dispatch_many();
    timer_set(next);
    irq_enable();
}

void
timer_init(void)
{
    irqstatus_t flag = irq_save();
    // Enable TIM2 clock on APB1.
    RCC_APB1PCENR |= (1u << 0); // TIM2EN
    TIMx->CNT = 0;
    // Prescaler: run the counter at the full peripheral clock / 1.
    TIMx->PSC = 0;
    TIMx->ARR = 0xffff;
    TIMx->DIER = TIM_DIER_CC1IE;
    TIMx->CCER = TIM_CCER_CC1E;
    // PFIC enable for TIM2 (handled in startup/irq glue).
    extern void ch32v_pfic_enable(uint8_t irqn, uint8_t prio);
    ch32v_pfic_enable(TIMx_IRQn, 2);
    timer_kick();
    timer_reset();
    TIMx->CR1 = TIM_CR1_CEN;
    irq_restore(flag);
}
DECL_INIT(timer_init);
