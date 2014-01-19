/*
 * rf_message.h
 *
 *  Created on: 2013.11.28.
 *      Author: koverg
 *
 *
 */
#ifndef RF_MESSAGE_H_
#define RF_MESSAGE_H_

#include <Time/Time.h>

typedef struct rf_message_sens1	// sensor type 1 message
{
	uint8_t device_id;		// the sender id
	time_t device_time;		// the time on device
	union
	{
		struct
		{
			int8_t tempDHT11;		// the temperature
			int8_t humidityDHT11;	// the humidity
			int16_t tempDS18B20;	// the temperature multiplied by 100
		} sens1;
		struct
		{
			int16_t heatSeconds;	// number of seconds we heated today
			int8_t heating;			// 0 - no heating, 1 - heating
		} heat1;
	} u1;
} rf_message_sens1;

typedef struct rf_message_res1	// sensor type 1 response
{
	uint8_t device_id;		// the sender id
	time_t device_time;		// the time on device
} rf_message_res1;

typedef struct rf_message_res2	// sensor type 1 response
{
	uint8_t device_id;		// the sender id
	time_t device_time;		// the time on device
	uint8_t timing[12];		// the timing for heat
	uint16_t checksum;
} rf_message_res2;

void print_sens1(rf_message_sens1 msg)
{
	printf("device id:   %d\r\n", msg.device_id);
	printf("device time: %ld\r\n", msg.device_time);
	printf("DHT11 temp:  %d\r\n", msg.u1.sens1.tempDHT11);
	printf("DHT11 humi:  %d\r\n", msg.u1.sens1.humidityDHT11);
	printf("DS18B20 temp:%d\r\n", msg.u1.sens1.tempDS18B20);
}

#endif /* RF_MESSAGE_H_ */
