// GPIO functions for the Ingenic X2600 co-processor
//
// Copyright (C) 2026
//
// This file may be distributed under the terms of the GNU GPLv3 license.
//
// Uses the Ingenic-standard GPIO register model (PXDAT/PXDATS/PXDATC/PXDIR/
// PXDIRS/PXDIRC). The co-processor shares the X2600 GPIO block with the host;
// only drive pins assigned to the real-time core in the printer's config.

#include <string.h>     // ffs
#include "board/irq.h"  // irq_save
#include "command.h"    // DECL_ENUMERATION_RANGE
#include "gpio.h"       // gpio_out_setup
#include "internal.h"   // pin encoding
#include "sched.h"      // sched_shutdown

// Ingenic GPIO port register block (per 32-pin port).
struct gpio_regs {
    volatile uint32_t DAT;     // 0x00 data
    volatile uint32_t DATS;    // 0x04 data set
    volatile uint32_t DATC;    // 0x08 data clear
    volatile uint32_t DIR;     // 0x0C direction (1 = input)
    volatile uint32_t DIRS;    // 0x10 direction set (input)
    volatile uint32_t DIRC;    // 0x14 direction clear (output)
    volatile uint32_t PEN;     // 0x18 pull enable
    volatile uint32_t PENS;    // 0x1C
    volatile uint32_t PENC;    // 0x20
};

// X2600 GPIO ports A..E (base 0x10010000, 0x1000 stride) - standard Ingenic map.
#define GPIO_PORT_BASE  0x10010000UL
#define GPIO_PORT_STRIDE 0x1000UL
#define GPIO_PORT(n) ((struct gpio_regs *)(GPIO_PORT_BASE + (n) * GPIO_PORT_STRIDE))

DECL_ENUMERATION_RANGE("pin", "PA0", GPIO('A', 0), 32);
DECL_ENUMERATION_RANGE("pin", "PB0", GPIO('B', 0), 32);
DECL_ENUMERATION_RANGE("pin", "PC0", GPIO('C', 0), 32);
DECL_ENUMERATION_RANGE("pin", "PD0", GPIO('D', 0), 32);
DECL_ENUMERATION_RANGE("pin", "PE0", GPIO('E', 0), 32);

static int
gpio_valid(uint32_t pin)
{
    return GPIO2PORT(pin) < 5;
}

void
gpio_peripheral(uint32_t pin, uint32_t mode, int32_t pull_up)
{
    struct gpio_regs *regs = GPIO_PORT(GPIO2PORT(pin));
    uint32_t bit = GPIO2BIT(pin);
    irqstatus_t flag = irq_save();
    if (mode == GPIO_OUTPUT) {
        regs->DIRC = bit; // output
    } else {
        regs->DIRS = bit; // input
    }
    irq_restore(flag);
}

struct gpio_out
gpio_out_setup(uint32_t pin, uint32_t val)
{
    if (!gpio_valid(pin))
        shutdown("Not an output pin");
    struct gpio_out g = { .regs = GPIO_PORT(GPIO2PORT(pin)),
                          .bit = GPIO2BIT(pin) };
    gpio_out_reset(g, val);
    return g;
}

void
gpio_out_reset(struct gpio_out g, uint32_t val)
{
    struct gpio_regs *regs = g.regs;
    irqstatus_t flag = irq_save();
    if (val)
        regs->DATS = g.bit;
    else
        regs->DATC = g.bit;
    regs->DIRC = g.bit; // ensure output
    irq_restore(flag);
}

void
gpio_out_toggle_noirq(struct gpio_out g)
{
    struct gpio_regs *regs = g.regs;
    regs->DAT ^= g.bit;
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
        regs->DATS = g.bit;
    else
        regs->DATC = g.bit;
}

struct gpio_in
gpio_in_setup(uint32_t pin, int32_t pull_up)
{
    if (!gpio_valid(pin))
        shutdown("Not a valid input pin");
    struct gpio_in g = { .regs = GPIO_PORT(GPIO2PORT(pin)),
                         .bit = GPIO2BIT(pin) };
    gpio_in_reset(g, pull_up);
    return g;
}

void
gpio_in_reset(struct gpio_in g, int32_t pull_up)
{
    struct gpio_regs *regs = g.regs;
    irqstatus_t flag = irq_save();
    regs->DIRS = g.bit; // input
    irq_restore(flag);
}

uint8_t
gpio_in_read(struct gpio_in g)
{
    struct gpio_regs *regs = g.regs;
    return !!(regs->DAT & g.bit);
}
