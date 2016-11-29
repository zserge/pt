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
	assert(pt.status == 1 && reent == 1 && state == 1);

	pt_reentrant(&pt, &reent, &state);
	assert(pt.status == 2 && reent == 2 && state == 2);

	pt_reentrant(&pt, &reent, &state);
	assert(pt.status == -1 && reent == 3 && state == 2);

	pt_reentrant(&pt, &reent, &state);
	assert(pt.status == -1 && reent == 4 && state == 2);
}

int main() {
	test_local_continuation();
	return 0;
}
