#define F_CPU 1000000UL

#define A_HR  PORTD4
#define B_HR  PORTD5
#define C_HR  PORTD6
#define D_HR  PORTD7

#define A_MIN PORTC2
#define B_MIN PORTC3
#define C_MIN PORTC4
#define D_MIN PORTC5

#define H_MINUS PORTB0
#define H_PLUS  PORTB1
#define M_MINUS PORTB2
#define M_PLUS  PORTD1

#define TSET  PORTC0

#define EN    PORTD3
#define SEC   PORTD0

#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>

unsigned volatile long numClicks  = 0;

unsigned volatile const long numMaxClicks        = (86400*2);
unsigned volatile const long numMinClicks        = 0;
unsigned volatile const int  numClicksInAnHour   = (3600*2);
unsigned volatile const int  numClicksInAMinute  = (60*2);

unsigned volatile int minutes = 0;
unsigned volatile int hours   = 0;

volatile char timeIsBeingSet = 0;
volatile char hrPlusPrevState = 0;
volatile char hrMinusPrevState = 0;
volatile char minPlusPrevState = 0;
volatile char minMinusPrevState = 0;

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
	// initialize DDRD
	DDRD |= ~(1 << M_PLUS);
	// initialize DDRC
	DDRC |= 0xFF;
	// initialize DDRB
	DDRB |= ~((1 << M_MINUS) | (1 << H_PLUS) | (1 << H_MINUS));
	// set the seconds LED high
	PORTC |= (1 << SEC);
}

void setup() {
	setupTimer();
	setupOutputPins();
	setupInterrupt();
	sei();
}

// UTILITY METHODS /////////////////////////////////////////////////////////////////

void incrementNumClicks() {
	numClicks++;
	if( numClicks >= numMaxClicks ) {
		numClicks = 0;
	}
}

void incrementNumClicksBy( int num ) {
	if( (numClicks+num) >= numMaxClicks ) {
		numClicks = (numClicks+num)-numMaxClicks;
	}
	else {
		numClicks = numClicks + num;
	}
}

void decrementNumClicksBy( int num ) {
	long temp = (numClicks-num);
	if( temp < 0 ) {
		numClicks = numMaxClicks-(num-numClicks);
	}
	else {
		numClicks = numClicks - num;
	}
}

void updateCounters() {
	long numSeconds = numClicks/2;
	int numMinutes = numSeconds/60;
	int numHours = numMinutes/60;
	
	minutes = numMinutes%60;
	hours = numHours;	
}

// INTERRUPTS //////////////////////////////////////////////////////////////////////

ISR( TIMER2_COMP_vect ) {
	PORTD ^= (1 << SEC);
	incrementNumClicks();
	updateCounters();
}

ISR( INT0_vect ) {
	if( !(timeIsBeingSet) ) {
		PORTC |= (1 << TSET);
		timeIsBeingSet = 1;
		stopTimer();
	}
	else {
		PORTC &= ~(1 << TSET);
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
		
		if( timeIsBeingSet ) {
			
			if( bit_is_set( PINB, H_PLUS ) ) {
				incrementNumClicksBy( numClicksInAnHour );
			}
			if( bit_is_set( PINB, H_MINUS ) ) {
				decrementNumClicksBy( numClicksInAnHour );
			}
			if( bit_is_set( PIND, M_PLUS ) ) {
				incrementNumClicksBy( numClicksInAMinute );
			}
			if( bit_is_set( PINB, M_MINUS ) ) {
				decrementNumClicksBy( numClicksInAMinute );
			}
			updateCounters();			
		}
	}
}

