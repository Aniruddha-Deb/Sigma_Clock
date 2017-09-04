#define F_CPU 1000000UL

#define A_HR  PORTD4
#define B_HR  PORTD5
#define C_HR  PORTD6
#define D_HR  PORTD7

#define A_MIN PORTC2
#define B_MIN PORTC3
#define C_MIN PORTC4
#define D_MIN PORTC5

#define EN    PORTD3
#define SEC   PORTD0

#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>

unsigned volatile long numClicks  = 0;

unsigned volatile int seconds = 0;
unsigned volatile int minutes = 0;
unsigned volatile int hours   = 0;

// SETUP FUNCTIONS //////////////////////////////////////////////////////////

void waitForTimer() {
	// wait till registers are ready to be written to	
	while ((ASSR & ((1 << OCR2UB) | (1 << TCR2UB) | (1 << TCN2UB)))); 
}

void setupTimer() {
	// set timer2 to use the external clock on pins TOSC1 and TOSC2
	ASSR |= 1 << AS2; 
	waitForTimer();
	// set operation mode to CTC
	TCCR2 |= (1<<WGM21); 
	// set prescaler of 1024
	TCCR2 |= ((1<<CS22)|(1<<CS21)|(1<<CS20)); 
	// set compare value of 16
	OCR2 = 16; 
	// set the interrupt flag
	TIMSK |= (1 << OCIE2); 
}

void stopTimer() {
	waitForTimer();
	// stop timer operation
	TCCR2 &= ~((1<<CS22)|(1<<CS21)|(1<<CS20)); 	// set timer counter to 0	TCNT2 = 0; 
}

void restartTimer() {
	waitForTimer();
	// restart timer with prescaler of 1024
	TCCR2 |= ((1<<CS22)|(1<<CS21)|(1<<CS20));
}

void setupInterrupt() {
	// setup interrupt on INT0 pin
	GICR |= (1 << INT0);
	// trigger interrupt on falling edge
	MCUCR |= 0x03 ;	
}

void setupOutputPins() {
	// initialize DDRD to be output
	DDRD |= 0xFF;
	// initialize DDRC to be output
	DDRC |= 0xFF;
	DDRB |= 0xFF;
	// set the seconds LED high
	PORTC |= (1 << SEC);
}

void setupADC() {
	// TODO setup the ADC
}

void setup() {
	setupTimer();
	setupADC();
	setupOutputPins();
	setupInterrupt();
	sei();
}

// INTERRUPTS //////////////////////////////////////////////////////////////////////

ISR( TIMER2_COMP_vect ) {
	PORTD ^= (1 << SEC);
	numClicks++;
	if( numClicks >= (86400*2) ) {
		numClicks = 0;
	}
	long numSeconds = numClicks/2;
	int numMinutes = numSeconds/60;
	int numHours = numMinutes/60;
	
	seconds = numSeconds%60;
	minutes = numMinutes%60;
	hours = numHours;
}

volatile int timeIsBeingSet = 0;

ISR( INT0_vect ) {
	if( !(timeIsBeingSet) ) {
		PORTB |= (1 << PORTB1);
		timeIsBeingSet = 1;
		stopTimer();
		// TODO set the time using the ADC here.
	}
	else {
		PORTB &= ~(1 << PORTB1);
		timeIsBeingSet = 0;
		restartTimer();
	}
}

// MAIN METHOD /////////////////////////////////////////////////////////////////////

int main(void) {

	setup();
	
	while( 1 ) {
		// TODO refactor this junk into methods.
		int minutesUnitsDigit = minutes%10;
		int minutesTensDigit = minutes/10;
		
		int hoursUnitsDigit = hours%10;
		int hoursTensDigit = hours/10;
		
		PORTD &= ~( (1 << A_HR) | (1 << B_HR) | (1 << C_HR) | (1 << D_HR) );
		PORTD |= (hoursUnitsDigit << 4);
		PORTC &= ~( (1 << A_MIN) | (1 << B_MIN) | (1 << C_MIN) | (1 << D_MIN) );
		PORTC |= (minutesUnitsDigit << 2);
		PORTD ^= (1 << EN);
		_delay_ms( 8 );
		PORTD &= ~( (1 << A_HR) | (1 << B_HR) | (1 << C_HR) | (1 << D_HR) );
		PORTD |= (hoursTensDigit << 4);
		PORTC &= ~( (1 << A_MIN) | (1 << B_MIN) | (1 << C_MIN) | (1 << D_MIN) );
		PORTC |= (minutesTensDigit << 2);
		PORTD ^= (1 << EN);
		_delay_ms( 8 );
	}
}

