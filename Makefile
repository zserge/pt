CFLAGS ?= -std=c99 -Wall -Wextra -Werror -pedantic

all: test

test: pt_test_switch pt_test_goto
	#pt_test_setjmp
	-./pt_test_switch
	-./pt_test_goto
	#-./pt_test_setjmp

pt_test_switch: pt_test.c pt.h
	$(CC) $(CFLAGS) $< -o $@
pt_test_goto: pt_test.c pt.h
	$(CC) $(CFLAGS) -Wno-pedantic -DPT_USE_GOTO=1 $< -o $@
#pt_test_setjmp: pt_test.c pt.h
	#$(CC) $(CFLAGS) -DPT_USE_SETJMP=1 $< -o $@

clean:
	rm -f *.o
	rm -f pt_test_switch pt_test_goto pt_test_setjmp

.PHONY: all clean
