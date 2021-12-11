/*
 * Fifo.h
 *
 *  Created on: Nov 16, 2021
 *      Author: fhdtr
 */

#ifndef INC_FIFO_H_
#define INC_FIFO_H_

#include "main.h"

#define FF_SIZE 	100
typedef uint8_t 	DATA_TYPE;


void FF_write(DATA_TYPE data);
DATA_TYPE FF_read();
uint8_t FF_isEmpty();
uint8_t FF_isFull();

#endif /* INC_FIFO_H_ */
