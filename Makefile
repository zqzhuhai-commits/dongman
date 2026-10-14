TARGET := dongman

SOURCES := source/main.c
OFILES := $(SOURCES:.c=.o)

include $(DEVKITPRO)/libnx/switch_rules

all: $(TARGET).nro
