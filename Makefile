TARGET := dongman

include $(DEVKITPRO)/libnx/switch_rules

all: $(TARGET).nro

$(TARGET).elf: main.c
	$(CC) $(CFLAGS) -o $@ main.c

$(TARGET).nro: $(TARGET).elf
	elf2nro $(TARGET).elf $(TARGET).nro $(NROFLAGS)
