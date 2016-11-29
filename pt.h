#ifndef PT_H
#define PT_H

#if PT_USE_SETJMP
/*
 * Local continuation that uses setjmp()/longjmp() to keep thread state.
 *
 * Pros: local variables are preserved, works with all control structures.
 * Cons: slow, may erase global register variables.
 */
#elif PT_USE_GOTO
/*
 * Local continuation based on goto label references.
 *
 * Pros: works with all control sturctures.
 * Cons: requires GCC or Clang, doesn't preserve local variables.
 */
#else
/*
 * Local continuation based on switch/case and line numbers.
 *
 * Pros: works with any C99 compiler, most simple implementation.
 * Cons: doesn't allow more than one label per line, doesn't preserve local variables.
 */
struct pt {
	int label;
	int status;
};
#define pt_init()                                                              \
  { .label = 0 }
#define pt_begin(pt)                                                           \
  switch ((pt)->label) {                                                       \
  case 0:
#define pt_label(pt, stat)                                                     \
  do {                                                                         \
    (pt)->label = __LINE__;                                                    \
    (pt)->status = (stat);                                                     \
  case __LINE__:;                                                              \
  } while (0)
#define pt_end(pt)                                                             \
  pt_label(pt, -1);                                                            \
  }
#endif

#endif /* PT_H */
