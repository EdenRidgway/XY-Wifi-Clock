/*
 * 
 * Copyright (c) 2015. Mario Mikočević <mozgy>
 * 
 * MIT Licence
 *
 */
 
#include <Wire.h>
#include "TM1650.h"

TM1650::TM1650( void ) {
	localI2CDisplayAddress = TM1650_DISPLAY_BASE;
	localI2CControlAddress = TM1650_CONTROL_BASE;
	localNumDigits = TM1650_NUM_DIGITS;
}

void TM1650::SendControl( unsigned char data, uint8_t segment ) {
	segment = segment % TM1650_NUM_DIGITS;
	Wire.beginTransmission( localI2CControlAddress + segment );
	Wire.write( data );
	Wire.endTransmission();
	TM1650_CONTROL_STORE[segment] = data;
}

void TM1650::SendControl( unsigned char data ) {
	SendControl( data, 0 );
}

void TM1650::SendDigit( unsigned char data, uint8_t segment ) {
	segment = segment % TM1650_NUM_DIGITS;
	data = data & 0x7F;
	data = data | ( TM1650_DIGITS_STORE[segment] & 0x80 );
	Wire.beginTransmission( localI2CDisplayAddress + segment );
	Wire.write( data );
	Wire.endTransmission();
	TM1650_DIGITS_STORE[segment] = data;
}

void TM1650::SendDigit( unsigned char data ) {
	SendDigit( data, 0 );
}

void TM1650::SetDot( uint8_t segment, bool onoff ) {
	segment = segment % TM1650_NUM_DIGITS;
	unsigned char digit = TM1650_DIGITS_STORE[segment];
	digit = ( onoff ) ? digit | 0x80 : digit & 0x7f;
	Wire.beginTransmission( localI2CDisplayAddress + segment );
	Wire.write( digit );
	Wire.endTransmission();
	TM1650_DIGITS_STORE[segment] = digit;
}

void TM1650::SetBrightness( unsigned char brightness, uint8_t segment ) {
	if( brightness > 0x07 ) {
		brightness = 0x07;
	}
	segment = segment % TM1650_NUM_DIGITS;
	SendControl( ( ( TM1650_CONTROL_STORE[segment] & TM1650_MASK_BRIGHTNESS ) | ( brightness << 4 ) ), segment );
}

void TM1650::SetBrightness( unsigned char brightness ) {
	SetBrightness( brightness, 0 );
}

void TM1650::ClearDisplay( void ) {
	for( uint8_t i=0; i<localNumDigits; i++) {
		SendDigit( 0x00, i );
		TM1650_DIGITS_STORE[i] = 0x00;
	}
}

void TM1650::DisplayON( void ) {
	for( uint8_t i=0; i<localNumDigits; i++) {
		SendControl( ( ( TM1650_CONTROL_STORE[i] & TM1650_MASK_ONOFF ) | TM1650_BIT_ONOFF ), i );
	}
}

void TM1650::DisplayOFF( void ) {
	for( uint8_t i=0; i<localNumDigits; i++) {
		SendControl( ( TM1650_CONTROL_STORE[i] & TM1650_MASK_ONOFF ), i );
	}
}

void TM1650::ColonON( void ) {
	SendControl( ( TM1650_CONTROL_STORE[0] & TM1650_MASK_COLON ) | TM1650_BIT_COLON );
}

void TM1650::ColonOFF( void ) {
	SendControl( TM1650_CONTROL_STORE[0] & TM1650_MASK_COLON );
}

void TM1650::WriteNum( uint16_t num, uint8_t position ) {
	position = position % TM1650_NUM_DIGITS;
	num = num & 0x0F;
	SendDigit( TM1650_Digit_Table[num], position );
}

void TM1650::WriteNum( uint16_t num ) {
	uint8_t digit;
	digit = num % 10;
	SendDigit( TM1650_Digit_Table[digit], 3 );
	num = num / 10;
	digit = num % 10;
	SendDigit( TM1650_Digit_Table[digit], 2 );
	num = num / 10;
	digit = num % 10;
	SendDigit( TM1650_Digit_Table[digit], 1 );
	num = num / 10;
	digit = num % 10;
	SendDigit( TM1650_Digit_Table[digit], 0 );
}

void TM1650::Init( void ) {
	for( uint8_t i=0; i<localNumDigits; i++) {
		TM1650_DIGITS_STORE[i] = 0x00;
		TM1650_CONTROL_STORE[i] = 0x00;
	}
	ClearDisplay();
	DisplayON();
}
