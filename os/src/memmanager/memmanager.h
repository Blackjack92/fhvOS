/*
 * memmanager.h
 *
 *  Created on: 04.04.2015
 *      Author: Marko
 */

#ifndef MEMMANAGER_MEMMANAGER_H_
#define MEMMANAGER_MEMMANAGER_H_

#include "../../../platform/platform.h"

#define PAGE_SIZE_64KB						0x10000
#define PAGE_SIZE_4KB						0x1000
#define MEMORY_REGIONS						8
#define L1_PAGE_TABLE						1
#define L2_PAGE_TABLE						2
#define L2_PAGE_TABLE_SIZE_1KB				0x400
#define L1_PAGE_TABLE_SIZE_16KB				0x4000
#define L1_TABLE_PAGE_COUNT 				4
#define L2_TABLE_PAGE_COUNT 				1

#define FAULT_PAGE_TABLE_ENTRY				0x0
#define FAULT_PAGE_TABLE  0x0
#define MASTER_PAGE_TABLE 0x1
#define COARSE_PAGE_TABLE 0x2

#define L1_PAGE_TABLE_SIZE_16KB			0x4000
#define L2_PAGE_TABLE_SIZE_1KB			0x400

typedef struct PAGESTATUS_T
{
	boolean_t reserved;
	unsigned int processID;
} pageStatus_t;

#define BOOT_ROM_REGION						0
static pageStatus_t romRegion[43];

#define MEMORY_MAPPED_IO_REGION				1
static pageStatus_t mmioRegion[261359];

#define KERNEL_REGION						2
static pageStatus_t kernelRegion[4095];

#define PAGE_TABLE_REGION					3
static pageStatus_t tableRegion[1279];

#define PROCESS_REGION						4
static pageStatus_t procRegion[256767];

#define INTERNAL_SRAM_REGION				5
static pageStatus_t sramRegion[15];

#define BOOT_ROM_EXCEPTIONS_REGION			6
static pageStatus_t romExceptionsRegion[1];

#define HIVECS_REGION						7
static pageStatus_t hivecsRegion[1];

#define BOOT_ROM_EXCEPTIONS_START_ADDRESS	0x20000
#define BOOT_ROM_EXCEPTIONS_END_ADDRESS		0x2001C
#define BOOT_ROM_START_ADDRESS				0x40000000
#define BOOT_ROM_END_ADDRESS				0x4002BFFF
#define MEMORY_MAPPED_IO_START_ADDRESS 		0x40310000
#define MEMORY_MAPPED_IO_END_ADDRESS 		0x7FFFFFFF
#define SDRAM_START_ADDRESS					0x80000000
#define SDRAM_END_ADDRESS					0xBFFFFFFF
#define INTERNAL_SRAM_START_ADDRESS			0x402F0400
#define INTERNAL_SRAM_END_ADDRESS			0x402FFFFF
#define KERNEL_START_ADDRESS				0x80000000
#define KERNEL_END_ADDRESS					0x80FFFFFF
#define PAGE_TABLES_START_ADDRESS			0x81000000
#define PAGE_TABLES_END_ADDRESS				0x814FFFFF
#define HIVECS_START_ADDRESS				0xFFFF0000
#define HIVECS_END_ADDRESS					0xFFFF001C
#define PAGE_TABLE_SPACE					PAGE_TABLES_END_ADDRESS - PAGE_TABLES_START_ADDRESS
#define PROCESS_PAGES_START_ADDRESS			0x00000004				// address changed to prevent null pointer exception
#define PROCESS_PAGES_END_ADDRESS			0x3EAFFFFF
#define PROCESS_MEMORY_SPACE				PROCESS_PAGES_END_ADDRESS - PROCESS_PAGES_START_ADDRESS
#define PAGES_IN_A_ROW_FOUND				0
#define MEMORY_OK							1
#define MEMORY_NOT_OK						-1

typedef pageStatus_t * pageStatusPointer_t;

// Data type for organizing the memory regions given in the ARM TRM.
typedef struct region
{
    unsigned int startAddress;
    unsigned int endAddress;
    unsigned int pageSize;
    unsigned int accessPermission;
    unsigned int cacheBufferAttributes;
    unsigned int numberOfPages;
    unsigned int reservedPages;
    pageStatusPointer_t pageStatus;
} memoryRegion_t;

typedef memoryRegion_t* memoryRegionPointer_t;

typedef uint32_t* pageAddressPointer_t;

extern int MemoryManagerInit();
extern pageAddressPointer_t MemoryManagerGetFreePagesInProcessRegion(unsigned int pagesToReserve);
extern memoryRegionPointer_t MemoryManagerGetRegion(unsigned int memoryRegionNumber);
extern int MemoryManagerReserveAllDirectMappedRegions(void);

#endif /* MEMMANAGER_MEMMANAGER_H_ */
