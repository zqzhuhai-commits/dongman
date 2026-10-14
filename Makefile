TARGET := dongman

SOURCES := .

include $(DEVKITPRO)/libnx/switch_rules

all: $(TARGET).nro
