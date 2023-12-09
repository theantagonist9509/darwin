CC := zig cc

TARGET_EXEC := darwin-client
LIBS := :libraylib.a GL m pthread dl rt X11

INC_DIRS := ../common/src

LIB_DIRS := $(HOME)/install/lib
LIB_INC_DIRS := $(HOME)/install/include

CPPFLAGS :=
CFLAGS :=
LDFLAGS :=
