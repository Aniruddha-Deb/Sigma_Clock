#ifndef __LCD_H__
#define __LCD_H__

#include <inttypes.h>
#include <avr/io.h>
#include <util/delay.h>
#include <avr/sfr_defs.h>
#include <stdlib.h>

#define LCD_OUT_REG    PORTC
#define LCD_IN_REG     PINC
#define LCD_DDR        DDRC

#define LCD_RS_PIN     0  // Register select pin in LCD_OUT_REG
#define LCD_RW_PIN     1  // Read/Write select pin in LCD_OUT_REG
#define LCD_E_PIN      2  // Enable pin in LCD_OUT_REG
#define LCD_D4_PIN     4  // Data line pin 1 in LCD_OUT_REG
#define LCD_D5_PIN     5  // Data line pin 2 in LCD_OUT_REG
#define LCD_D6_PIN     6  // Data line pin 3 in LCD_OUT_REG
#define LCD_D7_PIN     7  // Data line pin 4 in LCD_OUT_REG

#define LCD_BF_PIN     LCD_D7_PIN // Buffer overflow pin in LCD_OUT_REG

#define LCD_L1_START_DDRAM_ADDR 0x00 // DDRAM start address for LCD display line 1
#define LCD_L2_START_DDRAM_ADDR 0x40 // DDRAM start address for LCD display line 2
#define LCD_L3_START_DDRAM_ADDR 0x14 // DDRAM start address for LCD display line 3
#define LCD_L4_START_DDRAM_ADDR 0x54 // DDRAM start address for LCD display line 4

#define LCD_DELAY_E_PULSE() _delay_us(1)

// Sets the MCU port in read or write mode. Note that in the read mode, we are
// only interested in reading from D4-7 and hence we enable only those pins
// as inputs.
#define SET_LCD_PORT_IN_WRITE_MODE() ( LCD_DDR = 0xFF )
#define SET_LCD_PORT_IN_READ_MODE()  ( LCD_DDR = 0x0F )

// LCD E pin ( 1=High, 0=Low )
#define LCD_SET_ENABLE_HIGH()    ( LCD_OUT_REG |=  _BV( LCD_E_PIN  ) )
#define LCD_SET_ENABLE_LOW()     ( LCD_OUT_REG &= ~_BV( LCD_E_PIN  ) )

// LCD RS pin ( 1=Data mode, 0=Command mode)
#define LCD_SET_DATA_MODE()      ( LCD_OUT_REG |=  _BV( LCD_RS_PIN ) )
#define LCD_SET_CMD_MODE()       ( LCD_OUT_REG &= ~_BV( LCD_RS_PIN ) )

// LCD RW pin ( 1=Read mode, 0=Write mode)
#define LCD_SET_READ_MODE()      ( LCD_OUT_REG |=  _BV( LCD_RW_PIN ) )
#define LCD_SET_WRITE_MODE()     ( LCD_OUT_REG &= ~_BV( LCD_RW_PIN ) )

#define WAIT_IF_LCD_BUSY()   while( readLCDBusyFlag() ){}
#define LCD_CLR_SCR()        writeCommand( 0x01 ) ;
	
/**
 * Returns 1 if LCD is in busy state, else 0
 */
uint8_t readLCDBusyFlag(void) {
	
	// Note that we are operating in the 4 bit mode and hence the 
	// when we try to read the LCD in the command mode, we need to
	// pick the data in two nibbles - higher order first. Each
	// nibble is read on a prolonged Enable high pulse.
	uint8_t data = 0x00 ;
	
	LCD_SET_CMD_MODE() ;
	LCD_SET_READ_MODE() ;
	
	// Set the MCU port into read mode so that we can pick up the data
	SET_LCD_PORT_IN_READ_MODE() ;
	
	// Grab the high nibble
	LCD_SET_ENABLE_HIGH() ;
	LCD_DELAY_E_PULSE() ;
	data = LCD_IN_REG << 4 ;
	LCD_SET_ENABLE_LOW() ;
	
	// Stretch the low pulse a bit
	LCD_DELAY_E_PULSE() ;

	// Grab the low nibble
	LCD_SET_ENABLE_HIGH() ;
	LCD_DELAY_E_PULSE() ;
	data |= (LCD_IN_REG & 0x0F) ;
	LCD_SET_ENABLE_LOW() ;
	
	// Put back the MCU port in the write mode. 
	SET_LCD_PORT_IN_WRITE_MODE() ;
	
	return (data & 0x80) >> LCD_BF_PIN;
}

/**
 * Utility function to just write the lower nibble to the LCD data bus. 
 * Note that this function doesn't care which mode the nibble is written 
 * in. It is assumed that except for the RW flag, all other settings, 
 * including busy flag checks are done prior to calling this function.
 */
void writeNibbleToLCD( uint8_t data ) {
	
	LCD_OUT_REG = ((LCD_OUT_REG & 0x0F) | (data << 4)) ;

	LCD_SET_ENABLE_HIGH() ;
	LCD_DELAY_E_PULSE() ;
	LCD_SET_ENABLE_LOW() ;
	LCD_DELAY_E_PULSE() ;
}

/**
 * Writes the specified data to the LCD in the mode specified. This is
 * more of a refactored utility method to aid writeCommand and writeData
 * methods.
 *
 * data - The 8 bit data to be written
 * isCmd - 1 if the data is to be written as command, 0 otherwise
 */
void writeToLCD( uint8_t data, uint8_t isCmd ) {
	
	if( isCmd ) {
		LCD_SET_CMD_MODE() ;
	}
	else {
		LCD_SET_DATA_MODE() ;
	}
	LCD_SET_WRITE_MODE() ;
	
	// Write the data in two steps, nibble at a time. High first.
	// Output the upper four bits of data
	writeNibbleToLCD( (data >> 4) & 0x0F ) ;
	
	// Output the lower four bits of data
	writeNibbleToLCD( data & 0x0F ) ;
	
	_delay_us(500) ;
}

/** Write the specified command to the LCD */
void writeCommandToLCD( uint8_t cmd ) {
	WAIT_IF_LCD_BUSY() ;
	writeToLCD( cmd, 1 ) ;
}

/** Write the specified data to the LCD */
void writeDataToLCD( uint8_t data ) {
	WAIT_IF_LCD_BUSY() ;
	writeToLCD( data, 0 ) ;
}

/** 
 * Initializes the LCD. Note that this method is hardcoded for a 16x2 LCD. 
 * Yes, (sniff), its not generic enough to magically span over to bigger
 * LCDs. If there is such a need, its just a matter of tweaking the 
 * function set command and we are good to go.
 */
void initLCD() {
	
	SET_LCD_PORT_IN_WRITE_MODE() ;
	LCD_SET_CMD_MODE() ;
	LCD_SET_WRITE_MODE() ;

	_delay_ms( 15 ) ;
	writeNibbleToLCD( 0x03 ) ; // Hardcoded protocol for 4 bit interface
	
	_delay_ms( 5 ) ;
	writeNibbleToLCD( 0x03 ) ; // Hardcoded protocol for 4 bit interface
	
	_delay_us( 150 ) ;
	writeNibbleToLCD( 0x03 ) ; // Hardcoded protocol for 4 bit interface

	_delay_ms( 5 ) ;
	writeNibbleToLCD( 0x02 ) ; // Switch to 4 bit mode
	
	_delay_us( 40 ) ;
	
	writeCommandToLCD( 0x28 ) ; // Function set - 2 line, 5x8 font
	writeCommandToLCD( 0x08 ) ; // Display off, cursor off, blink off
	writeCommandToLCD( 0x0C ) ; // Display on, cursor on, blink off
	writeCommandToLCD( 0x06 ) ; // Increment cursor and don't shift screen
	writeCommandToLCD( 0x01 ) ; // Clear screen
	
	_delay_ms(10) ;
}

/** 
 * Moves the cursor DDRAM address to the specified row and column. Row 
 * and column values are zero based. Note that no error or sanity checking
 * is done on the input data.
 */
void moveLCDCursor( uint8_t row, uint8_t col ) {
	uint8_t lineBaseAddress = 0x00 ;
	if( row == 1 ) {
		lineBaseAddress = LCD_L2_START_DDRAM_ADDR ;
	}
	writeCommandToLCD( (1<<7) + lineBaseAddress + col ) ;
}

/** 
 * Writes the specified string at the current cursor position. It is assumed
 * that the string is null terminated. Also, please note that this function
 * does not do boundary detection and auto wrapping.
 */
void writeStringToLCD( char* s ) {
	while( *s ) {
		writeDataToLCD( *s ) ;
		s++ ;
	}
}

/** 
 * Writes the specified integer at the current cursor position.Please note 
 * that this function does not do boundary detection and auto wrapping.
 */
void writeIntToLCD( int val ) {
	
	char digitStr[10] ;
	itoa( val, digitStr, 10 ) ;
	writeStringToLCD( digitStr ) ;
}

/** 
 * Writes the specified character at the specified position. It is assumed
 * that the string is null terminated. Also, please note that this function
 * does not do boundary detection and auto wrapping.
 */
void writeCharToLCDatRC( uint8_t r, uint8_t c, char chr ) {
	moveLCDCursor( r, c ) ;
	writeDataToLCD( chr ) ;
}

/** 
 * Writes the specified string at the specified position. It is assumed
 * that the string is null terminated. Also, please note that this function
 * does not do boundary detection and auto wrapping.
 */
void writeStringToLCDatRC( uint8_t r, uint8_t c, char* s ) {
	moveLCDCursor( r, c ) ;
	writeStringToLCD( s ) ;
}

/** 
 * Writes the specified integer at the specified position. Please note 
 * that this function does not do boundary detection and auto wrapping.
 */
void writeIntToLCDatRC( uint8_t r, uint8_t c, int val ) {
	moveLCDCursor( r, c ) ;
	writeIntToLCD( val ) ;
}

/** 
 * Clears the specified region on the LCD with blank characters. 
 */
void clearLCDRegion( uint8_t sRow, uint8_t sCol, uint8_t eRow, uint8_t eCol ) {
	
	uint8_t rIndex = sRow ;
	uint8_t cIndex = sCol ;
	
	while( rIndex <= eRow ) {
		moveLCDCursor( rIndex, cIndex ) ;
		while( cIndex <= eCol ) {
			writeDataToLCD( ' ' ) ;
			cIndex++ ;
		}
		rIndex++ ;
	}
}

#endif