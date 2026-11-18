// Serial console for WCH CH32V (USART, STM32F1-compatible)
//
// Copyright (C) 2026
//
// This file may be distributed under the terms of the GNU GPLv3 license.

#include "autoconf.h"          // CONFIG_SERIAL_BAUD
#include "board/serial_irq.h"  // serial_rx_byte
#include "command.h"           // DECL_CONSTANT_STR
#include "compiler.h"          // __visible
#include "gpio.h"              // gpio_peripheral, GPIO_FUNCTION
#include "internal.h"          // USART registers, RCC, gpio
#include "sched.h"             // DECL_INIT

// The K1 nozzle/bed harness uses USART1 on PA9 (TX) / PA10 (RX).
DECL_CONSTANT_STR("RESERVE_PINS_serial", "PA10,PA9");
#define GPIO_Rx      GPIO('A', 10)
#define GPIO_Tx      GPIO('A', 9)
#define USARTx       USART(USART1_BASE)
#define USARTx_IRQn  37  // CH32V USART1 global interrupt number (PFIC)

#define USART_SR_ORE    (1u << 3)
#define USART_CR1_TXEIE (1u << 7)
#define CR1_FLAGS (USART_CR1_UE | USART_CR1_RE | USART_CR1_TE | USART_CR1_RXNEIE)

__visible void
USART1_IRQHandler(void)
{
    uint32_t sr = USARTx->SR;
    if (sr & (USART_SR_RXNE | USART_SR_ORE)) {
        // Reading SR then DR clears RXNE and ORE.
        serial_rx_byte(USARTx->DR);
    }
    if ((sr & USART_SR_TXE) && (USARTx->CR1 & USART_CR1_TXEIE)) {
        uint8_t data;
        int ret = serial_get_tx_byte(&data);
        if (ret)
            USARTx->CR1 = CR1_FLAGS;
        else
            USARTx->DR = data;
    }
}

void
serial_enable_tx_irq(void)
{
    USARTx->CR1 = CR1_FLAGS | USART_CR1_TXEIE;
}

void
serial_init(void)
{
    // Enable USART1 (APB2 bit 14) and GPIOA (APB2 bit 2) clocks.
    RCC_APB2PCENR |= (1u << 14) | (1u << 2);

    // USART1 is on APB2 (PCLK2 = full system clock here).
    uint32_t pclk = CONFIG_CLOCK_FREQ;
    uint32_t div = (pclk + CONFIG_SERIAL_BAUD / 2) / CONFIG_SERIAL_BAUD;
    // BRR = mantissa<<4 | fraction, where div is in 16ths.
    USARTx->BRR = ((div / 16) << 4) | (div % 16);
    USARTx->CR1 = CR1_FLAGS;

    extern void ch32v_pfic_enable(uint8_t irqn, uint8_t prio);
    ch32v_pfic_enable(USARTx_IRQn, 0);

    // PA9 = TX (alternate-function push-pull), PA10 = RX (input pull-up).
    gpio_peripheral(GPIO_Tx, GPIO_FUNCTION, 0);
    gpio_peripheral(GPIO_Rx, GPIO_INPUT, 1);
}
DECL_INIT(serial_init);
