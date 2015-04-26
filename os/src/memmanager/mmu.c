/*
 * mmu.c
 *
 *  Created on: 04.04.2015
 *      Author: Marko Petrovic
 */


#include "mmu.h"

#define MMU_DOMAIN_FULL_ACCESS 0xFFFFFFFF


static void mmuReserveAllDirectMappedRegions();
static void mmuReserveDirectMappedRegion(unsigned int memoryRegion);
static void mmuInitializeKernelMasterPageTable();
static void mmuSetKernelMasterPageTable(pageTablePointer_t table);
static void mmuSetProcessPageTable(pageTablePointer_t table);
static void mmuSetDomainToFullAccess(void);

pageTablePointer_t kernelMasterPageTable;


void MMUInit()
{
	MemoryManagerInit();

	MMUDisable();

	// reserve direct mapped regions
	mmuReserveAllDirectMappedRegions();

	// create master table
	kernelMasterPageTable = MMUCreateMasterPageTable();

	// TODO: initialize the kernel master table so the lookup works for the kernel
	mmuInitializeKernelMasterPageTable();

	// TODO: set kernel table
	mmuSetKernelMasterPageTable(kernelMasterPageTable);

	// TODO: set process table
	mmuSetProcessPageTable(kernelMasterPageTable);

	// set domain access
	mmuSetDomainToFullAccess();

	// enable mmu
	MMUEnable();
}

static void mmuReserveAllDirectMappedRegions(void)
{
	unsigned int memoryRegion;

	for(memoryRegion = 0; memoryRegion < MEMORY_REGIONS; memoryRegion++)
	{
		mmuReserveDirectMappedRegion(memoryRegion);
	}
}

static void mmuReserveDirectMappedRegion(unsigned int memoryRegion)
{
	memoryRegionPointer_t region = MemoryManagerGetSection(memoryRegion);

	if(TRUE == region->directAccess)
	{
		MemoryManagerReserveAllPages(region);
	}
}

pageTablePointer_t MMUCreateMasterPageTable(void)
{
	// reserve a 16kB pagetable in any region
	pageTablePointer_t newTable;
	unsigned int memoryRegion;

	for(memoryRegion = 0; memoryRegion < MEMORY_REGIONS; memoryRegion++)
	{
		newTable = (uint32_t*)MemoryManagerGetFreePagesInSection(memoryRegion, MMU_MASTER_TABLE_PAGE_COUNT);

		if(NULL != newTable)
		{
			break;
		}
	}

	if(NULL == newTable)
	{
		return NULL;
	}

	// set values of the regin to 0
	memset(newTable, 0, PAGE_SIZE * MMU_MASTER_TABLE_PAGE_COUNT);

	return newTable;
}


static void mmuInitializeKernelMasterPageTable()
{

}

static void mmuSetKernelMasterPageTable(pageTablePointer_t table)
{
	MMUFlushTLB();
	//MMUSetProcessTable();
}

static void mmuSetProcessPageTable(pageTablePointer_t table)
{
	MMUFlushTLB();
	//MMUSetProcessTable();
}


static void mmuSetDomainToFullAccess(void)
{
	MMUSetDomainAccess(MMU_DOMAIN_FULL_ACCESS);
}


void MMUHandleDataAbortException()
{

}


void MMUSwitchToProcess(process_t process)
{

}


void MMUInitProcess(process_t process)
{

}