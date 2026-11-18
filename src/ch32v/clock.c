// Clock setup for WCH CH32V (RCC, STM32F1-compatible)
//
// Copyright (C) 2026
//
// This file may be distributed under the terms of the GNU GPLv3 license.

#include "internal.h"   // RCC_*

void
clock_init(void)
{
    // Start from a known state: HSI on, system clock = HSI.
    RCC_CTLR |= RCC_CTLR_HSION;
    RCC_CFGR0 = RCC_CFGR0_SW_HSI;

    // Enable the external 8 MHz crystal and wait for it.
    RCC_CTLR |= RCC_CTLR_HSEON;
    while (!(RCC_CTLR & RCC_CTLR_HSERDY))
        ;

#if CONFIG_MACH_CH32V203
    // 8 MHz HSE * 18 = 144 MHz (PLLMULL field value 15 => x18 on V203).
    // APB1 must be <= 72 MHz (div2), APB2 full rate, ADC /12.
    RCC_CFGR0 = RCC_CFGR0_PLLSRC_HSE
              | RCC_CFGR0_PLLMULL(15)
              | RCC_CFGR0_HPRE_DIV1
              | RCC_CFGR0_PPRE1_DIV2
              | RCC_CFGR0_PPRE2_DIV1
              | RCC_CFGR0_ADCPRE_DIV6;
#else
    // CH32V103: 8 MHz HSE * 9 = 72 MHz (PLLMULL field value 7 => x9).
    RCC_CFGR0 = RCC_CFGR0_PLLSRC_HSE
              | RCC_CFGR0_PLLMULL(7)
              | RCC_CFGR0_HPRE_DIV1
              | RCC_CFGR0_PPRE1_DIV2
              | RCC_CFGR0_PPRE2_DIV1
              | RCC_CFGR0_ADCPRE_DIV6;
#endif

    // Flash latency: one wait-state per ~24-48MHz step. Use the WCH
    // flash prefetch/latency register when running above 48 MHz.
    // (CH32V flash ACTLR at 0x40022000.)
    *(volatile uint32_t *)0x40022000UL = 0x02; // 2 wait states + prefetch

    // Enable PLL and wait for lock.
    RCC_CTLR |= RCC_CTLR_PLLON;
    while (!(RCC_CTLR & RCC_CTLR_PLLRDY))
        ;

    // Switch system clock to PLL.
    RCC_CFGR0 = (RCC_CFGR0 & ~0x3u) | RCC_CFGR0_SW_PLL;
    while ((RCC_CFGR0 & RCC_CFGR0_SWS_MASK) != RCC_CFGR0_SWS_PLL)
        ;
}
