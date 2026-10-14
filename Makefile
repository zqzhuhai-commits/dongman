TARGET := dongman

SOURCES := main.c
OFILES := main.o

include $(DEVKITPRO)/libnx/switch_rules

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

all: $(TARGET).nro
