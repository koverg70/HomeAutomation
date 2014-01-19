//#define USE_LCD
//#define USE_SERIAL

// The Arduino core
#include <Arduino.h>

/*
 Copyright (C) 2011 J. Coliz <maniacbug@ymail.com>

 This program is free software; you can redistribute it and/or
 modify it under the terms of the GNU General Public License
 version 2 as published by the Free Software Foundation.
 */

/**
 * Example for Getting Started with nRF24L01+ radios.
 *
 * This is an example of how to use the RF24 class.  Write this sketch to two
 * different nodes.  Put one of the nodes into 'transmit' mode by connecting
 * with the serial monitor and sending a 'T'.  The ping node sends the current
 * time to the pong node, which responds by sending the value back.  The ping
 * node can then see how long the whole cycle took.
 */

#include "printf.h"

#include <SPI.h>
#include "nRF24L01.h"
#include "RF24.h"
#include "LedMatrix.h"

//
// Hardware configuration
//

// Set up nRF24L01 radio on SPI bus plus pins 9 & 10

RF24 radio(9, 10);

//
// Topology
//

// Radio pipe addresses for the 2 nodes to communicate.
const uint64_t pipes[2] = { 0xF0F0F0F0E1LL, 0xF0F0F0F0D2LL };

#define LED_PIN 19

void setup() {
	initLedMatrix();

	//
	// Print preamble
	//
	pinMode(LED_PIN, OUTPUT);

	//
	// Setup and configure rf radio
	//

	radio.begin();

	// optionally, increase the delay between retries & # of retries
	radio.setRetries(15, 15);

	radio.setAutoAck(false);
	radio.setDataRate(RF24_250KBPS);

	// optionally, reduce the payload size.  seems to
	// improve reliability
	radio.setPayloadSize(16);

	//
	// Open pipes to other nodes for communication
	//

	// This simple sketch opens two pipes for these two nodes to communicate
	// back and forth.
	// Open 'our' pipe for writing
	// Open the 'other' pipe for reading, in position #1 (we can have up to 5 pipes open for reading)

	radio.openWritingPipe(pipes[1]);
	radio.openReadingPipe(1, pipes[0]);

	//
	// Start listening
	//

	radio.startListening();

	//
	// Dump the configuration of the rf unit for debugging
	//

//	radio.printDetails();

	if (radio.read_register(RX_ADDR_P2) == 0xc3) {
		for (int i = 0; i < 10; ++i) {
			digitalWrite(LED_PIN, HIGH);
			delay(100);
			digitalWrite(LED_PIN, LOW);
			delay(100);
		}
	}
}

void loop(void) {
	// if there is data ready
	if (radio.available()) {
		digitalWrite(LED_PIN, HIGH);
		// Dump the payloads until we've gotten everything
		bool done = false;
		while (!done) {
			// Fetch the payload, and see if this was the last one.
			done = radio.read(&text, 16);

			// Spew it
			// Delay just a little bit to let the other unit
			// make the transition to receiver
			delay(40);
		}
		digitalWrite(LED_PIN, LOW);

		// First, stop listening so we can talk
		radio.stopListening();

		// Send the final one back.
		radio.write(&cur_pos, sizeof(cur_pos));

		delay(600);

		// Now, resume listening so we catch the next packets.
		radio.startListening();
	}
}

