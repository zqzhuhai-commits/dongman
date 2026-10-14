TARGET := dongman

include $(DEVKITPRO)/libnx/switch_rules

CFLAGS := -I$(DEVKITPRO)/libnx/include
LIBS := -L$(DEVKITPRO)/libnx/lib -lnx

all: $(TARGET).nro

main.o: main.c
	$(CC) $(CFLAGS) -c main.c -o main.o

$(TARGET).elf: main.o
	$(CC) -o $@ main.o $(LIBS)

$(TARGET).nro: $(TARGET).elf
	elf2nro $(TARGET).elf $(TARGET).nro $(NROFLAGS)
