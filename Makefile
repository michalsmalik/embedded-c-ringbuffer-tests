CC      ?= gcc
CFLAGS  ?= -std=c11 -Wall -Wextra -Werror -pedantic -O2 -g

.PHONY: test sanitize valgrind clean

test_ringbuf: test_ringbuf.c ringbuf.c ringbuf.h
	$(CC) $(CFLAGS) -o $@ test_ringbuf.c ringbuf.c

test: test_ringbuf
	./test_ringbuf

# AddressSanitizer + UndefinedBehaviorSanitizer: catch out-of-bounds access and UB at runtime
sanitize:
	$(CC) $(CFLAGS) -fsanitize=address,undefined -fno-omit-frame-pointer -o test_ringbuf_san test_ringbuf.c ringbuf.c
	./test_ringbuf_san

valgrind: test_ringbuf
	valgrind --error-exitcode=1 --leak-check=full -q ./test_ringbuf

clean:
	rm -f test_ringbuf test_ringbuf_san
