#define UBRRH // if you need Serial
#include <Arduino.h>

#include "printf.h"

#include <SPI.h>
#include "nRF24L01.h"
#include "RF24.h"

#include "rf_message.h"

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
#define HEAT_PIN 14
#define MESSAGE_FREQ_SEC 3
#define HEATING_CALC_SEC 2

#define BYTETOBINARYPATTERN "%d%d%d%d%d%d%d%d"
#define BYTETOBINARY(byte)  \
  (byte & 0x80 ? 1 : 0), \
  (byte & 0x40 ? 1 : 0), \
  (byte & 0x20 ? 1 : 0), \
  (byte & 0x10 ? 1 : 0), \
  (byte & 0x08 ? 1 : 0), \
  (byte & 0x04 ? 1 : 0), \
  (byte & 0x02 ? 1 : 0), \
  (byte & 0x01 ? 1 : 0)

rf_message_sens1 msg;
rf_message_res2 res;

int heatSeconds = 0;	// ennyit ment a fûtés
time_t lastMessage = 0;	// ekkor volt küldve az utolsó üzenet
time_t lastCalculate = 0;
boolean receive = false;
uint8_t timing[12];
boolean heatOn = false;
time_t heatStart = 0;
char timeBuffer[22];

int checksum(rf_message_res2 *res)
{
	int sum = 0;
	for (unsigned int i = 0; i < sizeof(rf_message_res2); ++i)
	{
		sum += ((uint8_t *)res)[i];
	}
	return sum;
}

uint8_t calcQuarter(time_t currentTime)
{
	int h = hour(currentTime);
	int m = minute(currentTime);
	return h * 4 + (m / 15);
}

uint8_t calculate(time_t currentTime)
{
	uint8_t currentQuarter = calcQuarter(currentTime);

	int idx = currentQuarter / 8;
	int bit = 7 - (currentQuarter % 8);

	uint8_t ret = (timing[idx] >> bit) & 1;

	return ret;
}

char *timeToText(time_t time)
{
	char * buffer = timeBuffer;
	// YEAR
	int val = year(time);
	buffer[0] = '0' + (val / 1000);
	val -= (val / 1000) * 1000;
	buffer[1] = '0' + (val / 100);
	val -= (val / 100) * 100;
	buffer[2] = '0' + (val / 10);
	val -= (val / 10) * 10;
	buffer[3] = '0' + val;
	buffer[4] = '.';

	// MONTH
	val = month(time);
	buffer[5] = '0' + (val / 10);
	val -= (val / 10) * 10;
	buffer[6] = '0' + val;
	buffer[7] = '.';

	// DAY
	val = day(time);
	buffer[8] = '0' + (val / 10);
	val -= (val / 10) * 10;
	buffer[9] = '0' + val;
	buffer[10] = '.';
	buffer[11] = ' ';

	// HOUR
	val = hour(time);
	buffer[12] = '0' + (val / 10);
	val -= (val / 10) * 10;
	buffer[13] = '0' + val;
	buffer[14] = ':';

	// MIN
	val = minute(time);
	buffer[15] = '0' + (val / 10);
	val -= (val / 10) * 10;
	buffer[16] = '0' + val;
	buffer[17] = ':';

	// SEC
	val = second(time);
	buffer[18] = '0' + (val / 10);
	val -= (val / 10) * 10;
	buffer[19] = '0' + val;

	buffer[20] = 0;

	return buffer;
}

#define LOG(x) printf("\r\n%s: %s", timeToText(now()), x)

void setup()
{
	// clear the timing
	for (int i = 0; i < 12; ++i)
	{
		timing[i] = 0;
	}

	Serial.begin(9600);
	printf_begin();
	LOG("nRF24 Temperature Heater");
	LOG("(c) koverg70 2014 v0.1");

	//
	// Print preamble
	//
	pinMode(LED_PIN, OUTPUT);
	pinMode(HEAT_PIN, OUTPUT);
	digitalWrite(HEAT_PIN, LOW);

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

	radio.openWritingPipe(pipes[1]);
	radio.openReadingPipe(1, pipes[0]);

	//
	// Start listening
	//
	if (radio.read_register(RX_ADDR_P2) == 0xc3)
	{
		LOG("Radio OK");
	}
	else
	{
		LOG("Radio error");
	}

	delay(2000);

	radio.stopListening();
}

void loop(void)
{
	time_t currentTime = now();

	if (currentTime - lastMessage >= MESSAGE_FREQ_SEC)
	{
		lastMessage = currentTime;

		radio.stopListening();
//		LOG("Sending ping...");
		msg.device_id = 50;
		msg.device_time = currentTime;
		msg.u1.heat1.heatSeconds = heatSeconds;
		msg.u1.heat1.heating = heatOn ? 1 : 0;
		radio.write(&msg, sizeof(rf_message_sens1));
//		LOG("Ping sent...");

		receive = true;
		radio.startListening();
	}

	if (radio.available())
	{
		bool done = false;
		while (!done)
		{
			// Fetch the payload, and see if this was the last one.
			done = radio.read(&res, sizeof(rf_message_res2));

			// Spew it
			// Delay just a little bit to let the other unit
			// make the transition to receiver
			delay(40);
		}
		receive = false;

		if (done)
		{
			if (res.device_id == 50)
			{
				printf("\r\nChecksum: %d", res.checksum);
				LOG("Heat message received...");
				int sum = res.checksum;
				res.checksum = 0;
				int sum2 = checksum(&res);
				if (sum != sum2)
				{
					LOG("Checksum error...");
					printf("\r\nSums: %d <--> %d", sum, sum2);
				}
				else
				{
					LOG("Checksum OK...");

					printf("\r\nServer time: %s", timeToText(res.device_time));

					memcpy(&timing, &(res.timing), 12);

					/*
					printf ("\r\nTiming: ");
					for (int i = 0; i < 12; ++i)
					{
						printf (BYTETOBINARYPATTERN, BYTETOBINARY(res.timing[i]));
					}
					*/

					long diff = (long)currentTime - (long)res.device_time;
					if (abs(diff) > 15)
					{
						printf("Time correction: %ld --> %ld diff: %ld", currentTime, res.device_time, abs(diff));
						setTime(res.device_time);
					}
				}
			}
			else
			{
				LOG("Other message received...");
			}
		}
		else
		{
			LOG("Timed out...");
		}
	}

	if (year(currentTime) > 2013 && currentTime - lastCalculate >= HEATING_CALC_SEC)
	{
		if (day(currentTime) != day(lastCalculate))
		{
			heatSeconds = 0;	// moved to next day, so we clear the heat seconds
		}

		if (calculate(currentTime))
		{
			if (!heatOn)
			{
				LOG("Heat ON.");
				heatOn = true;
				heatStart = currentTime;
			}
		}
		else
		{
			if (heatOn)
			{
				LOG("Heat OFF.");
				heatOn = false;
				heatSeconds += (currentTime - heatStart);
				heatStart = 0;
				printf("\r\nHeat seconds: %d", heatSeconds);
			}
		}

		digitalWrite(HEAT_PIN, heatOn ? HIGH : LOW);

		lastCalculate = currentTime;
	}

	if (!receive)
	{
		delay(500);
	}
}
