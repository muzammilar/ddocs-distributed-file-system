#ifndef MY_UTIL_C
#define MY_UTIL_C

#include "my_util.h"
#include <stdio.h>
#include <string.h>
void toBytes(message *m,char* buf)
{
	memcpy(buf,m,MAXMSGLEN);
}

message* toMessage(char* rcvBuffer)
{
	message* msg = new message();
	memcpy(msg, rcvBuffer, MAXMSGLEN);
	return msg;
}

#endif
