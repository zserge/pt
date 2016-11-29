#include <assert.h>

#include "pt.h"

/*
 * Test reentrant functions with local continuations
 */
static void pt_reentrant(struct pt *pt, int *reent, int *state) {
	(*reent)++;
	pt_begin(pt);
	pt_label(pt, 1);
	if (*state == 0) {
		*state = 1;
		return;
	}
	pt_label(pt, 2);
	if (*state == 1) {
		*state = 2;
		return;
	}
	pt_end(pt);
}

static void test_local_continuation() {
	int reent = 0;
	int state = 0;
	struct pt pt = pt_init();

	pt_reentrant(&pt, &reent, &state);
	assert(pt_status(&pt) == 1 && reent == 1 && state == 1);

	pt_reentrant(&pt, &reent, &state);
	assert(pt_status(&pt) == 2 && reent == 2 && state == 2);

	pt_reentrant(&pt, &reent, &state);
	assert(pt_status(&pt) == -1 && reent == 3 && state == 2);

	pt_reentrant(&pt, &reent, &state);
	assert(pt_status(&pt) == -1 && reent == 4 && state == 2);
}

/*
 * Test empty protothread
 */
static void pt_empty(struct pt *pt, int *reent) {
	pt_begin(pt);
	(*reent)++;
	pt_end(pt);
}
static void test_empty() {
	int reent = 0;
	struct pt pt = pt_init();

	pt_empty(&pt, &reent);
	assert(reent == 1);
	/* shoud not re-enter */
	pt_empty(&pt, &reent);
	assert(reent == 1);
}

/*
 * Test waiting for certain conditions
 */
static void pt_wait_ready(struct pt *pt, int *reent, int ready) {
	pt_begin(pt);
	(*reent)++;
	pt_wait(pt, ready);
	(*reent)++;
	pt_end(pt);
}
static void test_wait() {
	int ready = 0;
	int reent = 0;
	struct pt pt = pt_init();

	pt_wait_ready(&pt, &reent, ready);
	assert(reent == 1);
	pt_wait_ready(&pt, &reent, ready);
	assert(reent == 1);

	ready = 1;

	pt_wait_ready(&pt, &reent, ready);
	assert(reent == 2);
}

/*
 * Test protothread explicit yielding
 */
static void pt_yielding(struct pt *pt, int *state) {
  pt_begin(pt);
	(*state) = 1;
  pt_yield(pt);
	(*state) = 2;
  pt_end(pt);
}
static void test_yield() {
	int state = 0;
  struct pt pt1 = pt_init();
  struct pt pt2 = pt_init();

  pt_yielding(&pt1, &state);
  assert(pt_status(&pt1) == PT_STATUS_BLOCKED && state == 1);
  pt_yielding(&pt2, &state);
  assert(pt_status(&pt2) == PT_STATUS_BLOCKED && state == 1);
  pt_yielding(&pt1, &state);
  assert(pt_status(&pt1) == PT_STATUS_FINISHED && state == 2);
  pt_yielding(&pt2, &state);
  assert(pt_status(&pt2) == PT_STATUS_FINISHED && state == 2);
}

int main() {
	test_local_continuation();
	test_empty();
	test_wait();
	test_yield();
	return 0;
}
