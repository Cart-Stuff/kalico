// Shared-memory ring transport for the Ingenic X2600 co-processor
//
// Copyright (C) 2026
//
// This file may be distributed under the terms of the GNU GPLv3 license.

#include "internal.h"   // struct ipc_ring, IPC_DOORBELL, HOST_MSG_REG

// The two ring descriptors and their backing buffers live in the shared data
// region. The bootloader/host discovers them through the TPEL header that the
// firmware publishes (see startup.S / console.c).
static struct ipc_ring cmd_ring_desc;   // host -> MCU (commands)
static struct ipc_ring resp_ring_desc;  // MCU  -> host (responses)

// Backing buffers (4 KiB each, matching the stock data_size of 0x1000).
#define RING_SIZE 4096
static uint8_t cmd_buf[RING_SIZE] __attribute__((aligned(64)));
static uint8_t resp_buf[RING_SIZE] __attribute__((aligned(64)));

void
ipc_init(void)
{
    cmd_ring_desc.data = cmd_buf;
    cmd_ring_desc.size = RING_SIZE;
    cmd_ring_desc.w_pos = 0;
    cmd_ring_desc.r_pos = 0;

    resp_ring_desc.data = resp_buf;
    resp_ring_desc.size = RING_SIZE;
    resp_ring_desc.w_pos = 0;
    resp_ring_desc.r_pos = 0;
}

struct ipc_ring *
ipc_cmd_ring(void)
{
    return &cmd_ring_desc;
}

struct ipc_ring *
ipc_resp_ring(void)
{
    return &resp_ring_desc;
}

static inline uint32_t
ring_free(struct ipc_ring *r)
{
    // Space available for the producer (leave one byte to distinguish
    // full from empty).
    return (r->r_pos - r->w_pos - 1) & (r->size - 1);
}

static inline uint32_t
ring_avail(struct ipc_ring *r)
{
    return (r->w_pos - r->r_pos) & (r->size - 1);
}

// Write up to `len` bytes into the ring; returns the number accepted.
uint32_t
ipc_ring_write(struct ipc_ring *r, const uint8_t *src, uint32_t len)
{
    uint32_t free = ring_free(r);
    if (len > free)
        len = free;
    uint32_t first = r->size - r->w_pos;
    if (first > len)
        first = len;
    for (uint32_t i = 0; i < first; i++)
        r->data[r->w_pos + i] = src[i];
    for (uint32_t i = first; i < len; i++)
        r->data[i - first] = src[i];
    r->w_pos = (r->w_pos + len) & (r->size - 1);
    return len;
}

// Read up to `len` bytes out of the ring; returns the number produced.
uint32_t
ipc_ring_read(struct ipc_ring *r, uint8_t *dst, uint32_t len)
{
    uint32_t avail = ring_avail(r);
    if (len > avail)
        len = avail;
    uint32_t first = r->size - r->r_pos;
    if (first > len)
        first = len;
    for (uint32_t i = 0; i < first; i++)
        dst[i] = r->data[r->r_pos + i];
    for (uint32_t i = first; i < len; i++)
        dst[i] = r->data[i - first];
    r->r_pos = (r->r_pos + len) & (r->size - 1);
    return len;
}

uint32_t
ipc_ring_pending(struct ipc_ring *r)
{
    return ring_avail(r);
}

// Notify the host that the response ring has data.
void
ipc_kick_host(void)
{
    IPC_DOORBELL |= IPC_DB_HOST_BIT;
}
