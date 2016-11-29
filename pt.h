#ifndef PT_H
#define PT_H

/* Helper macros to generate unique labels */
#define _pt_line3(name, line) _pt_##name##line
#define _pt_line2(name, line) _pt_line3(name, line)
#define _pt_line(name) _pt_line2(name, __LINE__)

#if PT_USE_SETJMP
/*
 * Local continuation that uses setjmp()/longjmp() to keep thread state.
 *
 * Pros: local variables are preserved, works with all control structures.
 * Cons: slow, may erase global register variables.
 */
#include <setjmp.h>
struct pt {
  jmp_buf env;
  int isset;
  int status;
};
#define pt_init()                                                              \
  { .isset = 0, .status = 0 }
#define pt_begin(pt)                                                           \
  do {                                                                         \
    if ((pt)->isset) {                                                         \
      longjmp((pt)->env, 0);                                                   \
    }                                                                          \
  } while (0)
#define pt_label(pt, stat)                                                     \
  do {                                                                         \
    (pt)->isset = 1;                                                           \
    (pt)->status = (stat);                                                     \
    setjmp((pt)->env);                                                         \
  } while (0)
#define pt_end(pt) pt_label(pt, -1)
#elif PT_USE_GOTO
#include <unistd.h>
/*
 * Local continuation based on goto label references.
 *
 * Pros: works with all control sturctures.
 * Cons: requires GCC or Clang, doesn't preserve local variables.
 */
struct pt {
  void *label;
  int status;
};
#define pt_init()                                                              \
  { .label = NULL, .status = 0 }
#define pt_begin(pt)                                                           \
  do {                                                                         \
    if ((pt)->label != NULL) {                                                 \
      goto *(pt)->label;                                                       \
    }                                                                          \
  } while (0)

#define pt_label(pt, stat)                                                     \
  do {                                                                         \
    _pt_line(label) : (pt)->label = &&_pt_line(label);                         \
    (pt)->status = (stat);                                                     \
  } while (0)
#define pt_end(pt) pt_label(pt, -1)
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
  { .label = 0, .status = 0 }
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
