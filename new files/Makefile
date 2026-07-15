# Makefile for tfirm firmware (LUFA + MAX3421E + Passthrough)
# Target: Arduino Leonardo (ATmega32U4) with USB Host Shield
# Uses LUFA library (in lufa/ subdirectory)

# Compiler and toolchain
MCU          = atmega32u4
ARCH         = AVR8
F_CPU        = 16000000
F_USB        = 16000000
OPTIMIZATION = s
TARGET       = MousePassthrough
SRC          = $(TARGET).c Descriptors.c max3421e.c $(LUFA_SRC)
LUFA_PATH    = lufa/LUFA
CC           = avr-gcc
OBJCOPY      = avr-objcopy
OBJDUMP      = avr-objdump
SIZE         = avr-size
AVRDUDE      = avrdude_bin/avrdude.exe

# LUFA source files (minimal set)
LUFA_SRC = \
	$(LUFA_PATH)/Drivers/USB/Core/AVR8/Device_AVR8.c \
	$(LUFA_PATH)/Drivers/USB/Core/AVR8/Endpoint_AVR8.c \
	$(LUFA_PATH)/Drivers/USB/Core/AVR8/Host_AVR8.c \
	$(LUFA_PATH)/Drivers/USB/Core/AVR8/Pipe_AVR8.c \
	$(LUFA_PATH)/Drivers/USB/Core/AVR8/USBController_AVR8.c \
	$(LUFA_PATH)/Drivers/USB/Core/AVR8/USBInterrupt_AVR8.c \
	$(LUFA_PATH)/Drivers/USB/Core/ConfigDescriptors.c \
	$(LUFA_PATH)/Drivers/USB/Core/DeviceStandardReq.c \
	$(LUFA_PATH)/Drivers/USB/Core/Events.c \
	$(LUFA_PATH)/Drivers/USB/Core/HostStandardReq.c \
	$(LUFA_PATH)/Drivers/USB/Core/USBTask.c

# Compiler flags
CFLAGS = -mmcu=$(MCU) -DF_CPU=$(F_CPU)UL -DF_USB=$(F_USB)UL \
         -Os -fno-move-loop-invariants -fno-tree-scev-cprop \
         -fno-inline-small-functions -fno-tree-reassoc \
         -fno-tree-loop-optimize -fno-tree-switch-conversion \
         -fno-math-errno -fno-split-wide-types \
         -Wall -Wextra -Werror -Wno-unused-parameter \
         -ffunction-sections -fdata-sections \
         -I. -I$(LUFA_PATH)/.. \
         -DUSE_LUFA_CONFIG_HEADER \
         -DARCH=ARCH_$(ARCH) \
         -DUSB_VID=0x046D -DUSB_PID=0xC09B

LDFLAGS = -mmcu=$(MCU) -Wl,-Map=$(TARGET).map,--cref -Wl,--relax -Wl,--gc-sections

# Default target
all: $(TARGET).hex

# Compile C files
%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

# Link
$(TARGET).elf: $(SRC:.c=.o)
	$(CC) $(LDFLAGS) $^ -o $@
	$(SIZE) $@

# Generate hex
$(TARGET).hex: $(TARGET).elf
	$(OBJCOPY) -O ihex -R .eeprom $< $@

# Flash (requires avrdude and PORT specified)
flash: $(TARGET).hex
	$(AVRDUDE) -p $(MCU) -P $(PORT) -c avr109 -U flash:w:$(TARGET).hex:i

# Clean
clean:
	rm -f *.o *.elf *.hex *.map

# Dependencies
$(SRC:.c=.o): $(LUFA_PATH)/Drivers/USB/USB.h