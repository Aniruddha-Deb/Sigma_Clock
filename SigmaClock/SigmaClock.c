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

unsigned const long NUM_MAX_CLICKS         = (86400*2);
unsigned const int  NUM_CLICKS_IN_AN_HOUR  = (3600*2);
unsigned const int  NUM_CLICKS_IN_A_MINUTE = (60*2);
         const char SENSITIVITY            = 6;

unsigned volatile long numClicks  = 0;

unsigned volatile int minutes = 0;
unsigned volatile int hours   = 0;

volatile char timeUpdated = 0;
volatile char timeIsBeingSet = 0;
volatile char hrPlusCntr = 0;
volatile char hrMinusCntr = 0;
volatile char minPlusCntr = 0;
volatile char minMinusCntr = 0;

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
	// trigger interrupt on rising edge
	MCUCR |= ((1 << ISC01) | (1 << ISC00)) ;	
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
	// set the time set LED to low
	PORTC &= ~(1 << TSET);
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
	if( numClicks >= NUM_MAX_CLICKS ) {
		numClicks = 0;
	}
}

void incrementNumClicksBy( int num ) {
	if( (numClicks+num) >= NUM_MAX_CLICKS ) {
		numClicks = (numClicks+num)-NUM_MAX_CLICKS;
	}
	else {
		numClicks = numClicks + num;
	}
}

void decrementNumClicksBy( int num ) {
	long temp = (numClicks-num);
	if( temp < 0 ) {
		numClicks = NUM_MAX_CLICKS-(num-numClicks);
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

void updateUnitsDigit( int minutesUnitsDigit, int hoursUnitsDigit ) {
	PORTD &= ~( (1 << A_HR) | (1 << B_HR) | (1 << C_HR) | (1 << D_HR) );
	PORTD |= (hoursUnitsDigit << 4);
	PORTC &= ~( (1 << A_MIN) | (1 << B_MIN) | (1 << C_MIN) | (1 << D_MIN) );
	PORTC |= (minutesUnitsDigit << 2);
	PORTD ^= (1 << EN);
} 

void updateTensDigit( int minutesTensDigit, int hoursTensDigit ) {
	PORTD &= ~( (1 << A_HR) | (1 << B_HR) | (1 << C_HR) | (1 << D_HR) );
	PORTD |= (hoursTensDigit << 4);
	PORTC &= ~( (1 << A_MIN) | (1 << B_MIN) | (1 << C_MIN) | (1 << D_MIN) );
	PORTC |= (minutesTensDigit << 2);
	PORTD ^= (1 << EN);
}

void checkHoursPlusButton() {
	if( bit_is_set( PINB, H_PLUS ) ) {
		hrPlusCntr++;
		if( hrPlusCntr > SENSITIVITY ) {
			hrPlusCntr = 0;
			timeUpdated = 1;
			incrementNumClicksBy( NUM_CLICKS_IN_AN_HOUR );
		}
	}	
}

void checkHoursMinusButton() {
	if( bit_is_set( PINB, H_MINUS ) ) {
		hrMinusCntr++;
		if( hrMinusCntr > SENSITIVITY ) {
			hrMinusCntr = 0;
			timeUpdated = 1;
			decrementNumClicksBy( NUM_CLICKS_IN_AN_HOUR );
		}
	}
}

void checkHoursUpdateButtons() {
	checkHoursPlusButton();
	checkHoursMinusButton();
}

void checkMinutesPlusButton() {
	if( bit_is_set( PIND, M_PLUS ) ) {
		minPlusCntr++;
		if( minPlusCntr > SENSITIVITY ) {
			minPlusCntr = 0;
			timeUpdated = 1;
			incrementNumClicksBy( NUM_CLICKS_IN_A_MINUTE );
		}
	}	
}

void checkMinutesMinusButton() {
	if( bit_is_set( PINB, M_MINUS ) ) {
		minMinusCntr++;
		if( minMinusCntr > SENSITIVITY ) {
			minMinusCntr = 0;
			timeUpdated = 1;
			decrementNumClicksBy( NUM_CLICKS_IN_A_MINUTE );
		}
	}
}

void checkMinutesUpdateButtons() {
	checkMinutesPlusButton();
	checkMinutesMinusButton();
}

void checkTimeUpdateButtons() {
	checkHoursUpdateButtons();
	checkMinutesUpdateButtons();
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
	
	int minutesUnitsDigit, minutesTensDigit;
	int hoursUnitsDigit, hoursTensDigit;
	
	while( 1 ) {
		minutesUnitsDigit = minutes%10;
		minutesTensDigit = minutes/10;
		
		hoursUnitsDigit = hours%10;
		hoursTensDigit = hours/10;
		
		updateUnitsDigit( minutesUnitsDigit, hoursUnitsDigit );
		_delay_ms( 8 );
		updateTensDigit( minutesTensDigit, hoursTensDigit );
		_delay_ms( 8 );
		
		if( timeIsBeingSet ) {			
			checkTimeUpdateButtons();	
			if( timeUpdated ) {		
				updateCounters();
			}
		}
	}
}

