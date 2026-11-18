// ADC functions for the Ingenic X2600 co-processor (SAR ADC)
//
// Copyright (C) 2026
//
// This file may be distributed under the terms of the GNU GPLv3 license.

#include "board/irq.h"   // irq_save
#include "board/misc.h"  // timer_from_us
#include "command.h"     // shutdown
#include "compiler.h"    // ARRAY_SIZE
#include "gpio.h"        // gpio_adc_setup
#include "internal.h"    // GPIO
#include "sched.h"       // sched_shutdown

DECL_CONSTANT("ADC_MAX", 4095);

// X2600 SAR ADC: 16 channels, 12-bit, Ingenic-standard register model.
// Base address is the Ingenic SAR ADC block (host-visible; only channels
// assigned to the real-time core should be used).
struct sadc_regs {
    volatile uint32_t CTRL;   // 0x00 control (enable, start)
    volatile uint32_t CFG;    // 0x04 config (channel, mode)
    volatile uint32_t STATUS; // 0x08 status (EOC)
    volatile uint32_t DATA;   // 0x0C conversion result
};
#define SADC ((struct sadc_regs *)0x10070000UL)
#define SADC_CTRL_EN    (1u << 0)
#define SADC_CTRL_START (1u << 1)
#define SADC_STATUS_EOC (1u << 0)

// ADC channel -> pin map. On the K1, the thermistor inputs are the SAR ADC
// channels wired to the analog pads (AD0..AD15 on PB12..PB27).
static const uint16_t adc_pins[] = {
    GPIO('B', 12), GPIO('B', 13), GPIO('B', 14), GPIO('B', 15),
    GPIO('B', 16), GPIO('B', 17), GPIO('B', 18), GPIO('B', 19),
    GPIO('B', 20), GPIO('B', 21), GPIO('B', 22), GPIO('B', 23),
    GPIO('B', 24), GPIO('B', 25), GPIO('B', 26), GPIO('B', 27),
};

struct gpio_adc
gpio_adc_setup(uint32_t pin)
{
    int chan;
    for (chan = 0; ; chan++) {
        if (chan >= ARRAY_SIZE(adc_pins))
            shutdown("Not a valid ADC pin");
        if (adc_pins[chan] == pin)
            break;
    }
    SADC->CTRL = SADC_CTRL_EN;
    return (struct gpio_adc){ .adc = SADC, .chan = chan };
}

uint32_t
gpio_adc_sample(struct gpio_adc g)
{
    struct sadc_regs *adc = g.adc;
    if (adc->STATUS & SADC_STATUS_EOC)
        return 0;
    adc->CFG = g.chan;
    adc->CTRL = SADC_CTRL_EN | SADC_CTRL_START;
    return timer_from_us(20);
}

uint16_t
gpio_adc_read(struct gpio_adc g)
{
    struct sadc_regs *adc = g.adc;
    adc->STATUS = 0;
    return adc->DATA & 0xFFF;
}

void
gpio_adc_cancel_sample(struct gpio_adc g)
{
    struct sadc_regs *adc = g.adc;
    irqstatus_t flag = irq_save();
    if (!(adc->STATUS & SADC_STATUS_EOC))
        adc->STATUS = 0;
    irq_restore(flag);
}
