TARGET := dongman

SOURCES := source/main.c

include $(DEVKITPRO)/libnx/switch_rules

all: $(TARGET).nro
