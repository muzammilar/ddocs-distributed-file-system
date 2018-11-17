#ifndef MY_UTIL_H
#define MY_UTIL_H

#include "message.h"
#include <string.h>
#include <string>
#include <iostream>

#define MAXMSGLEN         2048
#define MAXBUFLEN              2049

struct message_and_sender{
	struct message* msg;
	char* senderAddress;
	unsigned short int senderPort;
};

struct job{
    unsigned int requester_id;
    unsigned int worker_id;
    message* msg;
};

struct client_life{
    unsigned int client_id;
    unsigned int life;
    std::string data_to_send;
};

struct worker_job{
    unsigned int worker_id;
    unsigned int life;
    unsigned int port;
    std::string ip;
    message* msg;
};

//unsigned int getFreePort(unsigned int my_port);
void toBytes(message *m,char * buf);
message* toMessage(char* rcvBuffer);
#endif