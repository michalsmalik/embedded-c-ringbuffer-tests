/*
 * ringbuf.h — fixed-size byte ring buffer (FIFO) for embedded-style code.
 *
 * No dynamic memory: the storage lives inside the struct, so a buffer can be a
 * global or a stack variable (typical for UART/serial receive buffers on a
 * microcontroller). When full, push() refuses new data instead of silently
 * overwriting the oldest byte, so the caller always knows data was dropped.
 */
#ifndef RINGBUF_H
#define RINGBUF_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

/* Capacity must be a power of two (>= 2): indexes wrap with a cheap bit mask. */
#ifndef RINGBUF_CAPACITY
#define RINGBUF_CAPACITY 16u
#endif

#if (RINGBUF_CAPACITY < 2) || ((RINGBUF_CAPACITY & (RINGBUF_CAPACITY - 1u)) != 0)
#error "RINGBUF_CAPACITY must be a power of two, at least 2"
#endif

typedef struct {
    uint8_t data[RINGBUF_CAPACITY];
    size_t head;  /* total bytes ever written; next write goes to head % capacity */
    size_t tail;  /* total bytes ever read;    next read  comes from tail % capacity */
} ringbuf_t;

void   ringbuf_init(ringbuf_t *rb);
bool   ringbuf_push(ringbuf_t *rb, uint8_t byte);   /* false if full (byte not stored) */
bool   ringbuf_pop(ringbuf_t *rb, uint8_t *out);    /* false if empty */
bool   ringbuf_peek(const ringbuf_t *rb, uint8_t *out);
size_t ringbuf_count(const ringbuf_t *rb);
bool   ringbuf_is_empty(const ringbuf_t *rb);
bool   ringbuf_is_full(const ringbuf_t *rb);

#endif /* RINGBUF_H */
