/*
 * LedMatrix.cpp
 *
 *  Created on: 2013.07.20.
 *      Author: koverg
 */

#include <Arduino.h>
#include <pins_arduino.h>
#include "8x6_chars.h"

// Pin connected to ST_CP of 74HC595
int latchPin = 1;	// D1
// Pin connected to SH_CP of 74HC595
int clockPin = 2;	// D2
// Pin connected to DS of 74HC595
int dataPin = 0;	// D0

#define TEXT_LEN 	15	// this is the length of the displayed text buffer
#define NUM_MATRIX	2	// this is the number of 8x8 LED matrices (NUM_MATRIX * 2 is the number of shift registers)

char text[TEXT_LEN+1] = "| 74HC959 test_";
long cur_pos = -(NUM_MATRIX*8);	// this is the pixel coordinate of the first displayed column
uint8_t shiftOutBuffer[NUM_MATRIX];

void initLedMatrix()
{
    DDRD |= _BV(1);  // pinMode(latchPin, OUTPUT);
    DDRD |= _BV(2);  //pinMode(clockPin, OUTPUT);
    DDRD |= _BV(0);  //pinMode(dataPin, OUTPUT);

    noInterrupts();           // disable all interrupts
    TCCR1A = 0;
    TCCR1B = 0;
    TCNT1  = 0;

    //OCR1A = 190;            // compare match register 16MHz/256/160Hz
    OCR1A = 31;               // compare match register 16MHz/256/2000Hz
    TCCR1B |= (1 << WGM12);   // CTC mode
    TCCR1B |= (1 << CS12);    // 256 prescaler
    TIMSK1 |= (1 << OCIE1A);  // enable timer compare interrupt
    interrupts();             // enable all interrupts
}

void myShiftOut(uint8_t val)
{
	if (val & 0b10000000) PORTD |= 0x01; else PORTD &= 0xfe;
	PORTD |= _BV(2);
	PORTD &= ~_BV(2);

	if (val & 0b01000000) PORTD |= 0x01; else PORTD &= 0xfe;
	PORTD |= _BV(2);
	PORTD &= ~_BV(2);

	if (val & 0b00100000) PORTD |= 0x01; else PORTD &= 0xfe;
	PORTD |= _BV(2);
	PORTD &= ~_BV(2);

	if (val & 0b00010000) PORTD |= 0x01; else PORTD &= 0xfe;
	PORTD |= _BV(2);
	PORTD &= ~_BV(2);

	if (val & 0b00001000) PORTD |= 0x01; else PORTD &= 0xfe;
	PORTD |= _BV(2);
	PORTD &= ~_BV(2);

	if (val & 0b00000100) PORTD |= 0x01; else PORTD &= 0xfe;
	PORTD |= _BV(2);
	PORTD &= ~_BV(2);

	if (val & 0b00000010) PORTD |= 0x01; else PORTD &= 0xfe;
	PORTD |= _BV(2);
	PORTD &= ~_BV(2);

	if (val & 0b00000001) PORTD |= 0x01; else PORTD &= 0xfe;
	PORTD |= _BV(2);
	PORTD &= ~_BV(2);
}

void shiftOutToAllMatrices(uint8_t bits[], int col)
{

	uint8_t cbits = (1 << col);

	PORTD &= ~_BV(1); 	// digitalWrite(latchPin, LOW);

	for (int i = 0; i < NUM_MATRIX; ++i)
	{
		myShiftOut(cbits);	// columns
		myShiftOut(bits[i]);   // LED values
	}

	PORTD |= _BV(1);	// digitalWrite(latchPin, HIGH);
}

#define MULTIPLEX 30

int jj = MULTIPLEX-1;
int kk = 7;

ISR(TIMER1_COMPA_vect)          // timer compare interrupt service routine
{
	if (++kk > 7)
	{
		kk = 0;
		if (++jj > MULTIPLEX-1)
		{
			jj = 0;
			if (++cur_pos > TEXT_LEN * 6)
			{
				cur_pos = -(NUM_MATRIX * 8);
			}
		}
	}
	for (int i = 0; i < NUM_MATRIX; ++i)
	{
		uint8_t pos = cur_pos + kk + (i * 8);
		if (0 <= pos && pos < TEXT_LEN * 6)
		{
			shiftOutBuffer[i] = matrixColBits(pos, text);
		}
		else
		{
			shiftOutBuffer[i] = 0;
		}
	}
	shiftOutToAllMatrices(shiftOutBuffer, kk);
}
