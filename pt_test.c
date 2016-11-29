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

/*
 * Test protothread early termination
 */
static void pt_exiting(struct pt *pt, int *state) {
  pt_begin(pt);
  (*state) = 1;
  pt_exit(pt, PT_STATUS_FINISHED);
  (*state) = 2;
  pt_end(pt);
}
static void test_exit() {
  int state = 0;
  struct pt pt = pt_init();

  pt_exiting(&pt, &state);
  assert(state == 1 && pt_status(&pt) == PT_STATUS_FINISHED);
  pt_exiting(&pt, &state);
  assert(state == 1 && pt_status(&pt) == PT_STATUS_FINISHED);
}

/*
 * Test complex protothread wait loops
 */
static void pt_loop_with_braces(struct pt *pt, int *state, int timeout,
                                int ready) {
  pt_begin(pt);
  *state = 1;
  pt_loop(pt, !timeout) {
    if (ready) {
      break;
    }
    (*state)++;
  }
  *state = -1;
  pt_end(pt);
}
static void pt_loop_without_braces(struct pt *pt, int *state, int timeout,
                                   int ready) {
  pt_begin(pt);
  *state = 1;
// clang-format off
#ifdef __clang__
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdangling-else"
#endif
  if (1)
    pt_loop(pt, !timeout)
      if (ready)
	break;
      else
	(*state)++;
#ifdef __clang__
#pragma clang diagnostic pop
#endif
  // clang-format on
  *state = -1;
  pt_end(pt);
}
static void test_loop() {
  int state = 0;
  struct pt pt1 = pt_init();
  struct pt pt2 = pt_init();
  struct pt pt3 = pt_init();

  pt_loop_with_braces(&pt1, &state, 0, 0);
  assert(state == 2);
  pt_loop_with_braces(&pt1, &state, 0, 0);
  assert(state == 3);
  pt_loop_with_braces(&pt1, &state, 1, 0);
  assert(state == -1);

  pt_loop_without_braces(&pt2, &state, 0, 0);
  assert(state == 2);
  pt_loop_without_braces(&pt2, &state, 0, 0);
  assert(state == 3);
  pt_loop_without_braces(&pt2, &state, 1, 0);
  assert(state == -1);

  pt_loop_without_braces(&pt3, &state, 0, 0);
  assert(state == 2);
  pt_loop_without_braces(&pt3, &state, 0, 0);
  assert(state == 3);
  pt_loop_without_braces(&pt3, &state, 0, 1);
  assert(state == -1);
}

struct pair {
  int x;
  int y;
};
typedef pt_queue(int, 3) int_queue_t;
typedef pt_queue(struct pair, 3) pair_queue_t;
static void pt_add(struct pt *pt, pair_queue_t *pairs, int_queue_t *ints) {
  pt_begin(pt);
  for (;;) {
    pt_wait(pt, !pt_queue_empty(pairs));
    struct pair *p = pt_queue_pop(pairs);
    pt_wait(pt, !pt_queue_full(ints));
    pt_queue_push(ints, p->x + p->y);
  }
  pt_end(pt);
}
static void test_queues() {
  struct pt pt = pt_init();
  int_queue_t ints = pt_queue_init();
  pair_queue_t pairs = pt_queue_init();

  assert(pt_queue_len(&ints) == 3);
  assert(pt_queue_cap(&ints) == 0);
  assert(pt_queue_empty(&ints));
  assert(!pt_queue_full(&ints));

  assert(pt_queue_len(&pairs) == 3);
  assert(pt_queue_cap(&pairs) == 0);
  assert(pt_queue_empty(&pairs));
  assert(!pt_queue_full(&pairs));

  assert(pt_queue_push(&ints, 1));
  assert(!pt_queue_empty(&ints));
  assert(!pt_queue_full(&ints));

  assert(pt_queue_push(&ints, 3));

  assert(pt_queue_len(&ints) == 3 && pt_queue_cap(&ints) == 2);

  assert(pt_queue_push(&ints, 5));
  assert(!pt_queue_push(&ints, 7));

  assert(pt_queue_full(&ints));

  assert(*(pt_queue_pop(&ints)) == 1);
  assert(!pt_queue_full(&ints));

  assert(*(pt_queue_pop(&ints)) == 3);
  assert(*(pt_queue_pop(&ints)) == 5);
  assert(pt_queue_pop(&ints) == NULL);

  assert(!pt_queue_full(&ints));
  assert(pt_queue_empty(&ints));

  pt_queue_push(&pairs, ((struct pair){1, 2}));
  pt_queue_push(&pairs, ((struct pair){7, 5}));

  pt_add(&pt, &pairs, &ints);

  assert(*(pt_queue_peek(&ints)) == 3);
  assert(*(pt_queue_peek(&ints)) == 3);
  assert(*(pt_queue_pop(&ints)) == 3);
  assert(*(pt_queue_pop(&ints)) == 12);
  assert(pt_queue_pop(&ints) == NULL);
  assert(pt_queue_peek(&ints) == NULL);

  pt_add(&pt, &pairs, &ints);
  assert(pt_queue_pop(&ints) == NULL);

  pt_queue_push(&pairs, ((struct pair){123, 456}));
  pt_add(&pt, &pairs, &ints);
  assert(*(pt_queue_pop(&ints)) == 123 + 456);
}

int main() {
  test_local_continuation();
  test_empty();
  test_wait();
  test_yield();
  test_exit();
  test_loop();
  test_queues();
  return 0;
}
