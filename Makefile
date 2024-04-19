# Optimize, turn on additional warnings, generate dep files
CFLAGS = -O3 -MMD -std=c17 -g -Wall -Wextra -no-pie -D_POSIX_C_SOURCE=200809L -msse4.2
# Link with libm so we can use the math library in tests
LDLIBS += -lm

src = $(wildcard src/*.c) $(wildcard src/implementations/*.c) $(wildcard src/implementations/impl_naive/*.c) $(wildcard src/implementations/impl_binary_conversion/*.c)
obj = $(src:.c=.o)
dep = $(obj:.o=.d)

.PHONY: all
all: main

main: $(obj)
	$(CC) $(CFLAGS) -o $@ $^ $(LDLIBS)

.PHONY: test
test: CFLAGS += -fsanitize=address -fsanitize=undefined -fno-omit-frame-pointer
test: main

.PHONY: analyze
analyze: CFLAGS += -fanalyzer
analyze: main

.PHONY: debug
debug: CFLAGS += -O0 -DANNOUNCE_TESTS -DLOG
debug: test

# include all dep files in the makefile
-include $(dep)

.PHONY: clean
clean:
	@find src -name "*.[o|d]" -type f -delete -printf "removed '%P'\n" && rm -vf main