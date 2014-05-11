#define UBRRH // if you need Serial
#include <Arduino.h>

#include "printf.h"

#include <SPI.h>
#include "nRF24L01.h"
#include "RF24.h"

#include <OneWire/OneWire.h>
#include <DHT/dht.h>

#include "rf_message.h"
#include "avr_sleep.h"

extern void initDS();
extern void readDS(rf_message_sens1 &msg);

extern void readDHT11(rf_message_sens1 &msg);

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

	Serial.begin(9600);
	printf_begin();
	printf("\n\rnRF24 Temperature Sensor\n\r");
	printf("\n\r(c) koverg70 2013\n\r");

	sleep_setup();
	initDS();

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
	radio.setPayloadSize(20);

	radio.openWritingPipe(pipes[0]);
	radio.openReadingPipe(1, pipes[1]);

	//
	// Start listening
	//

	if (radio.read_register(RX_ADDR_P2) == 0xc3) {
		for (int i = 0; i < 10; ++i) {
			digitalWrite(LED_PIN, HIGH);
			delay(100);
			digitalWrite(LED_PIN, LOW);
			delay(100);
		}
	}
}

long cur_pos = 0;
long timeout = 2000; // wait for 1 sec

rf_message_sens1 msg;
rf_message_res1 res;
char buff[20];

void loop(void) {
	readDHT11(msg);
	readDS(msg);
	msg.device_id = 2;
	msg.device_time = 0;
	print_sens1(msg);

	// if there is data ready
	radio.stopListening();

	uint32_t sent_at = millis();

	// Send the final one back.
	radio.write(&msg, sizeof(rf_message_sens1));

	radio.startListening();

	boolean radio_avail = false;

	while (!(radio_avail = radio.available()) && millis() - sent_at < timeout)
		;

	if (millis() - sent_at >= timeout) {
		printf("Time out...\r\n");
	}
	if (radio_avail) {
		digitalWrite(LED_PIN, HIGH);
		// Dump the payloads until we've gotten everything
		bool done = false;
		while (!done) {
			// Fetch the payload, and see if this was the last one.
			// res.device_id = 1;
			// res.device_time = now();
			done = radio.read(&res, sizeof(rf_message_res1));

			// TODO: process response, set local device time
			// Spew it
			// Delay just a little bit to let the other unit
			// make the transition to receiver
		}
		if (done) {
			// printf("Current position is: %ld\r\n", cur_pos);
			printf("Respons got within %d ms.\r\n", millis() - sent_at);
		}
	} else {
		printf("No response. Peer is not alive.\r\n");
	}

	radio.powerDown();

	delay(100);
	digitalWrite(LED_PIN, LOW);

	enterSleep();
	enterSleep();
	radio.powerUp();
	delay(10);
}

/*
 A DHT11 olvasása és kiírása
 */

#define dht_dpin 8 //no ; here. Set equal to channel sensor is on
dht DHT;

void readDHT11(rf_message_sens1 &msg) {
	DHT.read11(dht_dpin);

	msg.u1.sens1.tempDHT11 = (int8_t) DHT.temperature;
	msg.u1.sens1.humidityDHT11 = (int8_t) DHT.humidity;
}

/*
 A DS18B20 olvasása és kiírása
 */

#define ONEWIREPIN  2	 	/* OneWire bus on digital pin 7 */
#define MAXSENSORS  5		/* Maximum number of sensors on OneWire bus */

// Model IDs
#define DS18S20      0x10
#define DS18B20      0x28
#define DS1822       0x22

// OneWire commands
#define CONVERT_T       0x44  // Tells device to take a temperature reading and put it on the scratchpad
#define COPYSCRATCH     0x48  // Copy EEPROM
#define READSCRATCH     0xBE  // Read EEPROM
#define WRITESCRATCH    0x4E  // Write to EEPROM
#define RECALLSCRATCH   0xB8  // Reload from last known
#define READPOWERSUPPLY 0xB4  // Determine if device needs parasite power
#define ALARMSEARCH     0xEC  // Query bus for devices with an alarm condition
class Sensor /* hold info for a DS18 class digital temperature sensor */
{
public:
	byte addr[8];
	boolean parasite;
	float temp;

};

int HighByte, LowByte, TReading, SignBit, Tc_100;
byte i, j, sensors;
byte present = 0;
boolean ready;
int dt;
byte data[12];
byte addr[8];
OneWire ds(ONEWIREPIN); // DS18S20 Temperature chip i/o
Sensor DS[MAXSENSORS]; /* array of digital sensors */

void initDS(void) {
	// initialize inputs/outputs
	// start serial port
	sensors = 0;
	LOG_OUTPUT_LINE("Searching for sensors...");
	while (ds.search(addr)) {
		if (OneWire::crc8(addr, 7) != addr[7]) {
			LOG_OUTPUT("CRC is not valid!\n");
			break;
		}
		delay(1000);
		ds.write(READPOWERSUPPLY);
		boolean parasite = !ds.read_bit();
		present = ds.reset();
		LOG_OUTPUT("temp");
		LOG_OUTPUTN(sensors, DEC);
		LOG_OUTPUT(": ");
		DS[sensors].parasite = parasite;
		for (i = 0; i < 8; i++) {
			DS[sensors].addr[i] = addr[i];
			LOG_OUTPUTN(addr[i], HEX);
			LOG_OUTPUT(" ");
		}
		//LOG_OUTPUT(addr,HEX);
		if (addr[0] == DS18S20) {
			LOG_OUTPUT(" DS18S20");
		} else if (addr[0] == DS18B20) {
			LOG_OUTPUT(" DS18B20");
		} else {
			LOG_OUTPUT(" unknown");
		}
		if (DS[sensors].parasite) {
			LOG_OUTPUT(" parasite");
		} else {
			LOG_OUTPUT(" powered");
		}LOG_OUTPUT_LINE();
		sensors++;
	}
	LOG_OUTPUTN(sensors, DEC);
	LOG_OUTPUT(" sensors found");
	LOG_OUTPUT_LINE();
	LOG_OUTPUT("OneWire pin: ");
	LOG_OUTPUTN(ONEWIREPIN, DEC);
	LOG_OUTPUT_LINE();
	for (i = 0; i < sensors; i++) {
		LOG_OUTPUT("temp");
		LOG_OUTPUTN(i, DEC);
		if (i < sensors - 1) {
			LOG_OUTPUT(",");
		}
	}
	LOG_OUTPUT_LINE();
}

void get_ds(int sensors) { /* read sensor data */
	for (i = 0; i < sensors; i++) {
		ds.reset();
		ds.select(DS[i].addr);
		ds.write(CONVERT_T, DS[i].parasite); // start conversion, with parasite power off at the end

		if (DS[i].parasite) {
			dt = 75;
			delay(750); /* no way to test if ready, so wait max time */
		} else {
			ready = false;
			dt = 0;
			delay(10);
			while (!ready && dt < 75) { /* wait for ready signal */
				delay(10);
				ready = ds.read_bit();
				dt++;
			}
		}

		present = ds.reset();
		ds.select(DS[i].addr);
		ds.write(READSCRATCH); // Read Scratchpad

		for (j = 0; j < 9; j++) { // we need 9 bytes
			data[j] = ds.read();
		}

		/* check for valid data */
		if ((data[7] == 0x10) || (OneWire::crc8(addr, 8) != addr[8])) {
			LowByte = data[0];
			HighByte = data[1];
			TReading = (HighByte << 8) + LowByte;
			SignBit = TReading & 0x8000; // test most sig bit
			if (SignBit) // negative
			{
				TReading = (TReading ^ 0xffff) + 1; // 2's comp
			}
			if (DS[i].addr[0] == DS18B20) { /* DS18B20 0.0625 deg resolution */
				Tc_100 = (6 * TReading) + TReading / 4; // multiply by (100 * 0.0625) or 6.25
			} else if (DS[i].addr[0] == DS18S20) { /* DS18S20 0.5 deg resolution */
				Tc_100 = (TReading * 100 / 2);
			}

			if (SignBit) {
				DS[i].temp = -(float) Tc_100 / 100;
			} else {
				DS[i].temp = (float) Tc_100 / 100;
			}
		} else { /* invalid data (e.g. disconnected sensor) */
			DS[i].temp = NAN;
		}
	}
}

void readDS(rf_message_sens1 &msg) {
	get_ds(sensors);
	for (i = 0; i < sensors; i++) {
		if (isnan(DS[i].temp)) {
			msg.u1.sens1.tempDS18B20 = -50000; // NaH
		} else {
			msg.u1.sens1.tempDS18B20 = (int16_t) (DS[i].temp * 100);
		}
	}
}

