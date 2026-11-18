// Console (Klipper command framing) over X2600 IPC rings
//
// Copyright (C) 2026
//
// This file may be distributed under the terms of the GNU GPLv3 license.

#include <string.h>          // memcpy
#include "board/irq.h"       // irq_save
#include "command.h"         // command_find_and_dispatch, command_encode_and_frame
#include "compiler.h"        // __visible
#include "internal.h"        // struct ipc_ring
#include "sched.h"           // sched_wake_task, sched_check_wake

// Ring descriptors from ipc.c.
extern struct ipc_ring *ipc_cmd_ring(void);
extern struct ipc_ring *ipc_resp_ring(void);
extern uint32_t ipc_ring_read(struct ipc_ring *r, uint8_t *dst, uint32_t len);
extern uint32_t ipc_ring_write(struct ipc_ring *r, const uint8_t *src,
                               uint32_t len);
extern uint32_t ipc_ring_pending(struct ipc_ring *r);
extern void ipc_kick_host(void);
extern void ipc_init(void);

// Symbols referenced by the TPEL header in startup.S. The host reads these
// pointers to locate the ring descriptors.
__visible struct ipc_ring *ipc_cmd_ring_ptr;
__visible struct ipc_ring *ipc_resp_ring_ptr;

static struct task_wake console_wake;
static uint8_t receive_buf[192];
static uint32_t receive_pos;

void
console_setup(void)
{
    ipc_init();
    // Point the TPEL header pointers at the live descriptors.
    ipc_cmd_ring_ptr = ipc_cmd_ring();
    ipc_resp_ring_ptr = ipc_resp_ring();
}

// Drain available command bytes from the command ring and dispatch them.
void
console_task(void)
{
    if (!sched_check_wake(&console_wake))
        return;

    struct ipc_ring *cmd = ipc_cmd_ring();
    uint32_t n = ipc_ring_pending(cmd);
    if (!n)
        return;

    uint32_t space = sizeof(receive_buf) - receive_pos;
    if (n > space)
        n = space;
    uint32_t got = ipc_ring_read(cmd, receive_buf + receive_pos, n);
    receive_pos += got;

    uint_fast8_t pop_count;
    uint_fast8_t msglen = receive_pos > MESSAGE_MAX ? MESSAGE_MAX : receive_pos;
    int ret = command_find_and_dispatch(receive_buf, msglen, &pop_count);
    if (ret) {
        receive_pos -= pop_count;
        if (receive_pos) {
            memcpy(receive_buf, &receive_buf[pop_count], receive_pos);
            sched_wake_task(&console_wake);
        }
    }
}
DECL_TASK(console_task);

// Encode and transmit a response message over the response ring.
void
console_sendf(const struct command_encoder *ce, va_list args)
{
    uint8_t buf[MESSAGE_MAX];
    uint_fast8_t msglen = command_encode_and_frame(buf, ce, args);

    struct ipc_ring *resp = ipc_resp_ring();
    irqstatus_t flag = irq_save();
    uint32_t written = ipc_ring_write(resp, buf, msglen);
    irq_restore(flag);
    if (written)
        ipc_kick_host();
}

// Wake the console task when the host signals new command data.
void
console_notify(void)
{
    sched_wake_task(&console_wake);
}
