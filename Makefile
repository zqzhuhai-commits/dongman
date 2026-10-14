TARGET := dongman

SOURCES := source/main.c

PORTLIBS := $(DEVKITPRO)/portlibs/switch

include $(DEVKITPRO)/libnx/switch_rules

CFLAGS := -Iinclude -I$(DEVKITPRO)/libnx/include
LIBS := -L$(DEVKITPRO)/libnx/lib -lnx

all: $(TARGET).nro

$(TARGET).elf: $(SOURCES)
	$(CC) $(CFLAGS) -o $@ $(SOURCES) $(LIBS)

$(TARGET).nro: $(TARGET).elf
	nacptool --create "AnimeRepublic" "zqzhuhai" "1.0.0" $(TARGET).nacp
	elf2nro $(TARGET).elf $(TARGET).nro --icon=$(DEVKITPRO)/libnx/default_icon.jpg --nacp=$(TARGET).nacp

clean:
	rm -f *.o *.elf *.nro *.nacp *.d
