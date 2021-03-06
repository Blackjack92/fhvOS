/*
 * filesystem.c
 *
 *  Created on: 29.05.2015
 *      Author: Nicolaj Hoess
 */


#include "../includes/filesystem.h"
#include "../includes/systemcall.h"

int read_cwd(char* buf, int len)
{
	int ret = 0;
	systemCallMessage_t message;
	message.systemCallNumber = SYSTEM_CALL_CWD;
	message.messageArgs.callArg = len;
	message.messageArgs.returnBuf = buf;
	message.messageArgs.returnArg = &ret;
	SystemCall(&message);
	return ret;
}

int read_file(char* name, int start, char* buf, int len)
{
	systemCallMessage_t message;
	message.systemCallNumber = SYSTEM_CALL_READ;
	message.messageArgs.callArg = start;
	message.messageArgs.callBuf = name;
	message.messageArgs.returnBuf = buf;
	message.messageArgs.returnArg = &len;
	SystemCall(&message);
	return *message.messageArgs.returnArg;
}

int read_directory(char* name, directoryEntry_t* buf, int len)
{
	systemCallMessage_t message;
	message.systemCallNumber = SYSTEM_CALL_READ_DIR;
	message.messageArgs.callBuf = name;
	message.messageArgs.returnBuf = (char*) buf;
	message.messageArgs.returnArg = &len;
	SystemCall(&message);
	return *message.messageArgs.returnArg;
}

int set_cwd(char* path)
{
	int res = 0;
	systemCallMessage_t message;
	message.systemCallNumber = SYSTEM_CALL_CHDIR;
	message.messageArgs.callBuf = path;
	message.messageArgs.returnArg = &res;
	SystemCall(&message);
	return *message.messageArgs.returnArg;
}
