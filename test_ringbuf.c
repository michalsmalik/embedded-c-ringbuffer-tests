/*
 * test_ringbuf.c — tests for the ring buffer. Each test is one scenario from
 * the README's test table. Uses a tiny CHECK macro instead of assert() so that
 * the tests also run in release builds (assert disappears with -DNDEBUG).
 */
#include <stdio.h>
#include <stdlib.h>
#include "ringbuf.h"

static int failures = 0;

/* Tests that use a fixed number of items cap it at the capacity, so they work for any RINGBUF_CAPACITY. */
#define MIN(a, b) ((a) < (b) ? (a) : (b))

#define CHECK(cond) do { \
    if (!(cond)) { fprintf(stderr, "  FAIL %s:%d: %s\n", __FILE__, __LINE__, #cond); failures++; } \
} while (0)

static void test_new_buffer_is_empty(void)
{
    ringbuf_t rb;
    uint8_t b = 0;
    ringbuf_init(&rb);
    CHECK(ringbuf_is_empty(&rb));
    CHECK(!ringbuf_is_full(&rb));
    CHECK(ringbuf_count(&rb) == 0);
    CHECK(!ringbuf_pop(&rb, &b));   /* popping an empty buffer fails */
    CHECK(!ringbuf_peek(&rb, &b));
}

static void test_fifo_order(void)
{
    ringbuf_t rb;
    uint8_t b = 0;
    ringbuf_init(&rb);
    const uint8_t n = (uint8_t)MIN(5u, RINGBUF_CAPACITY);
    for (uint8_t i = 1; i <= n; i++) CHECK(ringbuf_push(&rb, i));
    CHECK(ringbuf_count(&rb) == n);
    for (uint8_t i = 1; i <= n; i++) {
        CHECK(ringbuf_pop(&rb, &b));
        CHECK(b == i);              /* first in, first out */
    }
    CHECK(ringbuf_is_empty(&rb));
}

static void test_full_rejects_push(void)
{
    ringbuf_t rb;
    uint8_t b = 0;
    ringbuf_init(&rb);
    for (size_t i = 0; i < RINGBUF_CAPACITY; i++) CHECK(ringbuf_push(&rb, (uint8_t)i));
    CHECK(ringbuf_is_full(&rb));
    CHECK(!ringbuf_push(&rb, 0xAA));          /* no room: rejected */
    CHECK(ringbuf_count(&rb) == RINGBUF_CAPACITY);
    CHECK(ringbuf_pop(&rb, &b) && b == 0);    /* oldest byte was NOT overwritten */
}

static void test_wraparound(void)
{
    ringbuf_t rb;
    uint8_t b = 0;
    ringbuf_init(&rb);
    /* Fill, drain half, refill: writes now wrap past the end of the array. */
    for (size_t i = 0; i < RINGBUF_CAPACITY; i++) ringbuf_push(&rb, (uint8_t)i);
    for (size_t i = 0; i < RINGBUF_CAPACITY / 2; i++) ringbuf_pop(&rb, &b);
    for (size_t i = 0; i < RINGBUF_CAPACITY / 2; i++) CHECK(ringbuf_push(&rb, (uint8_t)(100 + i)));
    CHECK(ringbuf_is_full(&rb));
    for (size_t i = RINGBUF_CAPACITY / 2; i < RINGBUF_CAPACITY; i++) CHECK(ringbuf_pop(&rb, &b) && b == i);
    for (size_t i = 0; i < RINGBUF_CAPACITY / 2; i++) CHECK(ringbuf_pop(&rb, &b) && b == 100 + i);
    CHECK(ringbuf_is_empty(&rb));
}

static void test_counter_overflow(void)
{
    ringbuf_t rb;
    uint8_t b = 0;
    ringbuf_init(&rb);
    /* Start just below SIZE_MAX so head/tail wrap to 0 during the test. */
    rb.head = rb.tail = (size_t)-3;
    const uint8_t n = (uint8_t)MIN(6u, RINGBUF_CAPACITY);
    for (uint8_t i = 0; i < n; i++) CHECK(ringbuf_push(&rb, i));
    CHECK(ringbuf_count(&rb) == n);
    for (uint8_t i = 0; i < n; i++) CHECK(ringbuf_pop(&rb, &b) && b == i);
    CHECK(ringbuf_is_empty(&rb));
}

static void test_null_safety(void)
{
    uint8_t b = 0;
    ringbuf_t rb;
    ringbuf_init(NULL);
    CHECK(!ringbuf_push(NULL, 1));
    CHECK(!ringbuf_pop(NULL, &b));
    CHECK(ringbuf_count(NULL) == 0);
    ringbuf_init(&rb);
    ringbuf_push(&rb, 7);
    CHECK(!ringbuf_pop(&rb, NULL));   /* no output pointer: nothing removed */
    CHECK(ringbuf_count(&rb) == 1);
}

/* Random push/pop sequence compared against a simple reference model. */
static void test_stress_against_model(void)
{
    ringbuf_t rb;
    uint8_t model[RINGBUF_CAPACITY];
    size_t m_head = 0, m_count = 0;
    uint8_t b = 0;
    ringbuf_init(&rb);
    srand(12345);   /* fixed seed: the test is repeatable */
    for (int step = 0; step < 10000; step++) {
        if (rand() % 2) {
            uint8_t v = (uint8_t)rand();
            bool ok = ringbuf_push(&rb, v);
            CHECK(ok == (m_count < RINGBUF_CAPACITY));
            if (ok) { model[(m_head + m_count) % RINGBUF_CAPACITY] = v; m_count++; }
        } else {
            bool ok = ringbuf_pop(&rb, &b);
            CHECK(ok == (m_count > 0));
            if (ok) { CHECK(b == model[m_head]); m_head = (m_head + 1) % RINGBUF_CAPACITY; m_count--; }
        }
        CHECK(ringbuf_count(&rb) == m_count);
    }
}

int main(void)
{
    struct { const char *name; void (*fn)(void); } tests[] = {
        {"new buffer is empty", test_new_buffer_is_empty},
        {"FIFO order", test_fifo_order},
        {"full buffer rejects push", test_full_rejects_push},
        {"wraparound", test_wraparound},
        {"index counter overflow", test_counter_overflow},
        {"NULL safety", test_null_safety},
        {"10,000 random steps vs. model", test_stress_against_model},
    };
    size_t n = sizeof tests / sizeof tests[0];
    for (size_t i = 0; i < n; i++) {
        int before = failures;
        tests[i].fn();
        printf("[%s] %s\n", failures == before ? " OK " : "FAIL", tests[i].name);
    }
    printf("\n%d failure(s)\n", failures);
    return failures == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}
