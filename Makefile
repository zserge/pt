CFLAGS ?= -std=gnu99 -Wall -Wextra -Werror
CFLAGS_STRICT ?= $(CFLAGS) -std=c99 -pedantic

all: test example_nc

test: pt_test_switch pt_test_goto pt_test_setjmp
	./pt_test_switch
	./pt_test_goto
	./pt_test_setjmp

pt_test_switch: pt_test.c pt.h
	$(CC) $(CFLAGS_STRICT) $< -o $@
pt_test_goto: pt_test.c pt.h
	$(CC) $(CFLAGS) -DPT_USE_GOTO=1 $< -o $@
pt_test_setjmp: pt_test.c pt.h
	$(CC) $(CFLAGS_STRICT) -DPT_USE_SETJMP=1 $< -o $@

example_nc: example_nc.o
example_nc.o : example_nc.c pt.h

clean:
	rm -f *.o
	rm -f pt_test_switch pt_test_goto pt_test_setjmp
	rm -f example_nc

.PHONY: all clean
