/*
 * driver_dmx.c
 *
 *  Created on: 05.06.2015
 *      Author: Kevin
 */

#include "driver_dmx.h"

int DMXInit(uint16_t id) {
	return DRIVER_OK;
}

int DMXOpen(uint16_t id) {
	return DRIVER_OK;
}

int DMXClose(uint16_t id) {
	return DRIVER_OK;
}

int DMXWrite(uint16_t id, char* buf, uint16_t len) {
	return DRIVER_OK;
}

int DMXRead(uint16_t id, char* buf, uint16_t len) {
	return DRIVER_OK;
}

int DMXIoctl(uint16_t id, uint16_t cmd, uint8_t mode, char* buf, uint16_t len) {
	return DRIVER_OK;
}
