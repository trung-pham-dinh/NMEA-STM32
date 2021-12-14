/*
 * NMEA.c
 *
 *  Created on: Dec 10, 2021
 *      Author: fhdtr
 */

#include "NMEA.h"
#include "Fifo.h"
#include <string.h>
#include <stdlib.h>

static uint8_t incomeByte;
static UART_HandleTypeDef* uart;

#define NMEA_START_CHAR '$'
#define NMEA_DELIM_CHAR ','
#define NMEA_END_CHAR '*'

#define NMEA_GGA_SIZE 15
#define NMEA_RMC_SIZE 13
#define NMEA_GSA_SIZE 18

#define NMEA_FIELD_MAX_SIZE (10+1) // 1 slot for '\0' (10 because the longest field is time: 10 chars)

typedef enum {
	NMEA_STATE_WAIT,
	NMEA_STATE_PARSE,
	NMEA_STATE_ID,
	NMEA_STATE_CHECKSUM
} NMEA_State;

typedef uint8_t field_t[NMEA_FIELD_MAX_SIZE];

static uint8_t GGA_frame[NMEA_GGA_SIZE][NMEA_FIELD_MAX_SIZE];
static uint8_t RMC_frame[NMEA_RMC_SIZE][NMEA_FIELD_MAX_SIZE];
static uint8_t GSA_frame[NMEA_GSA_SIZE][NMEA_FIELD_MAX_SIZE];

static uint8_t GGA_wasFieldReadBefore[NMEA_GGA_SIZE]; // prevent contiguously extracting, converting from dataframe
static uint8_t RMC_wasFieldReadBefore[NMEA_RMC_SIZE];
static uint8_t GSA_wasFieldReadBefore[NMEA_GSA_SIZE];

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
	static field_t *dataframe;
	static uint8_t *readBefore;
	static uint8_t size;

	static uint8_t indexChar = 0, indexField = 1; // first field is for Valid bit
	static uint8_t totalsum=0, checksum=0;

	if(FF_isEmpty()) return;

	uint8_t byte = FF_read();

	switch(state) {
	case NMEA_STATE_WAIT:
		if(byte == NMEA_START_CHAR) {
			state = NMEA_STATE_ID;

			indexChar = 0;
			indexField = 1;
			totalsum = 0;
			checksum = 0;
			dataframe = NULL;
			readBefore = NULL;
			size = 0;
		}
		break;
	case NMEA_STATE_ID:
		if(byte == NMEA_DELIM_CHAR) {
			cur_id[indexChar] = '\0';

			if(strcmp((char*)cur_id, "GPGGA") == 0) {
				dataframe = GGA_frame;
				readBefore = GGA_wasFieldReadBefore;
				size = NMEA_GGA_SIZE;

				HAL_GPIO_TogglePin(GPIOD, GPIO_PIN_14); // checking
			}
			else if(strcmp((char*)cur_id, "GPRMC") == 0) {
				dataframe = RMC_frame;
				readBefore = RMC_wasFieldReadBefore;
				size = NMEA_RMC_SIZE;
			}
			else if(strcmp((char*)cur_id, "GPGSA") == 0) {
				dataframe = GSA_frame;
				readBefore = GSA_wasFieldReadBefore;
				size = NMEA_GSA_SIZE;
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
			totalsum ^= byte;
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

			state = NMEA_STATE_CHECKSUM;
			indexChar = 0;
		}
		else {
			dataframe[indexField][indexChar++] = byte;
			totalsum ^= byte;
		}
		break;
	case NMEA_STATE_CHECKSUM:
		if(indexChar == 2) {
			dataframe[0][0] = (checksum==totalsum); // indicate this frame is valid or not
			memset(readBefore, 0, size); // all field has not been read because this frame is new
			state = NMEA_STATE_WAIT;
		}
		else {
			checksum += ((indexChar++)? 1:16)* ((byte>='0'&&byte<='9')? byte-48:byte-55);
		}
		break;
	}
}




uint8_t NMEA_Get_GGA_UTC_Time(uint8_t *h, uint8_t *m, float *s) {
	static uint8_t hour,minute;
	static float second;

	if(!GGA_frame[GGA_Status][0] ||  GGA_wasFieldReadBefore[GGA_UTC_Time] ) { // read from static variable if
																					// dataframe dataframe is invalid OR this function read it before
		*h = hour;
		*m = minute;
		*s = second;
		return 0;
	}

	*h = hour = (GGA_frame[GGA_UTC_Time][0]-48)*10 + (GGA_frame[GGA_UTC_Time][1]-48);
	*m = minute = (GGA_frame[GGA_UTC_Time][2]-48)*10 + (GGA_frame[GGA_UTC_Time][3]-48);
	*s = second = atof((char*)GGA_frame[GGA_UTC_Time]+4);

	GGA_wasFieldReadBefore[GGA_UTC_Time] = 1; // indicate this frame is read by this function
	return 1;
}


uint8_t NMEA_Get_GGA_Longitude(float *lon, char* e_w) {
	static float longitude;
	static char longi_dd[4];
	static char E_W;

	if(!GGA_frame[GGA_Status][0] ||  GGA_wasFieldReadBefore[GGA_Longitude] ) { // read from static variable if
																					// dataframe dataframe is invalid OR this function read it before
		*lon = longitude;
		*e_w = E_W;
		return 0;
	}

	*e_w = E_W = GGA_frame[GGA_EW_Indicator][0];
	strncpy(longi_dd, (char*)GGA_frame[GGA_Longitude], 3);
	*lon = longitude = 1.0*atoi(longi_dd) + atof((char*)GGA_frame[GGA_Longitude]+3) / 60;

	GGA_wasFieldReadBefore[GGA_Longitude] = 1; // indicate this frame is read by this function
	return 1;

}

uint8_t NMEA_Get_GGA_Latitude(float *lat, char* n_s) {
	static float latitude;
	static char lat_dd[3];
	static char N_S;

	if(!GGA_frame[GGA_Status][0] ||  GGA_wasFieldReadBefore[GGA_Latitude] ) { // read from static variable if
																					// dataframe dataframe is invalid OR this function read it before
		*lat = latitude;
		*n_s = N_S;
		return 0;
	}

	*n_s = N_S = GGA_frame[GGA_NS_Indicator][0];
	strncpy(lat_dd, (char*)GGA_frame[GGA_Latitude], 2);
	*lat = latitude = 1.0*atoi(lat_dd) + atof((char*)GGA_frame[GGA_Latitude]+2) / 60;

	GGA_wasFieldReadBefore[GGA_Latitude] = 1; // indicate this frame is read by this function

	return 1;
}

uint8_t NMEA_Get_GGA_MSL_Altitude(float *alti) {
	static float msl_altitude;

	if(!GGA_frame[GGA_Status][0] || GGA_wasFieldReadBefore[GGA_MSL_Altitude] ) { // read from static variable if
																					// dataframe dataframe is invalid OR this function read it before
		*alti = msl_altitude;
		return 0;
	}

	*alti = msl_altitude = atof((char*)GGA_frame[GGA_MSL_Altitude]);

	GGA_wasFieldReadBefore[GGA_MSL_Altitude] = 1; // indicate this frame is read by this function
	return 1;
}

