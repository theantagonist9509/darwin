include config.mk

all: config.mk Makefile

SRCS := $(wildcard src/*.c)
OBJS := $(SRCS:src/%.c=build/%.o)

all: $(OBJS)

build/%.o: src/%.c
	@mkdir -p build
	$(CC) -c $< -o $@ $(CPPFLAGS) $(CFLAGS)

clean:
	rm -f $(wildcard build/*)

.PHONY: all clean

-include $(DEPS)
