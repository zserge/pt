pt
==

[![Build Status](https://travis-ci.org/zserge/pt.svg?branch=master)](https://travis-ci.org/zserge/pt)

Pt is the most lightweight [protothreads][1] or [coroutines][2] implementation I
could only think of.

Pt allows building subroutines that can suspend at certain points and can be
resumed later. These are the building blocks for co-operative multitasking.

Pt has been tested on Linux and bare-metal STM32, but would work just fine on
any other embedded platform such as AVR or MSP430.

## Features

* All code is just a single header file - easy to integrate in your project
* Small code base - only 178 lines of code
* Simple API - protothread API is only 9 functions
* Supports switch/case, goto labels or `setjmp/longjmp` to save coroutine state (continuation)
* C99 (unless you use [goto labels][3], which is a GCC/Clang extension)
* Comes with message queues as a bonus (20 lines of code) and a wrapper for POSIX syscalls

## Example

```c
typedef pt_queue(struct packet, 32) packet_queue_t;
void producer(struct pt *pt, packet_queue_t *q) {
	pt_begin(pt);
	for (;;) {
		pt_wait(pt, !pt_queue_full(q) && packet_available());
		pt_queue_push(q, packet_read());
	}
	pt_end(pt);
}

void consumer(struct pt *pt, packet_queue_t *q) {
	pt_begin(pt);
	for (;;) {
		/* For for some data in the queue */
		pt_wait(pt, !pt_queue_empty(q));
		struct packet p = pt_queue_pop(q);
		/* process packet here */
	}
	pt_end(pt);
}

...

struct pt pt_producer = pt_init();
struct pt pt_consumer = pt_init();
packet_queue_t queue = pt_queue_init();

for (;;) {
	producer(&pt_producer, &queue);
	consumer(&pt_consumer, &queue);
}
```

## API

Protothread API:

* `struct pt my_pt = pt_init();` - protothread handle.
* `pt_init()` - returns an initialize protothread handle.
* `pt_begin(pt)` - must be the first line in each protothread.
* `pt_end(pt)` - must be the last line in each protothread, changes `pt` status
	to `PT_STATUS_FINISHED`.
* `pt_exit(pt, status)` - terminates current protothread `pt` with the given status.
* `pt_status(pt)` - returns `pt` status. Can be `PT_STATUS_FINISHED` or
	`PT_STATUS_BLOCKED` or any other status passed into `pt_exit`.
* `pt_yield(pt)` - suspends protothread until it's called again.
* `pt_wait(pt, cond)` - suspends protothread until `cond` becomes true.
* `pt_loop(pt, cond) { ... }` - executes the loop  (yielding on each iteration)
	while the condition is true, or until `break;` is called inside the loop.
* `pt_sys(pt, syscall(...))` - suspends protothread while `syscall(...)`
	returns `-1` and `errno` says that it should be retried. Should work with
	most POSIX syscalls.
* `pt_label(pt, status)` - low-level API to create continuation, normally
	should not be used.

Queue API:

* `pt_queue(type, size)` - defines queue type of given element type and
capacity. Normally should be used as `typedef pt_queue(struct my_item, 32)
my_item_queue_t;`
* `pt_queue_init()` - returns initialize queue instance. E.g. `my_item_queue_t
	q = pt_queue_init();`
* `pt_queue_len(q)` - returns queue length (number of items written and not read).
* `pt_queue_cap(q)` - returns maximum queue capacity (size of underlying buffer).
* `pt_queue_empty(q)` - returns 1 if queue is empty, 0 otherwise.
* `pt_queue_full(q)` - returns 1 if queue is full, 0 otherwise.
* `pt_queue_reset(q)` - reset queue to zero length.
* `pt_queue_push(q, item)` - pushes item to the queue and returns 1. If queue is full - returns 0.
* `pt_queue_peek(q)` - returns pointer to the first item in the queue, or NULL if queue is empty.
* `pt_queue_pop(q)` - returns pointer to the first item in the queue moving the
	read pointer. Returns NULL if queue is empty.

## How is it better than other protothread libraries (e.g. Adam Dunkels Protorheads)?

Pt has a very compact implementation of queues, that doesn't require
malloc/free, works with any data types and plays nicely with protothreads.

Pt has `pt_loop` which is much more powerful that `pt_wait()`, because it can
run some non-blocking code while waiting and have nested loops.

Pt support setjmp/longjmp and has a convenient wrapper for syscalls.

Pt doesn't tell you how to schedule protothreads. Since they are just
re-entrant functions - you can nest them, call them all in a loop, or build
your own scheduler.

Pt is well covered with tests.

## License

Code is distributed under MIT license, feel free to use it in your proprietary
projects as well.

[1]: http://dunkels.com/adam/pt/
[2]: https://en.wikipedia.org/wiki/Coroutine
[3]: https://gcc.gnu.org/onlinedocs/gcc/Labels-as-Values.html
