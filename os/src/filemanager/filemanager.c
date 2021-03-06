/*
 * filemanager.c
 *
 *  Created on: 19.05.2015
 *      Author: Nicolaj Hoess
 */

#include "filemanager.h"
#include "hardcoded_programs.h"
#include "../devicemanager/devicemanager.h"
#include "../processmanager/processmanager.h"
#include "../filesystem/ff.h"
#include "../driver/sdcard/mmcsd_proto.h"
#include "../kernel/kernel.h"
#include "../loader/loader.h"

#define FILE_MANAGER_MAX_PATH_LENGTH	(512)

// #pragma DATA_ALIGN(fatFileSystem, SOC_CACHELINE_SIZE);
static FATFS fatFileSystem;
typedef struct {
	void *dev;
	FATFS *fs;
	boolean_t initDone;
} externalStorageDevice_t;

extern externalStorageDevice_t fat_devices[2];

static DIR 			dir;
static FILINFO 		fileInfo;

static char currentWorkingDirectory[FILE_MANAGER_MAX_PATH_LENGTH];
static device_t indicatorDevice;

int FileManagerOpenExecutable(char* name, boolean_t searchInGlobalBinPath, int argc, char** argv, boolean_t blocking, context_t* context)
{
	if (searchInGlobalBinPath)
	{
		// Search in global bin path first (this includes hardcoded user-processes)
		void (*funcPtr)(int, char ** ) = HardCodedProgramsGetProgram(name);
		if (funcPtr != NULL)
		{
			process_t* proc = ProcessManagerStartProcess(name, funcPtr, argc, argv, blocking, context);
			return FILE_MANAGER_OK;
		}


		char fullPath[FILE_MANAGER_MAX_PATH_LENGTH];
		sprintf(fullPath, "%s/%s%s\0", FILE_MANAGER_BIN_PATH, name, FILE_MANAGER_EXTENSION);

		int size = FileManagerGetFileSize(fullPath);
		if (size > 0)
		{
			char* file = malloc(sizeof(char) * size);
			if (FileManagerOpenFile(fullPath, 0, file, size) < 0)
			{
				free(file);
				return FILE_MANAGER_NOT_FOUND;
			}
			LoaderLoad(name, file, size, argc, argv, blocking, context);
			return FILE_MANAGER_OK;
		}
		KernelVerbose("Did not find command in global bin path\n");
	}

	char fullPath[FILE_MANAGER_MAX_PATH_LENGTH + FILE_MANAGER_MAX_PATH_LENGTH];

	if (currentWorkingDirectory[strlen(currentWorkingDirectory) - 1] == '/')
	{
		sprintf(fullPath, "%s%s\0", currentWorkingDirectory, name);
	}
	else
	{
		sprintf(fullPath, "%s/%s\0", currentWorkingDirectory, name);
	}

	KernelDebug("Opening file %s\n", fullPath);

	int size = FileManagerGetFileSize(fullPath);
	KernelDebug("File size = %d\n", size);
	if (size <= 0)
	{
		KernelDebug("File not found\n");
		return FILE_MANAGER_NOT_FOUND;
	}

	char* fileBuf = malloc(sizeof(char) * size);
	if (FileManagerOpenFile(fullPath, 0, fileBuf, size) < 0)
	{
		KernelDebug("Error reading file\n");
		free(fileBuf);
		return FILE_MANAGER_NOT_FOUND;
	}

	// fileBuf contains the whole program
	LoaderLoad(name, fileBuf, size, argc, argv, blocking, context);

	return FILE_MANAGER_OK;
}

int FileManagerInit(device_t device, device_t indDevice) {
	indicatorDevice = indDevice;
	DeviceManagerOpen(indicatorDevice);

	DeviceManagerWrite(indicatorDevice, "1", 1);

	if (DeviceManagerInitDevice(device) != DRIVER_OK)
	{
		return FILE_MANAGER_NO_DEVICE_FOUND;
	}
	DeviceManagerOpen(device);

	int length = sizeof(mmcsdCardInfo);
	char* sdCard = (char*) malloc(length);

	if (DeviceManagerRead(device, sdCard, length) != DRIVER_OK)
	{
		DeviceManagerWrite(indicatorDevice, "0", 1);
		return FILE_MANAGER_NO_DEVICE_FOUND;
	}

	mmcsdCardInfo* card = (mmcsdCardInfo*)(sdCard);

	//mountFatDevice(0, card);

	f_mount(0, &fatFileSystem);
	fat_devices[0].dev = card;
	fat_devices[0].fs = &fatFileSystem;
	fat_devices[0].initDone = false;

	strncpy(currentWorkingDirectory, FILE_MANAGER_ROOT_PATH, FILE_MANAGER_MAX_PATH_LENGTH);

	DeviceManagerWrite(indicatorDevice, "0", 1);
	return FILE_MANAGER_OK;
}

int FileManagerListDirectoryContent(const char* name, directoryEntry_t* buf, int length) {
	KernelDebug("FileManager reading directory %s\n", name);

	volatile FRESULT fresult;

	DeviceManagerWrite(indicatorDevice, "1", 1);
	if (f_opendir(&dir, currentWorkingDirectory))
	{
		DeviceManagerWrite(indicatorDevice, "0", 1);
		return FILE_MANAGER_NOT_FOUND;
	}

	int cnt = 0;
	while (1) {
		if (f_readdir(&dir, &fileInfo) != FR_OK)
		{
			// Error occured while reading: Doing cleanup of the incomplete results and return
			memset(buf, 0, sizeof(entryType_t) * length);
			DeviceManagerWrite(indicatorDevice, "0", 1);
			return FILE_MANAGER_NOT_FOUND;
		}

		// If the file name is blank, then this is the end of the listing.
		if (!fileInfo.fname[0]) { break; }

		/*printf("%c%c%c%c%c %u/%02u/%02u %02u:%02u %9u  %s\n",
				(fileInfo.fattrib & AM_DIR) ? 'D' : '-',
				(fileInfo.fattrib & AM_RDO) ? 'R' : '-',
				(fileInfo.fattrib & AM_HID) ? 'H' : '-',
				(fileInfo.fattrib & AM_SYS) ? 'S' : '-',
				(fileInfo.fattrib & AM_ARC) ? 'A' : '-',
				(fileInfo.fdate >> 9) + 1980,
				(fileInfo.fdate >> 5) & 15, fileInfo.fdate & 31,
				(fileInfo.ftime >> 11),
				(fileInfo.ftime >> 5) & 63, fileInfo.fsize,
				fileInfo.fname);*/

		buf[cnt].type = (fileInfo.fattrib & AM_DIR) ? TYPE_DIRECTORY : TYPE_FILE;
		buf[cnt].size = fileInfo.fsize;
		strncpy(buf[cnt].name, fileInfo.fname, FILE_MANAGER_MAX_FILE_LENGTH);
		cnt++;
		if (cnt >= length) {
			DeviceManagerWrite(indicatorDevice, "0", 1);
			return FILE_MANAGER_BUFFER_TOO_SMALL;
		}
	}

	DeviceManagerWrite(indicatorDevice, "0", 1);
	KernelDebug("FileManager read %d directory entries\n", cnt);

	return FILE_MANAGER_OK;
}

int FileManagerOpenFile(const char* name, int startByte, char* buf, int length) {
	DeviceManagerWrite(indicatorDevice, "1", 1);
	KernelDebug("FileManager opening file %s\n", name);
	FIL file;
	if (f_open(&file, name, FA_READ) != FR_OK)
	{
		DeviceManagerWrite(indicatorDevice, "0", 1);
		return FILE_MANAGER_NOT_FOUND;
	}

	unsigned short read = 0;

	char* temp_buffer = malloc(sizeof(char) * length);
	if (f_read(&file, temp_buffer, length, &read) != FR_OK)
	{
		free(temp_buffer);
		DeviceManagerWrite(indicatorDevice, "0", 1);
		return FILE_MANAGER_NOT_FOUND;
	}

	memset(buf, '\0', length);
	memcpy(buf, temp_buffer, read);
	f_close(&file);

	free(temp_buffer);

	KernelDebug("FileManager read %d in a buffer of %d\n", read, length);
	DeviceManagerWrite(indicatorDevice, "0", 1);
	return (read > length) ? FILE_MANAGER_BUFFER_TOO_SMALL : FILE_MANAGER_OK;
}

int FileManagerSetCurrentWorkingDirectory(char *name)
{
	KernelDebug("FileManager setting cwd to %s\n", name);

	// Check if the given cwd is valid acc. string length
	unsigned int len = strlen(name);
	if (len > FILE_MANAGER_MAX_PATH_LENGTH)
	{
		return FILE_MANAGER_NOT_FOUND;
	}

	// Check if we can open the directory
	if (f_opendir(&dir, name) != FR_OK)
	{
		return FILE_MANAGER_NOT_FOUND;
	}

	strncpy(currentWorkingDirectory, name, len + 1);

	return FILE_MANAGER_OK;
}

int FileManagerGetCurrentWorkingDirectory(char *buf, int len)
{
	unsigned int cwd_len = strlen(currentWorkingDirectory);
	if (len < cwd_len)
	{
		return FILE_MANAGER_BUFFER_TOO_SMALL;
	}

	strncpy(buf, currentWorkingDirectory, cwd_len + 1);

	return FILE_MANAGER_OK;
}


int FileManagerGetFileSize(char* name)
{
	DeviceManagerWrite(indicatorDevice, "1", 1);
	KernelDebug("FileManager opening file %s\n", name);
	FIL file;
	if (f_open(&file, name, FA_READ) != FR_OK)
	{
		DeviceManagerWrite(indicatorDevice, "0", 1);
		return -1;
	}

	f_close(&file);
	DeviceManagerWrite(indicatorDevice, "0", 1);
	return file.fsize;
}
