/*
 * uartDriver.h
 *
 *  Created on: 20.03.2015
 *      Author: Kevin
 */

#ifndef DRIVER_UART_UARTDRIVER_H_
#define DRIVER_UART_UARTDRIVER_H_

#include "../../hal/platform/platform.h"
#include "../../hal/uart/uartHal.h"

void UARTDriverInit(uart_t);
void UARTDriverWrite(uart_t, uint8_t* msg);

#endif /* DRIVER_UART_UARTDRIVER_H_ */
