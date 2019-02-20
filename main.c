#include <stdbool.h>
#include <stdint.h>
#include <avr/io.h>
#include <avr/interrupt.h>

#define COUNT_SLEEP	109	// 8 * 109 = 872 seconds
#define COUNT_ACTIVE	3	// 8 *   3 =  24 seconds

ISR (WDT_vect)
{
	// Watchdog timer interrupt handler. Re-enable the interrupt:
	WDTCSR |= _BV(WDIE);
}

// Enter deep sleep mode.
static void
sleep (void)
{
	// Set the Sleep Mode register to Power Down mode:
	SMCR = _BV(SM1) | _BV(SE);

	// The brown-out detector is probably not even enabled, because it
	// requires special fuse settings. However, to be thorough, let's
	// disable it anyway. It will only be disabled during sleep if a timed
	// sequence is followed.

	// Enable BODS and BODSE:
	MCUCR = _BV(BODS) | _BV(BODSE);

	// Within four cycles, set BODS to one and BODSE to zero:
	MCUCR = _BV(BODS);

	// BODS becomes active after three cycles, for a period of three cycles.
	// Issue sleep instruction within three-cycle window:
	__asm volatile (
		"nop\n\t"
		"nop\n\t"
		"nop\n\t"
		"nop\n\t"
		"sleep\n\t");
}

static void
watchdog_set (void)
{
	// Start timed sequence:
	WDTCSR = _BV(WDE) | _BV(WDCE);

	// Set prescaler to 8 seconds and enable interrupt:
	WDTCSR = _BV(WDE) | _BV(WDP3) | _BV(WDP0) | _BV(WDIE);
}

static void
init (void)
{
	// Set all ports to output:
	DDRB = 1;
	DDRC = 1;
	DDRD = 1;

	// Set all outputs low:
	PORTB = 0;
	PORTC = 0;
	PORTD = 0;

	// Disable Analog Comparator:
	ACSR |= _BV(ACD);

	// Disable digital input buffers:
	DIDR1 = _BV(AIN1D) | _BV(AIN0D);
	DIDR0 = _BV(ADC5D)
	      | _BV(ADC4D)
	      | _BV(ADC3D)
	      | _BV(ADC2D)
	      | _BV(ADC1D)
	      | _BV(ADC0D)
	      ;

	// Must disable ADC before shutting it down:
	ADCSRA &= ~_BV(ADEN);

	// Reduce power by shutting down peripherals:
	PRR = _BV(PRADC)
	    | _BV(PRUSART0)
	    | _BV(PRSPI)
	    | _BV(PRTIM0)
	    | _BV(PRTIM1)
	    | _BV(PRTIM2)
	    | _BV(PRTWI)
	    ;

	// Set the Watchdog Timer to wake us every 8 seconds:
	watchdog_set();

	// Enable interrupts:
	sei();
}

static void
loop (void)
{
	for (;;)
	{
		static bool active = false;
		static uint8_t count = 0;

		sleep();

		if (active)
		{
			// Wait until the active period is over:
			if (++count != COUNT_ACTIVE)
				continue;

			// Reset port B0 and counter:
			PORTB  = 0;
			count  = 0;
			active = false;
			continue;
		}

		// Wait until the inactive period is over:
		if (++count != COUNT_SLEEP)
			continue;

		// Set port B0, reset counter:
		PORTB  = 1;
		count  = 0;
		active = true;
	}
}

// Entry point.
int
main (void)
{
	init();
	loop();
}
