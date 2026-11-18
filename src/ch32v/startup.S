// Startup and vector table for WCH CH32V (QingKe RISC-V)
//
// Copyright (C) 2026
//
// This file may be distributed under the terms of the GNU GPLv3 license.
//
// The QingKe core boots from the reset vector (offset 0) which jumps to
// ResetHandler. Interrupts are dispatched through the PFIC using a vector
// table of absolute handler addresses (mtvec in vectored mode).

    .section .vector_table,"a",%progbits
    .globl  VectorTable
    .align  2
VectorTable:
    // Offset 0: reset instruction. A real `j` (jal x0) to ResetHandler; the
    // assembler/linker resolves the cross-section offset.
    j       ResetHandler
    // QingKe vectors are handler addresses. Unused entries point to the
    // default trap handler. The first few slots are core exceptions/NMI.
    .word   DefaultHandler   // 0x04 (unused / instruction misaligned)
    .word   DefaultHandler   // 0x08
    .word   DefaultHandler   // 0x0C
    .word   DefaultHandler   // 0x10
    .word   DefaultHandler   // 0x14 NMI
    .word   DefaultHandler   // 0x18 HardFault / exception
    .word   DefaultHandler   // 0x1C
    .word   DefaultHandler   // 0x20
    .word   DefaultHandler   // 0x24
    .word   DefaultHandler   // 0x28
    .word   DefaultHandler   // 0x2C
    .word   DefaultHandler   // 0x30
    .word   DefaultHandler   // 0x34
    .word   DefaultHandler   // 0x38
    .word   DefaultHandler   // 0x3C
    // Peripheral interrupts begin at vector 16. TIM2 global = IRQ 28,
    // USART1 global = IRQ 37 (CH32V PFIC numbering).
    .rept   (28 - 16)
    .word   DefaultHandler
    .endr
    .word   TIM2_IRQHandler  // IRQ 28 (TIM2)
    .rept   (37 - 29)
    .word   DefaultHandler
    .endr
    .word   USART1_IRQHandler // IRQ 37 (USART1)
    .rept   (64 - 38)
    .word   DefaultHandler
    .endr

    .section .text.ResetHandler,"ax",%progbits
    .globl  ResetHandler
    .type   ResetHandler, %function
ResetHandler:
    // Set the global pointer (relaxation disabled at link time would be
    // ideal; initialise gp explicitly for small-data access).
    .option push
    .option norelax
    la      gp, __global_pointer$
    .option pop

    // Set the stack pointer to the top of RAM.
    la      sp, _stack_end

    // Copy .data from flash to RAM.
    la      a0, _data_flash
    la      a1, _data_start
    la      a2, _data_end
1:  bgeu    a1, a2, 2f
    lw      t0, 0(a0)
    sw      t0, 0(a1)
    addi    a0, a0, 4
    addi    a1, a1, 4
    j       1b
2:
    // Zero .bss.
    la      a0, _bss_start
    la      a1, _bss_end
3:  bgeu    a0, a1, 4f
    sw      zero, 0(a0)
    addi    a0, a0, 4
    j       3b
4:
    // Configure mtvec for vectored interrupts at our table.
    la      t0, VectorTable
    ori     t0, t0, 3          // vectored mode (QingKe: addr|0x3)
    csrw    mtvec, t0

    // Enable global interrupts (QingKe CSR 0x800 bit 0).
    li      t0, 1
    csrrs   zero, 0x800, t0

    // Enter C main.
    jal     ch32v_main
5:  j       5b                  // should never return

    .size   ResetHandler, .-ResetHandler

    .section .text.DefaultHandler,"ax",%progbits
    .globl  DefaultHandler
    .type   DefaultHandler, %function
DefaultHandler:
    j       DefaultHandler      // spin on unexpected interrupt
    .size   DefaultHandler, .-DefaultHandler

// PFIC interrupt enable helper called from C:
//   void ch32v_pfic_enable(uint8_t irqn, uint8_t prio)
// On QingKe the PFIC is NVIC-register-compatible for enable/priority:
//   ISER at 0xE000E100 (one bit per IRQ), IPR at 0xE000E400 (one byte per IRQ).
    .section .text.ch32v_pfic_enable,"ax",%progbits
    .globl  ch32v_pfic_enable
    .type   ch32v_pfic_enable, %function
ch32v_pfic_enable:
    // a0 = irqn, a1 = priority
    li      t0, 0xE000E100       // PFIC ISER base
    srli    t1, a0, 5            // irqn / 32
    slli    t1, t1, 2
    add     t0, t0, t1
    li      t2, 1
    andi    t3, a0, 31
    sll     t2, t2, t3           // 1 << (irqn % 32)
    sw      t2, 0(t0)
    // Priority byte
    li      t0, 0xE000E400       // PFIC IPR base
    add     t0, t0, a0
    sb      a1, 0(t0)
    ret
    .size   ch32v_pfic_enable, .-ch32v_pfic_enable

    // Provide the small-data anchor symbol the linker expects.
    .section .sdata,"aw",%progbits
    .globl  __global_pointer$
__global_pointer$:
    .word   0
