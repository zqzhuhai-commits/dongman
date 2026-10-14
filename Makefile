TARGET := dongman

SOURCES := source/main.c

include $(DEVKITPRO)/libnx/switch_rules

CFLAGS := 
LIBS := -lnx

all: $(TARGET).nro
