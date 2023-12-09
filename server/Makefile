include config.mk
$(TARGET_EXEC): config.mk Makefile

CPPFLAGS += -Isrc $(addprefix -I,$(INC_DIRS) $(LIB_INC_DIRS)) -MMD -MP
LDFLAGS += $(addprefix -L,$(LIB_DIRS)) $(addprefix -l,$(LIBS))

SRCS := $(wildcard src/*.c)
DEPS := $(SRCS:src/%.c=build/%.d)
OBJS := $(SRCS:src/%.c=build/%.o)

COMMON_OBJS := $(COMMON_SRCS:../common/src/%.c=../common/build/%.o)

$(TARGET_EXEC): $(OBJS) $(COMMON_OBJS)
	$(CC) $(LDFLAGS) $(OBJS) $(COMMON_OBJS) -o $@

build/%.o: src/%.c
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@

-include $(DEPS)

$(COMMON_OBJS):
	make -C ../common

clean:
	rm -f $(TARGET_EXEC) $(wildcard build/*)
	make -C ../common clean

.PHONY: clean
