#define F_CPU 1000000UL

#include <avr/io.h>
#include <util/delay.h>
#include <avr/interrupt.h>

void initADC() {
	ADCSRA |= ((1 << ADPS1) | (1 << ADPS0)); // sets frequency to 1000/8 kHz
	ADMUX  |= (1 << REFS0); // Reference set to AVcc with capacitor at Aref
	ADCSRA |= (1 << ADFR); // set free running mode
	ADMUX  |= (1 << ADLAR); // operate ADC in 8 bit mode
	ADCSRA |= (1 << ADEN); // enable ADC
	ADCSRA |= (1 << ADSC); // start ADC conversion
	ADCSRA |= (1 << ADIE); // enable interrupts
	sei();
}

int main( void ) {
	
	DDRD  |= 0xFF; 
	initADC();
	while( 1 ) {
	}
} 

ISR( ADC_vect ) {
	PORTD = 0x00;
	int num = ADCH / 32;
	for( int i=0; i<num; i++ ) {
		PORTD |= (1 << i);
	}	
}