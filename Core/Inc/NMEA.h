/*
 * NMEA.h
 *
 *  Created on: Dec 10, 2021
 *      Author: fhdtr
 */

#ifndef INC_NMEA_H_
#define INC_NMEA_H_
#include "main.h"

typedef enum {
  GGA_UTC_Time,
  GGA_Latitude,
  GGA_NS_Indicator,
  GGA_Longitude,
  GGA_EW_Indicator,
  GGA_Position_Fix_Indicator,
  GGA_Satellites_Used,
  GGA_HDOP,
  GGA_MSL_Altitude,
  GGA_Units_1,
  GGA_Geoid_Separation,
  GGA_Units_2,
  GGA_Age_of_Diff_Corr,
  GGA_Diff_Ref_Station_ID,
  GGA_Checksum
} GGA_Data;

typedef enum {
	GGA,
	GLL
}NMEA_ID;

void NMEA_Init(UART_HandleTypeDef* huart);
void NMEA_ReadByte();
void NMEA_Parser();

#endif /* INC_NMEA_H_ */
