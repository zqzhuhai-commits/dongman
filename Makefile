TARGET := dongman

include $(DEVKITPRO)/libnx/switch_rules

CFLAGS := -I$(DEVKITPRO)/libnx/include
LDFLAGS := -L$(DEVKITPRO)/libnx/lib -lnx -T$(DEVKITPRO)/libnx/switch.ld

all: $(TARGET).nro

$(TARGET).elf: main.c
	$(CC) $(CFLAGS) -o $@ main.c $(LDFLAGS)

$(TARGET).nro: $(TARGET).elf
	elf2nro $(TARGET).elf $(TARGET).nro $(NROFLAGS)
