// GPIO functions for WCH CH32V (STM32F1-compatible GPIO)
//
// Copyright (C) 2026
//
// This file may be distributed under the terms of the GNU GPLv3 license.

#include <string.h>     // ffs
#include "board/irq.h"  // irq_save
#include "command.h"    // DECL_ENUMERATION_RANGE
#include "gpio.h"       // gpio_out_setup
#include "internal.h"   // GPIO registers, RCC
#include "sched.h"      // sched_shutdown

DECL_ENUMERATION_RANGE("pin", "PA0", GPIO('A', 0), 16);
DECL_ENUMERATION_RANGE("pin", "PB0", GPIO('B', 0), 16);
DECL_ENUMERATION_RANGE("pin", "PC0", GPIO('C', 0), 16);
DECL_ENUMERATION_RANGE("pin", "PD0", GPIO('D', 0), 16);

struct gpio_regs * const digital_regs[] = {
    ['A' - 'A'] = GPIO_REGS(GPIOA_BASE),
    ['B' - 'A'] = GPIO_REGS(GPIOB_BASE),
    ['C' - 'A'] = GPIO_REGS(GPIOC_BASE),
    ['D' - 'A'] = GPIO_REGS(GPIOD_BASE),
};

// APB2 peripheral clock enable bits for each GPIO port.
static const uint32_t gpio_pcken[] = {
    ['A' - 'A'] = (1u << 2), // IOPAEN
    ['B' - 'A'] = (1u << 3), // IOPBEN
    ['C' - 'A'] = (1u << 4), // IOPCEN
    ['D' - 'A'] = (1u << 5), // IOPDEN
};

static int
regs_to_pin(struct gpio_regs *regs, uint32_t bit)
{
    int i;
    for (i = 0; i < ARRAY_SIZE(digital_regs); i++)
        if (digital_regs[i] == regs)
            return GPIO('A' + i, ffs(bit) - 1);
    return 0;
}

static int
gpio_valid(uint32_t pin)
{
    uint32_t port = GPIO2PORT(pin);
    return port < ARRAY_SIZE(digital_regs) && digital_regs[port];
}

static void
gpio_clock_enable(uint32_t port)
{
    RCC_APB2PCENR |= gpio_pcken[port];
}

// Program the CRL/CRH nibble for a pin.
static void
gpio_config(uint32_t pin, uint32_t mode_cnf)
{
    uint32_t port = GPIO2PORT(pin);
    struct gpio_regs *regs = digital_regs[port];
    uint32_t bitnum = ffs(GPIO2BIT(pin)) - 1;
    volatile uint32_t *cr = (bitnum < 8) ? &regs->CRL : &regs->CRH;
    uint32_t shift = (bitnum & 7) * 4;
    irqstatus_t flag = irq_save();
    *cr = (*cr & ~(0xFu << shift)) | (mode_cnf << shift);
    irq_restore(flag);
}

void
gpio_peripheral(uint32_t pin, uint32_t mode, int32_t pull_up)
{
    uint32_t port = GPIO2PORT(pin);
    gpio_clock_enable(port);
    uint32_t cfg;
    if (mode == GPIO_OUTPUT) {
        cfg = GPIO_MODE_OUT_50MHZ | GPIO_CNF_OUT_PP;
    } else if (mode == GPIO_ANALOG) {
        cfg = GPIO_MODE_INPUT | GPIO_CNF_IN_ANALOG;
    } else if (mode == GPIO_INPUT) {
        if (pull_up) {
            cfg = GPIO_MODE_INPUT | GPIO_CNF_IN_PULL;
            // ODR selects pull-up (1) vs pull-down (0).
            struct gpio_regs *regs = digital_regs[port];
            if (pull_up > 0)
                regs->BSRR = GPIO2BIT(pin);
            else
                regs->BSRR = GPIO2BIT(pin) << 16;
        } else {
            cfg = GPIO_MODE_INPUT | GPIO_CNF_IN_FLOAT;
        }
    } else { // GPIO_FUNCTION / alternate function
        cfg = GPIO_MODE_OUT_50MHZ | GPIO_CNF_OUT_AF_PP;
    }
    gpio_config(pin, cfg);
}

struct gpio_out
gpio_out_setup(uint32_t pin, uint32_t val)
{
    if (!gpio_valid(pin))
        shutdown("Not an output pin");
    struct gpio_regs *regs = digital_regs[GPIO2PORT(pin)];
    gpio_clock_enable(GPIO2PORT(pin));
    struct gpio_out g = { .regs = regs, .bit = GPIO2BIT(pin) };
    gpio_out_reset(g, val);
    return g;
}

void
gpio_out_reset(struct gpio_out g, uint32_t val)
{
    struct gpio_regs *regs = g.regs;
    int pin = regs_to_pin(regs, g.bit);
    irqstatus_t flag = irq_save();
    if (val)
        regs->BSRR = g.bit;
    else
        regs->BSRR = g.bit << 16;
    gpio_peripheral(pin, GPIO_OUTPUT, 0);
    irq_restore(flag);
}

void
gpio_out_toggle_noirq(struct gpio_out g)
{
    struct gpio_regs *regs = g.regs;
    regs->ODR ^= g.bit;
}

void
gpio_out_toggle(struct gpio_out g)
{
    irqstatus_t flag = irq_save();
    gpio_out_toggle_noirq(g);
    irq_restore(flag);
}

void
gpio_out_write(struct gpio_out g, uint32_t val)
{
    struct gpio_regs *regs = g.regs;
    if (val)
        regs->BSRR = g.bit;
    else
        regs->BSRR = g.bit << 16;
}

struct gpio_in
gpio_in_setup(uint32_t pin, int32_t pull_up)
{
    if (!gpio_valid(pin))
        shutdown("Not a valid input pin");
    struct gpio_regs *regs = digital_regs[GPIO2PORT(pin)];
    struct gpio_in g = { .regs = regs, .bit = GPIO2BIT(pin) };
    gpio_in_reset(g, pull_up);
    return g;
}

void
gpio_in_reset(struct gpio_in g, int32_t pull_up)
{
    int pin = regs_to_pin(g.regs, g.bit);
    irqstatus_t flag = irq_save();
    gpio_peripheral(pin, GPIO_INPUT, pull_up);
    irq_restore(flag);
}

uint8_t
gpio_in_read(struct gpio_in g)
{
    struct gpio_regs *regs = g.regs;
    return !!(regs->IDR & g.bit);
}
