# Atmel Atmega328P as a super low power on/off timer switch

This firmware programs the Atmega328P to deep-sleep for 872 seconds, turn on
pin B0 for 24 seconds, then turn off the pin and go back to deep sleep. This
cycle repeats indefinitely. This creates a timer switch with a duty cycle of 24
seconds and a period of almost 15 minutes. The durations are easily changeable
in the code, but must be multiples of 8 seconds since that's the maximum
duration of the watchdog timeout. You can shorten the timeout, but it means
waking more often.

We try to get the lowest possible current consumption in deep sleep by turning
off every possible peripheral and clock, configuring the sleep mode to
power-down, and using the Watchdog interrupt as a wakeup. That's the lowest the
chip will go. By burning hardware fuses, we configure the chip to run off the
low-power 128 KHz internal oscillator instead of an external crystal to save
even more power. The chip can now run completely standalone without external
circuitry. Further power can be saved by running the chip from a low voltage
source, which can be as low as 2V.

Measured power usage is 28µA @ 5V, and 24µA @ 3.3V.

The goal was to make a simple "alarm clock" that can turn on a power-hungry
ESP8266 board every 15 minutes so that it can take temperature measurements and
send them over WiFi, then turning it off again. The ESP8266 is not exactly a
low power device, so the Atmega drives a MOSFET that switches it hard on and
off.

The Atmega328P in DIP package is programmed seated in an Arduino Uno board
through the ICSP header. It appears that the Arduino bootloader can't or won't
program the chip fuses, so we use an Atmel JTAGICE MkII programmer to burn the
fuses for us. You can easily change the programmer type in the Makefile.

You'll need AVR-GCC to compile this code.

```sh
# make			# Compile the code
# make flash		# Flash it to the device
# make flash-fuses	# Burn the hardware fuses
```
