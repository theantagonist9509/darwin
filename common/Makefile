CC := zig cc
CFLAGS :=

SRCS := $(wildcard src/*.c)
OBJS := $(SRCS:src/%.c=build/%.o)

all: $(OBJS)

build/%.o: src/%.c
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@

-include $(DEPS)

clean:
	rm -f $(wildcard build/*)

.PHONY: all clean
