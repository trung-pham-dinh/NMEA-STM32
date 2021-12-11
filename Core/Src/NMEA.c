/*
 * NMEA.c
 *
 *  Created on: Dec 10, 2021
 *      Author: fhdtr
 */


#include "Fifo.h"
#include <string.h>

static uint8_t incomeByte;
static UART_HandleTypeDef* uart;

#define NMEA_START_CHAR '$'
#define NMEA_DELIM_CHAR ','
#define NMEA_END_CHAR '*'

#define NMEA_GGA_SIZE 15
#define NMEA_RMC_SIZE 13
#define NMEA_GSV_SIZE 18

#define NMEA_FIELD_MAX_SIZE (10+1) // 1 slot for '\0' (10 because the longest field is time: 10 chars)

typedef enum {
	NMEA_STATE_WAIT,
	NMEA_STATE_PARSE,
	NMEA_STATE_ID,
	NMEA_STATE_END
} NMEA_State;


static uint8_t GGA_frame[NMEA_GGA_SIZE][NMEA_FIELD_MAX_SIZE];
static uint8_t RMC_frame[NMEA_RMC_SIZE][NMEA_FIELD_MAX_SIZE];
static uint8_t GSA_frame[NMEA_GSV_SIZE][NMEA_FIELD_MAX_SIZE];



void NMEA_Init(UART_HandleTypeDef* huart) {
	uart = huart;
	HAL_UART_Receive_IT(uart, &incomeByte, 1);
}

void NMEA_ReadByte() {
	if(FF_isFull()) return;

	FF_write(incomeByte);
	HAL_UART_Receive_IT(uart, &incomeByte, 1);
}

void NMEA_Parser() {
	static NMEA_State state = NMEA_STATE_WAIT;
	static uint8_t cur_id[6];
	static uint8_t indexChar = 0, indexField = 0;
	static uint8_t (*dataframe)[NMEA_FIELD_MAX_SIZE];

	if(FF_isEmpty()) return;

	uint8_t byte = FF_read();

	switch(state) {
	case NMEA_STATE_WAIT:
		if(byte == NMEA_START_CHAR) {
			state = NMEA_STATE_ID;
			memset(cur_id, '\0', 6);
			indexChar = 0;
			indexField = 0;
			dataframe = NULL;
		}
		break;
	case NMEA_STATE_ID:
		if(byte == NMEA_DELIM_CHAR) {
			cur_id[indexChar] = '\0';

			if(strcmp((char*)cur_id, "GPGGA") == 0) {
				dataframe = GGA_frame;
				HAL_GPIO_TogglePin(GPIOD, GPIO_PIN_14);
			}
			else if(strcmp((char*)cur_id, "GPRMC") == 0) {
				dataframe = RMC_frame;
			}
			else if(strcmp((char*)cur_id, "GPGSA") == 0) {
				dataframe = GSA_frame;
			}
			else {
				state = NMEA_STATE_WAIT;
				break;
			}

			indexChar = 0;
			state = NMEA_STATE_PARSE;
		}
		else {
			cur_id[indexChar++] = byte;
		}
		break;
	case NMEA_STATE_PARSE:
		if(byte == NMEA_DELIM_CHAR) {
			dataframe[indexField][indexChar]  = '\0';
			indexField++;
			indexChar = 0;
		}
		else if(byte == NMEA_END_CHAR) {
			dataframe[indexField][indexChar]  = '\0';

			state = NMEA_STATE_END;
			indexField++;
			indexChar = 0;
		}
		else {
			dataframe[indexField][indexChar++] = byte;
		}
		break;
	case NMEA_STATE_END:
		dataframe[indexField][indexChar++] = byte;
		if(indexChar == 2) {
			dataframe[indexField][indexChar]  = '\0';
			state = NMEA_STATE_WAIT;
		}
		break;
	}
}

