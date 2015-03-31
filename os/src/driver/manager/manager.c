/*
 * manager.c
 *
 *  Created on: 17.03.2015
 *      Author: Nicolaj Hoess
 */

#include "manager.h"
#include <stdlib.h>

#include "../led/led.h"
#include "../uart/uartDriver.h"

#define MAX_DRIVER		255

static driver_t* drivers[MAX_DRIVER];

void DriverManagerInit()
{
	// Create all drivers

	// LED Driver
	driver_t* led = malloc(sizeof(driver_t));
	led->init = &LEDInit;
	led->open = &LEDOpen;
	led->close = &LEDClose;
	led->read = &LEDRead;
	led->write = &LEDWrite;
	led->ioctl = &LEDIoctl;
	drivers[DRIVER_ID_LED] = led;

	// UART Driver
	driver_t* uart = malloc(sizeof(driver_t));
	uart->init = &UARTDriverInit;
	uart->open = &UARTDriverOpen;
	uart->close = &UARTDriverClose;
	uart->read = &UARTDriverRead;
	uart->write = &UARTDriverWrite;
	uart->ioctl = &UARTDriverIoctl;
	drivers[DRIVER_ID_UART] = uart;
}

driver_t* DriverManagerGetDriver(driver_id_t driver_id)
{
	return drivers[driver_id];
}

void DriverManagerDestruct()
{
	int i;
	for (i = 0; i < MAX_DRIVER; i++) {
		if (drivers[i] != NULL) {
			free(drivers[i]);
			drivers[i] = NULL;
		}
	}
}
