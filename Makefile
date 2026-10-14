TARGET := dongman

SOURCES := main.c
OFILES := main.o

include $(DEVKITPRO)/libnx/switch_rules

all: $(TARGET).nro
