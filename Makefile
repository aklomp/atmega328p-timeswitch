CROSS	?= /opt/cross/avr/bin/avr-
CC	 = $(CROSS)gcc
OBJCOPY	 = $(CROSS)objcopy
AVRDUDE	 = avrdude

MCU	= atmega328p
F_CPU	= 8000000

TARGET	= main

COMMON_FLAGS = -Os -std=c99 -DF_CPU=$(F_CPU)UL -mmcu=$(MCU)

CFLAGS	 = $(COMMON_FLAGS)
CFLAGS	+= -funsigned-char -funsigned-bitfields -fpack-struct -fshort-enums
CFLAGS	+= -Wall -Wstrict-prototypes -Wa,-adhlns=$(<:.c=.lst)

LDFLAGS	 = $(COMMON_FLAGS)
LDFLAGS	+= -Wl,-Map=$(TARGET).map,--cref

SRCS	= $(wildcard *.c)
OBJS	= $(SRCS:.c=.o)

.PHONY: clean flash

$(TARGET).hex: $(TARGET).elf
	$(OBJCOPY) -O ihex -R .eeprom $^ $@

$(TARGET).elf: $(OBJS)
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $^

%.o: %.c
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ -c $^

# We program the Atmega328P by placing it into an Arduino Uno board, then
# connecting an Atmel JTAGICE MkII to the ICSP header on the edge of the board
# with jumper wires. We lower the ISP bitrate to avoid timeouts. Unfortunately
# we can't use the native Arduino bootloader because it can't flash the fuses.
flash: $(TARGET).hex
	$(AVRDUDE) -F -c jtag2isp -B 100 -p $(MCU) -e -U flash:w:$(TARGET).hex

# The lfuse is set to 0xC3 to use the internal 128 KHz oscillator as clock.
# The efuse is set to 0xFF to disable brown-out detection.
flash-fuses:
	$(AVRDUDE) -F -c jtag2isp -B 100 -p $(MCU) -u -U lfuse:w:0xC3:m
	sleep 1
	$(AVRDUDE) -F -c jtag2isp -B 100 -p $(MCU) -u -U efuse:w:0xFF:m

clean:
	$(RM) $(OBJS) $(TARGET).{hex,elf,lst,map,d}
