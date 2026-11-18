// ADC functions for WCH CH32V (ADC1, STM32F1-compatible)
//
// Copyright (C) 2026
//
// This file may be distributed under the terms of the GNU GPLv3 license.

#include "board/irq.h"   // irq_save
#include "board/misc.h"  // timer_from_us
#include "command.h"     // shutdown
#include "compiler.h"    // ARRAY_SIZE
#include "gpio.h"        // gpio_adc_setup
#include "internal.h"    // GPIO, ADC1_BASE, RCC
#include "sched.h"       // sched_shutdown

DECL_CONSTANT("ADC_MAX", 4095);

#define ADC_TEMPERATURE_PIN 0xfe
DECL_ENUMERATION("pin", "ADC_TEMPERATURE", ADC_TEMPERATURE_PIN);

// ADC1 register block (STM32F1 layout).
struct adc_regs {
    volatile uint32_t SR;    // 0x00 status
    volatile uint32_t CR1;   // 0x04 control 1
    volatile uint32_t CR2;   // 0x08 control 2
    volatile uint32_t SMPR1; // 0x0C sample time 1
    volatile uint32_t SMPR2; // 0x10 sample time 2
    volatile uint32_t JOFR[4];
    volatile uint32_t HTR;
    volatile uint32_t LTR;
    volatile uint32_t SQR1;  // 0x28 regular sequence 1
    volatile uint32_t SQR2;  // 0x2C
    volatile uint32_t SQR3;  // 0x30
    volatile uint32_t JSQR;
    volatile uint32_t JDR[4];
    volatile uint32_t DR;    // 0x4C regular data
};

#define ADC_SR_STRT     (1u << 4)
#define ADC_SR_EOC      (1u << 1)
#define ADC_CR2_ADON    (1u << 0)
#define ADC_CR2_CAL     (1u << 2)
#define ADC_CR2_RSTCAL  (1u << 3)
#define ADC_CR2_SWSTART (1u << 22)
#define ADC_CR2_EXTTRIG (1u << 20)
#define ADC_CR2_TSVREFE (1u << 23)
// (7 << 17) selects SWSTART as the external trigger source.
#define CR2_FLAGS (ADC_CR2_ADON | (7u << 17) | ADC_CR2_EXTTRIG | ADC_CR2_TSVREFE)

// ADC channel -> GPIO pin map (ADC1 channels 0..15, 16 = internal temp).
static const uint16_t adc_pins[] = {
    GPIO('A', 0), GPIO('A', 1), GPIO('A', 2), GPIO('A', 3),
    GPIO('A', 4), GPIO('A', 5), GPIO('A', 6), GPIO('A', 7),
    GPIO('B', 0), GPIO('B', 1), GPIO('C', 0), GPIO('C', 1),
    GPIO('C', 2), GPIO('C', 3), GPIO('C', 4), GPIO('C', 5),
    ADC_TEMPERATURE_PIN,
};

static void
adc_calibrate(struct adc_regs *adc)
{
    adc->CR2 = ADC_CR2_ADON;
    // ~10us settle
    for (volatile int i = 0; i < 1000; i++)
        ;
    adc->CR2 = ADC_CR2_ADON | ADC_CR2_RSTCAL;
    while (adc->CR2 & ADC_CR2_RSTCAL)
        ;
    adc->CR2 = ADC_CR2_ADON | ADC_CR2_CAL;
    while (adc->CR2 & ADC_CR2_CAL)
        ;
}

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

    struct adc_regs *adc = (struct adc_regs *)ADC1_BASE;
    static int adc_enabled;
    if (!adc_enabled) {
        // ADC1 clock = APB2 bit 9.
        RCC_APB2PCENR |= (1u << 9);
        adc_calibrate(adc);
        // ~41.5-cycle sample time (field value 4) on every channel.
        uint32_t aticks = 4;
        adc->SMPR1 = (aticks | (aticks << 3) | (aticks << 6) | (aticks << 9)
                      | (aticks << 12) | (aticks << 15) | (aticks << 18)
                      | (aticks << 21));
        adc->SMPR2 = (aticks | (aticks << 3) | (aticks << 6) | (aticks << 9)
                      | (aticks << 12) | (aticks << 15) | (aticks << 18)
                      | (aticks << 21) | (aticks << 24) | (aticks << 27));
        adc->CR2 = CR2_FLAGS;
        adc_enabled = 1;
    }

    if (pin != ADC_TEMPERATURE_PIN)
        gpio_peripheral(pin, GPIO_ANALOG, 0);

    return (struct gpio_adc){ .adc = adc, .chan = chan };
}

uint32_t
gpio_adc_sample(struct gpio_adc g)
{
    struct adc_regs *adc = g.adc;
    uint32_t sr = adc->SR;
    if (sr & ADC_SR_STRT) {
        if (!(sr & ADC_SR_EOC) || adc->SQR3 != g.chan)
            goto need_delay;
        return 0;
    }
    adc->SQR3 = g.chan;
    adc->CR2 = ADC_CR2_SWSTART | CR2_FLAGS;

need_delay:
    return timer_from_us(20);
}

uint16_t
gpio_adc_read(struct gpio_adc g)
{
    struct adc_regs *adc = g.adc;
    adc->SR = ~ADC_SR_STRT;
    return adc->DR;
}

void
gpio_adc_cancel_sample(struct gpio_adc g)
{
    struct adc_regs *adc = g.adc;
    irqstatus_t flag = irq_save();
    if ((adc->SR & ADC_SR_STRT) && adc->SQR3 == g.chan)
        gpio_adc_read(g);
    irq_restore(flag);
}
