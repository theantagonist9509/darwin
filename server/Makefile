include config.mk

$(TARGET_EXEC): config.mk Makefile

SRCS := $(wildcard src/*.c)
DEPS := $(SRCS:src/%.c=build/%.d)
OBJS := $(SRCS:src/%.c=build/%.o)

COMMON_OBJS = $(wildcard ../common/build/*.o)

CPPFLAGS += -Isrc -I../common/src $(addprefix -I,$(INC_DIRS)) -MMD -MP
LDFLAGS += $(addprefix -l,$(LIBS)) $(addprefix -L,$(LIB_DIRS))

$(TARGET_EXEC): $(OBJS) common_objs
	$(CC) $(OBJS) $(COMMON_OBJS) -o $@ $(LDFLAGS)

build/%.o: src/%.c
	@mkdir -p build
	$(CC) -c $< -o $@ $(CPPFLAGS) $(CFLAGS)

-include $(DEPS)

common_objs:
	$(MAKE) -C ../common

clean:
	$(MAKE) -C ../common clean
	$(RM) $(TARGET_EXEC) $(wildcard build/*)

.PHONY: common_objs clean
