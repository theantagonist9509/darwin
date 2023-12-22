CC := zig cc

TARGET_EXEC := darwin-client

LIBS := :libraylib.a GL m pthread dl rt X11

CPPFLAGS :=
CFLAGS :=
LDFLAGS :=

INC_DIRS := $(HOME)/install/include
LIB_DIRS := $(HOME)/install/lib
