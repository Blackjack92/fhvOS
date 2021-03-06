/*
 * scheduler.h
 *
 *  Created on: 01.04.2015
 *      Author: Kevin
 */

#ifndef SCHEDULER_SCHEDULER_H_
#define SCHEDULER_SCHEDULER_H_

#include "../platform/platform.h"
#include "../devicemanager/devicemanager.h"

/*
 * Defines for better understanding
 */
#define SCHEDULER_OK		(0)
#define SCHEDULER_ERROR		(-1)

#define PROCESSES_MAX		(64)
#define REGISTER_COUNT		(13)

#define INVALID_PROCESS_ID	(-1)

/*
 * Typedefs for the process struct
 */
typedef int8_t processId_t;
typedef void (*processFunc)(int, char**);

typedef address_t* register_t;
typedef struct process_t_struct process_t;

/*
 * Enums for the process struct
 */
typedef enum {
	READY = 0,
	RUNNING = 1,
	BLOCKED = 2,
	SLEEPING = 4,
	FREE = 8
} processState_t;

// Process switch context
#pragma pack(push,1)
typedef struct {
	address_t* cpsr;
	address_t* lr;
	address_t* sp;
	register_t registers[REGISTER_COUNT];
	address_t* pc;
} context_t;
#pragma pack(pop)

struct process_t_struct {
	processId_t id;
	processFunc func;
	processState_t state;

	context_t* context;
	address_t* pageTableL1;

	long wakeupTime;

	process_t* parent;
	boolean_t blockedState;

	address_t* temp_pc;

	char* programData;
	unsigned int programDataLen;
};

/*
 * Scheduler functions
 */
extern int SchedulerInit(device_t stdoutDevice);
extern int SchedulerStart(device_t timerDevice);
extern process_t* SchedulerStartProcess(processFunc func);
extern int SchedulerRunNextProcess(context_t* context);
extern int SchedulerKillProcess(processId_t id);
extern process_t* SchedulerGetRunningProcess(void);
extern void SchedulerBlockProcess(processId_t process);
extern void SchedulerUnblockProcess(processId_t process);
extern void SchedulerSleepProcess(processId_t process, unsigned int millis);
extern void SchedulerDisableScheduling(void);
extern void SchedulerEnableScheduling(void);

#endif /* SCHEDULER_SCHEDULER_H_ */
