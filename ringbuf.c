/*
 * ringbuf.c — implementation. head and tail only ever count up; the slot is
 * index & (capacity - 1). count = head - tail, which stays correct even when the
 * counters wrap around SIZE_MAX, because unsigned subtraction is modular.
 * This avoids the classic "is it full or empty when head == tail?" ambiguity.
 */
#include "ringbuf.h"

#define MASK (RINGBUF_CAPACITY - 1u)

void ringbuf_init(ringbuf_t *rb)
{
    if (rb == NULL) return;
    rb->head = 0;
    rb->tail = 0;
}

size_t ringbuf_count(const ringbuf_t *rb)
{
    return rb == NULL ? 0 : rb->head - rb->tail;
}

bool ringbuf_is_empty(const ringbuf_t *rb)
{
    return ringbuf_count(rb) == 0;
}

bool ringbuf_is_full(const ringbuf_t *rb)
{
    return rb != NULL && ringbuf_count(rb) == RINGBUF_CAPACITY;
}

bool ringbuf_push(ringbuf_t *rb, uint8_t byte)
{
    if (rb == NULL || ringbuf_is_full(rb)) return false;
    rb->data[rb->head & MASK] = byte;
    rb->head++;
    return true;
}

bool ringbuf_peek(const ringbuf_t *rb, uint8_t *out)
{
    if (rb == NULL || out == NULL || ringbuf_is_empty(rb)) return false;
    *out = rb->data[rb->tail & MASK];
    return true;
}

bool ringbuf_pop(ringbuf_t *rb, uint8_t *out)
{
    if (!ringbuf_peek(rb, out)) return false;
    rb->tail++;
    return true;
}
