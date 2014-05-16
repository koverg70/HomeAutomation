/**
 * Home automation Ethernet Server v1.0.1
 * (c) Gabor KÖVÉR 2013-2014
 *
 * TODO:
 *   - mutassa, hogy éppen melyik idõzítés az aktív
 *   - mutassuk meg a fûtés estén is az utolsó észlelési idõt és a leghosszabb várakozást
 */

#define UBRRH // if you need Serial
#include <Arduino.h>
#include "printf.h"
#include "SPI/SPI.h"
#include "RF24/nRF24L01.h"
#include "RF24/RF24.h"
#include "rf_message.h"
#include "EtherShield/etherShield.h"
#include "Time/Time.h"
#include "EEPROM/EEPROM.h"

#define LED_PIN 19

extern bool IsDST(int year, int month, int day);
extern char dow(int y, char m, char d);

//
// Hardware configuration
//

// Set up nRF24L01 radio on SPI bus plus pins 9 & 10
RF24 radio(8, 9);
//
// Topology
//
// Radio pipe addresses for the 2 nodes to communicate.
const uint64_t pipes[2] = { 0xF0F0F0F0E1LL, 0xF0F0F0F0D2LL };


//-----------------------------------------------------------
// The ethernet shield
EtherShield es = EtherShield();
static uint8_t mymac[6] = { 0x54, 0x55, 0x58, 0x10, 0x00, 0x24 };
static uint8_t myip[4] = { 192, 168, 1, 44 };
static uint8_t gwip[4] = { 192, 168, 1, 1 };
static uint8_t ntpip[4] = { 193, 224, 45, 107 };
//-----------------------------------------------------------
// WWW
#define MYWWWPORT 80
#define BUFFER_SIZE 800
static uint8_t buf[BUFFER_SIZE + 1];
//-----------------------------------------------------------
#define GMT_ZONE 1
#define SETTINGS_SIZE 96
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

#define MAX_SENSORS 9
//-- server state
static time_t startTime = 0;
static time_t lastSensorData[MAX_SENSORS];
static uint32_t longestSensorWait[MAX_SENSORS];
int8_t tempDHT11[MAX_SENSORS];		// the temperature
int8_t humidityDHT11[MAX_SENSORS];	// the humidity
int16_t tempDS18B20[MAX_SENSORS];	// the temperature multiplied by 100
time_t lastHttp = 0;

void initEthernet()
{
	es.ES_enc28j60Init(mymac);
	Serial.println("mac address set");
	// init the ethernet/ip layer:
	es.ES_init_ip_arp_udp_tcp(mymac, myip, MYWWWPORT);
	es.ES_client_set_gwip(gwip);
	Serial.println("ip layer set");
	setTime(0);
	lastHttp = now();
}

void setup() {

	for (int i = 0; i < MAX_SENSORS; ++i)
	{
		lastSensorData[i] = 0;
		longestSensorWait[i] = 0;
	}

	Serial.begin(9600);
	printf_begin();
	printf("\n\rnRF24 Temperature Server with Ethernet\n\r");
	printf("\n\r(c) koverg70 2013\n\r");

	initEthernet();

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

	//radio.openWritingPipe(pipes[0]);
	//radio.openReadingPipe(1,pipes[1]);
	radio.openWritingPipe(pipes[1]);
	radio.openReadingPipe(1, pipes[0]);

	// test SPI to nRF24L01+
	if (radio.read_register(RX_ADDR_P2) == 0xc3) {
		for (int i = 0; i < 10; ++i) {
			digitalWrite(LED_PIN, HIGH);
			delay(100);
			digitalWrite(LED_PIN, LOW);
			delay(100);
		}
	}

	delay(100);

	//
	// Start listening
	//
	radio.startListening();

	es.ES_client_ntp_request(buf, ntpip, 25000);
}

int checksum(rf_message_res2 *res)
{
	int sum = 0;
	for (unsigned int i = 0; i < sizeof(rf_message_res2); ++i)
	{
		sum += ((uint8_t *)res)[i];
	}
	return sum;
}

void timeDateToText(time_t time, char *buffer)
{
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
}

uint16_t http200ok(void)
{
	return (es.ES_fill_tcp_data_p(buf, 0, PSTR(
			"HTTP/1.0 200 OK\r\n"
			"Content-Type: text/html\r\n"
			"Pragma: no-cache\r\n"
			"Access-Control-Allow-Origin: *\r\n"	// needed to allow request from other sites
			"\r\n")));
}

// prepare the webpage by writing the data to the tcp send buffer
uint16_t print_webpage(uint8_t *buf)
{
	time_t nnn = now();
	uint16_t plen;
	char timeBuff[96];
	plen = http200ok();
	plen = es.ES_fill_tcp_data_p(buf,plen, PSTR("{time: \""));
	timeDateToText(now(), timeBuff);
	plen = es.ES_fill_tcp_data(buf,plen, timeBuff);
	plen = es.ES_fill_tcp_data_p(buf,plen, PSTR("\", start: \""));
	ltoa(nnn - startTime, timeBuff, 10);
	plen = es.ES_fill_tcp_data(buf,plen, timeBuff);
	plen = es.ES_fill_tcp_data_p(buf,plen, PSTR("\""));

	for (int i = 0; i < MAX_SENSORS; ++i)
	{
		if (lastSensorData[i] != 0)
		{
			itoa(i, timeBuff, 10);
			plen = es.ES_fill_tcp_data_p(buf,plen, PSTR(", sensors"));
			plen = es.ES_fill_tcp_data(buf,plen, timeBuff);
			plen = es.ES_fill_tcp_data_p(buf,plen, PSTR(": {"));

			plen = es.ES_fill_tcp_data_p(buf,plen, PSTR("lastReceived: \""));
			itoa(nnn - lastSensorData[i], timeBuff, 10);
			plen = es.ES_fill_tcp_data(buf,plen, timeBuff);

			plen = es.ES_fill_tcp_data_p(buf,plen, PSTR("\", longestWait: \""));
			itoa(longestSensorWait[i], timeBuff, 10);
			plen = es.ES_fill_tcp_data(buf,plen, timeBuff);

			plen = es.ES_fill_tcp_data_p(buf,plen, PSTR("\", tempDHT11: \""));
			itoa(tempDHT11[i], timeBuff, 10);
			plen = es.ES_fill_tcp_data(buf,plen, timeBuff);

			plen = es.ES_fill_tcp_data_p(buf,plen, PSTR("\", humidityDHT11: \""));
			itoa(humidityDHT11[i], timeBuff, 10);
			plen = es.ES_fill_tcp_data(buf,plen, timeBuff);

			plen = es.ES_fill_tcp_data_p(buf,plen, PSTR("\", tempDS18B20: \""));
			ltoa(tempDS18B20[i], timeBuff, 10);
			plen = es.ES_fill_tcp_data(buf,plen, timeBuff);

			plen = es.ES_fill_tcp_data_p(buf, plen, PSTR("\"}"));
		}
	}

	plen = es.ES_fill_tcp_data_p(buf, plen, PSTR(", settings: \""));
	for (int i = 0; i < SETTINGS_SIZE; ++i)
	{
		byte b = EEPROM.read(i);
		//byte b = settings[i];
		if (b != '0' && b != '1')
		{
			b = '0';
		}
		timeBuff[i] = b;
	}
	timeBuff[SETTINGS_SIZE] = 0;
	plen = es.ES_fill_tcp_data(buf,plen, timeBuff);

	plen = es.ES_fill_tcp_data_p(buf, plen, PSTR("\"}\0"));
	return plen;
}

rf_message_sens1 msg;
rf_message_res1 res;
rf_message_res2 res2;

void loop(void)
{
	// nRF24L01+ if there is data ready
	if (radio.available())
	{
		digitalWrite(LED_PIN, HIGH);
		// Dump the payloads until we've gotten everything
		bool done = false;
		while (!done) {
			// Fetch the payload, and see if this was the last one.
			done = radio.read(&msg, sizeof(rf_message_sens1));

			// Spew it
			// Delay just a little bit to let the other unit
			// make the transition to receiver
			delay(40);
		}
		digitalWrite(LED_PIN, LOW);

		if (done)
		{
			print_sens1(msg);
			if (msg.device_id == 50)
			{
				printf("\r\nHeater message...");

				//printf("\r\nSending response...");

				res2.device_id = 50;
				res2.device_time = now();
				for (int i = 0; i < 12; ++i)
				{
					res2.timing[i] = 0;
				}
				for (int i = 0; i < SETTINGS_SIZE; ++i)
				{
					byte b = EEPROM.read(i);
					if (b == '1')
					{
						res2.timing[i/8] |= (1 << (7-(i%8)));
					}
				}
				res2.checksum = 0;

//				printf ("\r\nTiming: ");
//				for (int i = 0; i < 12; ++i)
//				{
//					printf (BYTETOBINARYPATTERN, BYTETOBINARY(res2.timing[i]));
//				}

				res2.checksum = checksum(&res2);
//				printf("\r\nChecksum: %d", res2.checksum);

				// First, stop listening so we can talk
				radio.stopListening();
				// Send the final one back.
				radio.write(&res2, sizeof(rf_message_res2));

				//printf("\r\nResponse sent...");
				delay(10);	// TODO: 1000
			}
			else
			{
				int index = 0;
				if (msg.device_id > 0 && msg.device_id <= 8)
				{
					index = msg.device_id;
				}
				// save data
				tempDHT11[index] = msg.u1.sens1.tempDHT11;
				humidityDHT11[index] = msg.u1.sens1.humidityDHT11;
				tempDS18B20[index] = msg.u1.sens1.tempDS18B20;
				time_t nnn = now();
				lastSensorData[index] = nnn;

				printf("\r\nSending response...");
				// First, stop listening so we can talk
				radio.stopListening();
				// Send the final one back.
				res.device_id = 50;	// TODO: most 50-es a heater és a központ is ???
				res.device_time = nnn;
				radio.write(&res, sizeof(rf_message_res1));

				printf("\r\nResponse sent...");
				delay(10);	// TODO: 1000
			}

		} else {
			printf("\r\nTimed out...");
		}

		// Now, resume listening so we catch the next packets.
		radio.startListening();
	}

	// ethernet
	uint16_t plen, dat_p;

	// read packet, handle ping and wait for a tcp packet:
	dat_p = es.ES_packetloop_icmp_tcp(buf,
			es.ES_enc28j60PacketReceive(BUFFER_SIZE, buf));

	/* dat_p will be unequal to zero if there is a valid
	 * http get */
	if (dat_p != 0) {
		// there is a request

		// process NTP response
		if (buf[IP_PROTO_P] == IP_PROTO_UDP_V &&
			buf[UDP_SRC_PORT_H_P] == 0 &&
			buf[UDP_SRC_PORT_L_P] == 0x7b )
		{
			//Serial.println("Processing NTP response...");
			uint32_t time = 0;
			if (es.ES_client_ntp_process_answer(buf, &time, 25000))
			{
				startTime = time - 2208988800UL + (GMT_ZONE * 60 * 60); // 70 years back plus the GTM shift
				if (IsDST(year(startTime), month(startTime), day(startTime)))
				{
					startTime += 60 * 60;	// one hour because it's a daylight saving day
				}
				setTime(startTime);
				//Serial.println("Time adjusted with NTP time.");
			}
		}
		else if (strncmp("GET ", (char *) &(buf[dat_p]), 4) == 0)
		{
			// process HTTP request
			// just one web page in the "root directory" of the web server
			if (strncmp("/ ", (char *) &(buf[dat_p + 4]), 2) == 0)
			{
				// aktuális státusz visszaküldése
				dat_p = print_webpage(buf);
			}
			else if (strncmp("/S-", (char *) &(buf[dat_p + 4]), 3) == 0)
			{
				char *p = (char *) &(buf[dat_p + 7]);
				printf("\r\nIdõzítések beállítása: %s",  p);
				// idõzítések beállítása
				for (int i = 0; i < SETTINGS_SIZE; ++i)
				{
					byte b = *(p++);
					if (b != '0' && b != '1')
					{
						b = '0';
					}
					EEPROM.write(i, b);
					//settings[i] = b;
				}
				dat_p = http200ok();
				dat_p = es.ES_fill_tcp_data_p(buf, dat_p, PSTR("<h1>200 OK</h1>"));
			}
			else
			{
				dat_p = es.ES_fill_tcp_data_p(buf, 0,PSTR("HTTP/1.0 401 Unauthorized\r\nContent-Type: text/html\r\n\r\n<h1>401 Unauthorized</h1>"));
			}
		}
		else
		{
			// head, post and other methods:
			dat_p = http200ok();
			dat_p = es.ES_fill_tcp_data_p(buf, dat_p, PSTR("<h1>200 OK</h1>"));
		}
		es.ES_www_server_reply(buf, dat_p); // send web page data

		lastHttp = now();
	}

	time_t nnn = now();

	if (nnn - lastHttp > 120)
	{
		initEthernet();
	}

	for (int index = 0; index < MAX_SENSORS; ++index)
	{
		if (lastSensorData[index] != 0)
		{
			unsigned int w = nnn - lastSensorData[index];
			if (w > longestSensorWait[index])
			{
				longestSensorWait[index] = w;
			}
		}
	}
}

