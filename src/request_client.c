/* main.c */
#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <math.h>
#include <pthread.h>
#include "message.h"
#include "my_util.h"
#include <vector>
#include <unistd.h>
#include <string>
#include <cstring>
#include <sys/types.h> // standard system types
#include <netinet/in.h> // Internet address structures
#include <sys/socket.h> // socket API
#include <arpa/inet.h>
#include <netdb.h> // host to IP resolution
using namespace std;

//assignment is understood using Beej's guide
void printAndExit(string x){
		cout<<"usage: "<<x<<" <server hostname> <port> <query_type> <query>\n";
		exit(1);
}

int main(int argc, char *argv[])
{
	unsigned short int port_num;//server port number. our port number is auto generated
	unsigned short int local_port;
	unsigned int query_type;
	char* port_num_char;
	char* server_hostname;
	struct hostent* server_ent;
	struct sockaddr_in server_addr;
	struct sockaddr_in req_client_addr;
	struct sockaddr_in msg_sender_addr;
	socklen_t addr_len;
	int numbBytesRecieved;
	int numbBytesSent;
	struct message *m1;
	char recieve_buffer[MAXBUFLEN];
	char msg_to_send_array[MAXMSGLEN];
	int client_sockfd;
	struct addrinfo server_hints,*server_info,*p;
	m1 = (struct message *)NULL;
	/*
     * check command line arguments
     */
    string file_name(argv[0]);
     if(argc < 5){
		perror("Arguments");
		printAndExit(file_name);
	}
	if((server_ent=gethostbyname(argv[1])) == NULL){
		herror("gethostbyname");
		printAndExit(file_name);
	}
	port_num_char=argv[2];
	port_num=(short)(atoi(port_num_char));
	if(port_num> 65535){
		perror("Port Number");
		printAndExit(file_name);
	}
	string query_string(argv[3]);
	if(query_string.compare("SEARCH_PATENTS_BY_INVENTOR")==0){
		query_type=SEARCH_PATENTS_BY_INVENTOR;
	}
	else if(query_string.compare("SEARCH_CITATIONS_BY_PATENT_ID")==0){
		query_type=SEARCH_CITATIONS_BY_PATENT_ID;		
	}
	else if(query_string.compare("SEARCH_CITATIONS_BY_INVENTOR")==0){
		query_type=SEARCH_CITATIONS_BY_INVENTOR;				
	}
	else{
		perror("Query");
		printAndExit(file_name);
	}

	//creating socket
	if ((client_sockfd = socket(AF_INET, SOCK_DGRAM, 0)) == -1) {
		perror("socket");
		printAndExit(file_name);
	}
	struct timeval tv;
	tv.tv_sec = 3;//timeout of 3 sec
	tv.tv_usec = 0;
	if (setsockopt(client_sockfd, SOL_SOCKET, SO_RCVTIMEO,&tv,sizeof(tv)) ==-1) {
    	perror("Error");
	}

	memset(&server_hints,0,sizeof(server_hints));
	server_hints.ai_family=AF_INET;//ipv4
	server_hints.ai_socktype=SOCK_DGRAM;
	int addrinfovalue;
	if((addrinfovalue = getaddrinfo(argv[1], port_num_char, &server_hints, &server_info)) != 0){
		perror("Server Not Found");
		printAndExit(file_name);
	}

	m1 = (struct message *) malloc(sizeof(struct message));
	
	m1->Magic = MAGIC;
	m1->Client_ID = 0;
	m1->Command = QUERY;
	m1->Data_Range_Start =0;
	m1->Data_Range_End = 0;
	m1->Next_Message = 0;
	m1->Query_Type = query_type;
	strcpy(m1->Query, argv[4]);
	m1->Query_Length = strlen(m1->Query);
	m1->Data_Length = 0;
	strcpy(m1->Data, "");
	if (m1->Query_Length > MAX_QLEN){
		printf("Too big query size");
		printf("usage: %s <server hostname> <port> <query_type> <query>\n",argv[0]);
		return 0;
	}


	//send msg to server and wait for response
	server_addr.sin_family=AF_INET;
	server_addr.sin_port=htons((short)port_num);
	server_addr.sin_addr=*((struct in_addr *)server_ent->h_addr);
	memset(&(server_addr.sin_zero), '\0', 8); // zero the rest of the struct

	//bind to a local port
	req_client_addr.sin_family=AF_INET;
	req_client_addr.sin_addr.s_addr=htonl(INADDR_ANY);//my address
	req_client_addr.sin_port=0;//random local port
	bind(client_sockfd,(struct sockaddr*)& req_client_addr,sizeof(req_client_addr));
	unsigned int sockaddr_size_client=sizeof(req_client_addr);
	if(getsockname(client_sockfd,(struct sockaddr*)&req_client_addr,&(sockaddr_size_client))!=0){
		close(client_sockfd);
		perror("getsockname");
		printAndExit(argv[0]);		
	}
	local_port=ntohs(req_client_addr.sin_port);
//	cout<<"The local IP address is: "<<inet_ntoa(req_client_addr.sin_addr)<<endl;
	cout<<"The local port bound for connection: "<<local_port<<endl;
	addr_len = sizeof(struct sockaddr);

	//request and connect the server
	struct message_and_sender* msg_and_sender=(struct message_and_sender*)NULL;
	toBytes(m1,msg_to_send_array);
	bool connected_to_server=false;
	while(!connected_to_server){
		while ((numbBytesSent = sendto(client_sockfd, msg_to_send_array, MAXMSGLEN, 0,
				(struct sockaddr *)&server_addr, sizeof(struct sockaddr))) == -1) {
			cout<<"Send Error! Retrying."<<endl;
		}
		//recieve the okay message.
		cout<<"Requesting to Join"<<endl;
		memset(recieve_buffer,'\0', MAXBUFLEN);
		if ((numbBytesRecieved=recvfrom(client_sockfd, recieve_buffer, MAXMSGLEN , 0,(struct sockaddr *)&msg_sender_addr, &addr_len)) == -1) {
			cout<<"No Response From Server after 3 second."<<endl;
		}
		else{
			recieve_buffer[numbBytesRecieved]='\0';
			msg_and_sender= new message_and_sender();
			msg_and_sender->msg=toMessage(recieve_buffer);
			if(msg_and_sender->msg->Command==ACK_JOB && msg_and_sender->msg->Magic==MAGIC){
				connected_to_server=true;
			}
		}
		recieve_buffer[numbBytesRecieved]='\0';
	}
	m1->Client_ID=msg_and_sender->msg->Client_ID;
	m1->Command=PING;
	bool job_finished=false;
	message* m_rcv=(struct message*)NULL;
	toBytes(m1,msg_to_send_array);
	while(!job_finished){
		while ((numbBytesSent = sendto(client_sockfd, msg_to_send_array, MAXMSGLEN, 0,
				(struct sockaddr *)&server_addr, sizeof(struct sockaddr))) == -1) {
			cout<<"Send Error! Retrying."<<endl;
		}
		//recieve the okay message.
		memset(recieve_buffer,'\0', MAXBUFLEN);
		if ((numbBytesRecieved=recvfrom(client_sockfd, recieve_buffer, MAXMSGLEN , 0,(struct sockaddr *)&msg_sender_addr, &addr_len)) == -1) {
			cout<<"No Response From Server after 3 second."<<endl;
			usleep(2000000);//sleep so that total time=5 sec.
		}else{
			recieve_buffer[numbBytesRecieved]='\0';
			//process message
			m_rcv=toMessage(recieve_buffer);
			if(m_rcv->Magic!=MAGIC){continue;}
			//print_message_function((void*)m_rcv);
		    if (m_rcv->Data_Length > 0) {
		        printf("Data (%d Bytes) : %s\n", m_rcv->Data_Length, m_rcv->Data);
    		}

			if(m_rcv->Command==DONE_NO_DATA_FOUND || m_rcv->Command==DONE_DATA_FOUND){
				job_finished=true;
			}
			free(m_rcv);
			usleep(5000000);
		}
	}
	/* Deallocating memmory m1, m2 */
	free(m1);
	close(client_sockfd);
	return 1;
}

