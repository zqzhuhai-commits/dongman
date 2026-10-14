TARGET := dongman

SOURCES := \
    source/main.c \
    source/http.c \
    source/parser.c \
    source/ui.c \
    source/player.c

export PORTLIBS := $(DEVKITPRO)/portlibs/switch

include $(DEVKITPRO)/libnx/switch_rules

CFLAGS := -Iinclude $(PORTLIBS_CFLAGS)
LIBS := $(PORTLIBS_LIBS) -lSDL2 -lSDL2_image -lcurl -lm -lz -lnx -lstdc++

all: $(TARGET).nro

$(TARGET).elf: $(SOURCES)
	$(CC) $(CFLAGS) -o $@ $(SOURCES) $(LIBS)

$(TARGET).nro: $(TARGET).elf
	nacptool --create "AnimeRepublic" "zqzhuhai" "1.0.0" $(TARGET).nacp
	elf2nro $(TARGET).elf $(TARGET).nro --icon=$(DEVKITPRO)/libnx/default_icon.jpg --nacp=$(TARGET).nacp

clean:
	rm -f *.o *.elf *.nro *.nacp *.d
