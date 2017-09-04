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
unsigned volatile long numSeconds = 0;
unsigned volatile int numMinutes = 0;
unsigned volatile int numHours   = 0;

unsigned volatile int seconds = 0;
unsigned volatile int minutes = 0;
unsigned volatile int hours   = 0;

int main(void) {
	
	ASSR |= 1 << AS2;
	TCCR2 |= (1<<WGM21);
	TCCR2 |= ((1<<CS22)|(1<<CS21)|(1<<CS20));
	OCR2 = 16;
	TIMSK |= (1 << OCIE2);
	
	DDRD |= 0xFF;
	DDRC |= 0xFF;
	
	PORTC |= (1 << SEC);
	sei();

	while( 1 ) {
		int secondsUnitsDigit = seconds%10;
		int secondsTensDigit = seconds/10;
		
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

ISR( TIMER2_COMP_vect ) {
	PORTD ^= (1 << SEC);
	numClicks++;
	if( numClicks >= (86400*2) ) {
		numClicks = 0;
	}
	numSeconds = numClicks/2;
	numMinutes = numSeconds/60;
	numHours = numMinutes/60;
	
	seconds = numSeconds%60;
	minutes = numMinutes%60;
	hours = numHours;
}