#include <stdbool.h>
#include <stdint.h>
#include <avr/io.h>
#include <avr/interrupt.h>

#define COUNT_SLEEP	109	// 8 * 109 = 872 seconds
#define COUNT_ACTIVE	3	// 8 *   3 =  24 seconds

ISR (WDT_vect)
{
	// Watchdog timer interrupt handler. Re-enable the interrupt.
	WDTCSR |= _BV(WDIE);
}

// Enter deep sleep mode.
static void sleep (void)
{
	// Set the Sleep Mode register to Power Down mode.
	SMCR = _BV(SM1) | _BV(SE);

	// The brown-out detector is probably not even enabled, because it
	// requires special fuse settings. However, to be thorough, let's
	// disable it anyway. It will only be disabled during sleep if a timed
	// sequence is followed.

	// Enable BODS and BODSE.
	MCUCR = _BV(BODS) | _BV(BODSE);

	// Within four cycles, set BODS to one and BODSE to zero.
	MCUCR = _BV(BODS);

	// BODS becomes active after three cycles, for a period of three cycles.
	// Issue the sleep instruction within a three-cycle window.
	__asm__ volatile (
		"nop   \n\t"
		"nop   \n\t"
		"nop   \n\t"
		"nop   \n\t"
		"sleep \n\t"
	);
}

static void watchdog_set (void)
{
	// Start the timed sequence.
	WDTCSR = _BV(WDE) | _BV(WDCE);

	// Set the prescaler to 8 seconds and enable the interrupt.
	WDTCSR = _BV(WDE) | _BV(WDP3) | _BV(WDP0) | _BV(WDIE);
}

static void init (void)
{
	// Set all ports to output mode.
	DDRB = 1;
	DDRC = 1;
	DDRD = 1;

	// Set all outputs low.
	PORTB = 0;
	PORTC = 0;
	PORTD = 0;

	// Disable the Analog Comparator.
	ACSR = _BV(ACD);

	// Disable the digital input buffers.
	DIDR0 = _BV(ADC5D)
	      | _BV(ADC4D)
	      | _BV(ADC3D)
	      | _BV(ADC2D)
	      | _BV(ADC1D)
	      | _BV(ADC0D)
	      ;

	DIDR1 = _BV(AIN1D)
	      | _BV(AIN0D)
	      ;

	// Disable the ADC before shutting it down.
	ADCSRA = 0;

	// Reduce power by shutting down peripherals.
	PRR = _BV(PRADC)
	    | _BV(PRUSART0)
	    | _BV(PRSPI)
	    | _BV(PRTIM0)
	    | _BV(PRTIM1)
	    | _BV(PRTIM2)
	    | _BV(PRTWI)
	    ;

	// Set the Watchdog Timer to wake the core every 8 seconds.
	watchdog_set();

	// Enable interrupts.
	sei();
}

static bool step_active (void)
{
	static uint8_t count = 0;

	// Remain active until the period is over.
	if (++count != COUNT_ACTIVE)
		return true;

	// Reset port B0 and the counter.
	PORTB = 0;
	count = 0;
	return false;
}

static bool step_sleep (void)
{
	static uint8_t count = 0;

	// Remain dormant until the period is over.
	if (++count != COUNT_SLEEP)
		return false;

	// Set port B0, reset the counter.
	PORTB = 1;
	count = 0;
	return true;
}

static void loop (void)
{
	for (bool active = false; ; active = active ? step_active() : step_sleep())
		sleep();
}

// Entry point.
int main (void)
{
	init();
	loop();
}
