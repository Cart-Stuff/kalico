// Startup + TPEL boot header for the Ingenic X2600 co-processor
//
// Copyright (C) 2026
//
// This file may be distributed under the terms of the GNU GPLv3 license.
//
// The X2600 bootloader loads this image into the co-processor SRAM at
// 0x0FF20000 and points register 0xB2A00004 at the image start. The image
// therefore begins with a TPEL descriptor: a reset jump, the TPEL magic, and
// the command/response ring descriptor pointers the host's soc_mcu.ko reads.

    .section .vector_table,"a",%progbits
    .globl  VectorTable
    .align  2
VectorTable:
    // +0x00: reset instruction (jal to ResetHandler)
    j       ResetHandler
    // +0x04: TPEL magic
    .word   0x4C455054
    // +0x08 .. +0x14: reserved
    .word   0
    .word   0
    .word   0
    .word   0
    // +0x18: command-ring descriptor pointer (host -> MCU)
    .word   ipc_cmd_ring_ptr
    // +0x1C: response-ring descriptor pointer (MCU -> host)
    .word   ipc_resp_ring_ptr
    // +0x20: shared data region base
    .word   0
    // +0x24: shared data region size
    .word   4096

    .section .text.ResetHandler,"ax",%progbits
    .globl  ResetHandler
    .type   ResetHandler, %function
ResetHandler:
    .option push
    .option norelax
    la      gp, __global_pointer$
    .option pop
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
    // mtvec -> our trap vector (direct mode; IPC uses the doorbell IRQ).
    la      t0, DefaultHandler
    csrw    mtvec, t0

    // Enable global interrupts (mstatus.MIE).
    li      t0, 8
    csrrs   zero, mstatus, t0

    jal     x2600_main
5:  j       5b
    .size   ResetHandler, .-ResetHandler

    .section .text.DefaultHandler,"ax",%progbits
    .globl  DefaultHandler
    .type   DefaultHandler, %function
DefaultHandler:
    j       DefaultHandler
    .size   DefaultHandler, .-DefaultHandler

    .section .sdata,"aw",%progbits
    .globl  __global_pointer$
__global_pointer$:
    .word   0
