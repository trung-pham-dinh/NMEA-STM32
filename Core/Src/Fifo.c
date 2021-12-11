/*
 * Fifo.c
 *
 *  Created on: Nov 16, 2021
 *      Author: fhdtr
 */


#include "Fifo.h"

static DATA_TYPE buffer[FF_SIZE];

static uint32_t front = 0;
static uint32_t rear = 0;
static uint32_t count = 0;

void FF_write(DATA_TYPE data) {
	if(count >= FF_SIZE) return;

	buffer[rear++] = data;
	if(rear == FF_SIZE) {
		rear = 0;
	}
	count++;
}

DATA_TYPE FF_read() {
	if(count <= 0) return 0;

	DATA_TYPE temp = buffer[front++];

	if(front == FF_SIZE) {
		front = 0;
	}

	count--;

	return temp;
}

uint8_t FF_isEmpty() {
	return (count == 0);
}

uint8_t FF_isFull() {
	return (count == FF_SIZE);
}
