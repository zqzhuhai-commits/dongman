TARGET := dongman
BUILD := build

SOURCES := \
    source/main.c \
    source/http.c \
    source/parser.c \
    source/ui.c \
    source/player.c

include $(DEVKITPRO)/libnx/switch_rules

CFLAGS := -Iinclude -I$(PORTLIBS)/include
LDFLAGS := -L$(PORTLIBS)/lib

LIBS := -lSDL2 -lSDL2_image -lcurl -lm -lz -lnx -lstdc++

OFILES := $(SOURCES:.c=.o)

all: $(BUILD)/$(TARGET).nro

$(BUILD)/%.o: %.c
	@mkdir -p $(BUILD)/$(dir $<)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD)/$(TARGET).elf: $(OFILES)
	@mkdir -p $(BUILD)
	$(CC) $(LDFLAGS) -o $@ $(OFILES) $(LIBS)

$(BUILD)/$(TARGET).nro: $(BUILD)/$(TARGET).elf
	nacptool --create "动漫共和国" "zqzhuhai" "1.0.0" $(BUILD)/$(TARGET).nacp
	elf2nro $(BUILD)/$(TARGET).elf $(BUILD)/$(TARGET).nro --icon=$(DEVKITPRO)/libnx/default_icon.jpg --nacp=$(BUILD)/$(TARGET).nacp

clean:
	rm -rf build
