# embedded-c-ringbuffer-tests

A small, heap-free ring buffer (FIFO) in C11, the kind used to buffer bytes from a UART/serial line
on a microcontroller, with a test suite, sanitizer and valgrind runs, and CI.

- **No `malloc`**: storage lives inside the struct, so a buffer can be a global or a stack variable.
- **Never overwrites silently**: `ringbuf_push()` returns `false` when the buffer is full.
- **No full/empty ambiguity**: `head` and `tail` are free-running counters; `count = head - tail`
  stays correct even when they wrap past `SIZE_MAX` (unsigned arithmetic is modular).
- **Power-of-two capacity** (compile-time, default 16): indexes wrap with a bit mask instead of `%`.

## API

```c
#include "ringbuf.h"

ringbuf_t rx;
ringbuf_init(&rx);

ringbuf_push(&rx, byte);          // false if full: the byte was NOT stored
uint8_t b;
while (ringbuf_pop(&rx, &b)) {    // false when empty
    handle(b);
}
```

Also: `ringbuf_peek`, `ringbuf_count`, `ringbuf_is_empty`, `ringbuf_is_full`.
Change the size with `-DRINGBUF_CAPACITY=64u` (must be a power of two, at least 2; anything else fails to compile).

## Build and test

```bash
make test        # build with -Wall -Wextra -Werror -pedantic and run the tests
make sanitize    # same tests under AddressSanitizer + UndefinedBehaviorSanitizer
make valgrind    # same tests under valgrind (memory errors)
make clean
```

## Test scenarios

| # | Scenario | Expected result |
|---|---|---|
| 1 | New buffer | empty, count 0, `pop`/`peek` return false |
| 2 | Push 5, pop 5 | bytes come out in the same order (FIFO) |
| 3 | Fill to capacity, push once more | push returns false, oldest byte is still there |
| 4 | Fill, drain half, refill | writes wrap past the end of the array; order is kept |
| 5 | Counters start near `SIZE_MAX` | count and order stay correct when counters wrap to 0 |
| 6 | `NULL` buffer or output pointer | functions return false/0, nothing is removed |
| 7 | 10,000 random push/pop steps | always matches a simple reference model (fixed seed, repeatable) |

CI runs all of them with GCC and Clang, at capacities 2, 16 and 256, plus the sanitizer and valgrind runs.

## Do the tests catch bugs?

To check the tests themselves, I planted three bugs in `ringbuf.c` one at a time (mutation testing).
Each one was caught:

| Planted bug | Change | Caught by |
|---|---|---|
| Off-by-one in "full" | `count == CAPACITY` → `count == CAPACITY - 1` | scenarios 3, 4, 7 |
| Overwrite when full | removed the `is_full` check in `push` | scenarios 3, 7 |
| Reads newest instead of oldest | `pop` reads at `head - 1` instead of `tail` | scenarios 2, 3, 4, 5, 7 |

Example bug report for the first one, the way it would go to a developer:

> **Title:** Buffer reports "full" one byte early
> **Steps:** init, push `CAPACITY` bytes.
> **Expected:** all pushes succeed; `is_full` becomes true after the last one.
> **Actual:** the last push returns false; usable capacity is `CAPACITY - 1`.
> **Cause:** off-by-one in `ringbuf_is_full()`.

## License

[MIT](LICENSE)

## AI usage

This project was created with the help of AI. The code, tests and documentation were written with [Claude Code](https://claude.com/claude-code) (Anthropic), then reviewed, run and verified by me.
