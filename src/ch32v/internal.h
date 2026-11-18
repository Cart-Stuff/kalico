#ifndef __CH32V_INTERNAL_H
#define __CH32V_INTERNAL_H
// WCH CH32V (QingKe RISC-V) register map and core helpers.
//
// Peripheral registers are STM32F1-compatible (RCC, GPIO, AFIO, USART, TIM,
// ADC). The CPU core is QingKe RISC-V: interrupt control is via the PFIC and
// custom CSRs (0x800 global enable), NOT the ARM NVIC.

#include <stdint.h>

// ---------------------------------------------------------------------------
// RCC (reset & clock control) - 0x40021000 (STM32F1 layout)
// ---------------------------------------------------------------------------
#define RCC_BASE        0x40021000UL
#define RCC_CTLR        (*(volatile uint32_t *)(RCC_BASE + 0x00))
#define RCC_CFGR0       (*(volatile uint32_t *)(RCC_BASE + 0x04))
#define RCC_INTR        (*(volatile uint32_t *)(RCC_BASE + 0x08))
#define RCC_APB2PRSTR   (*(volatile uint32_t *)(RCC_BASE + 0x0C))
#define RCC_APB1PRSTR   (*(volatile uint32_t *)(RCC_BASE + 0x10))
#define RCC_AHBPCENR    (*(volatile uint32_t *)(RCC_BASE + 0x14))
#define RCC_APB2PCENR   (*(volatile uint32_t *)(RCC_BASE + 0x18))
#define RCC_APB1PCENR   (*(volatile uint32_t *)(RCC_BASE + 0x1C))
#define RCC_RSTSCKR     (*(volatile uint32_t *)(RCC_BASE + 0x24))

// RCC_CTLR bits
#define RCC_CTLR_HSION      (1u << 0)
#define RCC_CTLR_HSIRDY     (1u << 1)
#define RCC_CTLR_HSEON      (1u << 16)
#define RCC_CTLR_HSERDY     (1u << 17)
#define RCC_CTLR_HSEBYP     (1u << 18)
#define RCC_CTLR_PLLON      (1u << 24)
#define RCC_CTLR_PLLRDY     (1u << 25)
// RCC_CFGR0 bits
#define RCC_CFGR0_SW_HSI    (0u << 0)
#define RCC_CFGR0_SW_HSE    (1u << 0)
#define RCC_CFGR0_SW_PLL    (2u << 0)
#define RCC_CFGR0_SWS_MASK  (3u << 2)
#define RCC_CFGR0_SWS_PLL   (2u << 2)
#define RCC_CFGR0_HPRE_DIV1 (0u << 4)
#define RCC_CFGR0_PPRE1_DIV2 (4u << 8)
#define RCC_CFGR0_PPRE2_DIV1 (0u << 11)
#define RCC_CFGR0_ADCPRE_DIV6 (2u << 14)
#define RCC_CFGR0_PLLSRC_HSE (1u << 16)
#define RCC_CFGR0_PLLXTPRE  (1u << 17)
// PLL multiplier field (bits 18..21): 0=x2 ... 7=x9 ... 13=x18 (V203)
#define RCC_CFGR0_PLLMULL(n) (((n) & 0xF) << 18)

// ---------------------------------------------------------------------------
// GPIO / AFIO - STM32F1 layout, GPIOA at 0x40010800
// ---------------------------------------------------------------------------
#define AFIO_BASE       0x40010000UL
#define GPIOA_BASE      0x40010800UL
#define GPIOB_BASE      0x40010C00UL
#define GPIOC_BASE      0x40011000UL
#define GPIOD_BASE      0x40011400UL
#define GPIOE_BASE      0x40011800UL

struct gpio_regs {
    volatile uint32_t CRL;   // 0x00 config low  (pins 0..7)
    volatile uint32_t CRH;   // 0x04 config high (pins 8..15)
    volatile uint32_t IDR;   // 0x08 input data
    volatile uint32_t ODR;   // 0x0C output data
    volatile uint32_t BSRR;  // 0x10 bit set/reset
    volatile uint32_t BRR;   // 0x14 bit reset
    volatile uint32_t LCKR;  // 0x18 lock
};
#define GPIO_REGS(x) ((struct gpio_regs *)(x))

// Pin-number encoding (matches the generic Klipper convention).
#define GPIO(PORT, NUM) (((PORT)-'A') * 16 + (NUM))
#define GPIO2PORT(PIN) ((PIN) / 16)
#define GPIO2BIT(PIN) (1<<((PIN) % 16))

// CRL/CRH per-pin 4-bit field: [CNF1:CNF0:MODE1:MODE0]
#define GPIO_MODE_INPUT     0x0
#define GPIO_MODE_OUT_10MHZ 0x1
#define GPIO_MODE_OUT_2MHZ  0x2
#define GPIO_MODE_OUT_50MHZ 0x3
#define GPIO_CNF_OUT_PP     (0x0 << 2)
#define GPIO_CNF_OUT_OD     (0x1 << 2)
#define GPIO_CNF_OUT_AF_PP  (0x2 << 2)
#define GPIO_CNF_OUT_AF_OD  (0x3 << 2)
#define GPIO_CNF_IN_ANALOG  (0x0 << 2)
#define GPIO_CNF_IN_FLOAT   (0x1 << 2)
#define GPIO_CNF_IN_PULL    (0x2 << 2)

// ---------------------------------------------------------------------------
// USART - STM32F1 layout, USART1 at 0x40013800
// ---------------------------------------------------------------------------
#define USART1_BASE     0x40013800UL
#define USART2_BASE     0x40004400UL
#define USART3_BASE     0x40004800UL

struct usart_regs {
    volatile uint32_t SR;    // 0x00 status
    volatile uint32_t DR;    // 0x04 data
    volatile uint32_t BRR;   // 0x08 baud rate
    volatile uint32_t CR1;   // 0x0C control 1
    volatile uint32_t CR2;   // 0x10 control 2
    volatile uint32_t CR3;   // 0x14 control 3
    volatile uint32_t GTPR;  // 0x18 guard time / prescaler
};
#define USART(x) ((struct usart_regs *)(x))
#define USART_SR_RXNE   (1u << 5)
#define USART_SR_TXE    (1u << 7)
#define USART_SR_TC     (1u << 6)
#define USART_CR1_UE    (1u << 13)
#define USART_CR1_TE    (1u << 3)
#define USART_CR1_RE    (1u << 2)
#define USART_CR1_RXNEIE (1u << 5)

// ---------------------------------------------------------------------------
// General-purpose timers TIM2..TIM5 - STM32F1 layout
// (On CH32V these appear in the 0x4000xxxx / 0x4001xxxx space.)
// ---------------------------------------------------------------------------
struct tim_regs {
    volatile uint32_t CR1;   // 0x00
    volatile uint32_t CR2;   // 0x04
    volatile uint32_t SMCR;  // 0x08
    volatile uint32_t DIER;  // 0x0C
    volatile uint32_t SR;    // 0x10
    volatile uint32_t EGR;   // 0x14
    volatile uint32_t CCMR1; // 0x18
    volatile uint32_t CCMR2; // 0x1C
    volatile uint32_t CCER;  // 0x20
    volatile uint32_t CNT;   // 0x24
    volatile uint32_t PSC;   // 0x28
    volatile uint32_t ARR;   // 0x2C
    volatile uint32_t RCR;   // 0x30
    volatile uint32_t CCR1;  // 0x34
    volatile uint32_t CCR2;  // 0x38
    volatile uint32_t CCR3;  // 0x3C
    volatile uint32_t CCR4;  // 0x40
    volatile uint32_t BDTR;  // 0x44
    volatile uint32_t DCR;   // 0x48
    volatile uint32_t DMAR;  // 0x4C
};
#define TIM2 ((struct tim_regs *)0x40000000UL)
#define TIM3 ((struct tim_regs *)0x40000400UL)
#define TIM4 ((struct tim_regs *)0x40000800UL)

// ---------------------------------------------------------------------------
// ADC1 - STM32F1 layout, 0x40012400
// ---------------------------------------------------------------------------
#define ADC1_BASE       0x40012400UL

// ---------------------------------------------------------------------------
// QingKe RISC-V core: PFIC + custom CSRs
// ---------------------------------------------------------------------------
// Global interrupt enable is CSR 0x800 (QingKe custom, bit 0 = GIE).
#define CSR_GINTENR     0x800

// PFIC sleep control (ARM SCR-compatible address on QingKe).
#define PFIC_SCTLR      (*(volatile uint32_t *)0xE000ED10UL)

// Global interrupt enable CSR (QingKe custom, bit 0 = GIE).
#define CH32V_GIE_BIT   1u

void clock_init(void);
void console_setup(void);

#endif // __CH32V_INTERNAL_H
