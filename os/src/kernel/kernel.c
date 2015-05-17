/*
 * kernel.c
 *
 *  Created on: 14.05.2015
 *      Author: Marko Petrovic
 *      Contr.: Nicolaj Hoess
 */

#include "kernel.h"
#include "../processmanager/mutex.h"
#include <stdio.h>

static long uptimeTicks = 0;
static mutex_t outputMutex;

long KernelGetUptime()
{
	return uptimeTicks;
}

long KernelTick(int ticks)
{
	return (uptimeTicks += ticks);
}

int	KernelInfo(const char *format, ...)
{
	MutexLock(&outputMutex);

	va_list arg;
	va_start (arg, format);
	int res = 0;
	res += printf("[INFO  @%i] ", KernelGetUptime());
	res += vprintf(format, arg);
	va_end(arg);

	MutexUnlock(&outputMutex);

	return res;
}
int	KernelDebug(const char *format, ...)
{
	MutexLock(&outputMutex);

	va_list arg;
	va_start (arg, format);
	int res = 0;
	res += printf("[DEBUG @ %i] ", KernelGetUptime());
	res += vprintf(format, arg);
	va_end(arg);

	MutexUnlock(&outputMutex);

	return res;
}
int	KernelError(const char *format, ...)
{
	MutexLock(&outputMutex);

	va_list arg;
	va_start (arg, format);
	int res = 0;
	res += printf("[ERROR @%i] ", KernelGetUptime());
	res += vprintf(format, arg);
	va_end(arg);

	MutexUnlock(&outputMutex);

	return res;
}
