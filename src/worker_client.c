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
#include <fstream>
using namespace std;

//assignment is understood using Beej's guide
void printAndExit(string x){
		cout<<"usage: "<<x<<" <server hostname> <port>\n";
		exit(1);
}

pthread_mutex_t mtx;
int endRange=0;
int startRage=0;
char* server_hostname;
unsigned short int port_num;//server port number. our port number is auto generated
int client_sockfd;
string data_to_send="";
bool jobCancelled=false;

void* mapReduceJob(void* m1){
	data_to_send="";
	message* msg=(struct  message*)m1;
	ifstream fin;
	unsigned int query_type=msg->Query_Type;
	unsigned int query_len=msg->Query_Length;
	string query(msg->Query);
	//mutex;
	pthread_mutex_lock(&mtx);
	msg->Command=NOT_DONE;
	msg->Data_Range_End=startRage;
	pthread_mutex_unlock(&mtx);
	//mutex end
	string lineBuffer;
	int i=1;
	string lineBuffer2;
	if(query_type==SEARCH_CITATIONS_BY_PATENT_ID){
		fin.open("Citations.txt");
		//skip first line
		getline(fin,lineBuffer);
		while(i<startRage){
			pthread_mutex_lock(&mtx);
			if(jobCancelled){pthread_mutex_unlock(&mtx);break;}
			pthread_mutex_unlock(&mtx);
			getline(fin,lineBuffer);
			i++;
		}
		cout<<query<<endl;
		while(i<endRange){
			pthread_mutex_lock(&mtx);
			if(jobCancelled){pthread_mutex_unlock(&mtx);break;}
			lineBuffer="";
			lineBuffer2="";
			getline(fin,lineBuffer2,',');
			if(lineBuffer2.compare(query)==0){
				//mutex
				getline(fin,lineBuffer);
				data_to_send.append(lineBuffer);
				data_to_send.append("\n");
				//mutex end;
			}else{
				getline(fin,lineBuffer);
			}
			pthread_mutex_unlock(&mtx);
			//getline(fin,lineBuffer);
			i++;
			if(i%1000==0){
				pthread_mutex_lock(&mtx);
				msg->Data_Range_End=i;
				pthread_mutex_unlock(&mtx);
			}
		}
	}
	else{//1 or 3.
		string firstName="";
		string middleName="";
		string lastNmae="";
		//only taking last name :P
		if(query.find(",")!=-1){
			lastNmae=query.substr(0,query.find(","));
			query.erase(0,query.find(",")+1);
			if(query.find(",")!=-1){
			firstName=query.substr(0,query.find(","));
			query.erase(0,query.find(",")+1);
			middleName=query;
			}
			else
				firstName=query;
		}
		else{
			lastNmae=query;
		}
		fin.open("Inventors.txt");
		//skip first line
		getline(fin,lineBuffer);
		while(i<startRage){
			pthread_mutex_lock(&mtx);
			if(jobCancelled){pthread_mutex_unlock(&mtx);break;}
			pthread_mutex_unlock(&mtx);
			getline(fin,lineBuffer);
			i++;
		}
		string patent,my_lname,my_fname,my_mname;
		while(i<endRange){
			pthread_mutex_lock(&mtx);
			if(jobCancelled){pthread_mutex_unlock(&mtx);break;}
			pthread_mutex_unlock(&mtx);
//			getline(fin,lineBuffer,'\"');//pre patent
			getline(fin,patent,',');
			getline(fin,lineBuffer,'\"');
			getline(fin,my_lname,'\"');
			getline(fin,lineBuffer,',');
			getline(fin,lineBuffer,'\"');
			getline(fin,my_fname,'\"');
			getline(fin,lineBuffer,',');
			getline(fin,lineBuffer,'\"');
			getline(fin,my_mname,'\"');

			if(my_lname.compare(lastNmae)==0){
				if(firstName.compare("")==0){
					pthread_mutex_lock(&mtx);
					data_to_send.append(patent);
					data_to_send.append("\n");
					pthread_mutex_unlock(&mtx);
				}
				else if(firstName.compare(my_fname)==0){
					if(middleName.compare("")==0 || middleName.compare(my_mname)==0){
						pthread_mutex_lock(&mtx);
						data_to_send.append(patent);
						data_to_send.append("\n");
						pthread_mutex_unlock(&mtx);
					}
				}
			}
			getline(fin,lineBuffer);			
			i++;
			if(i%1000==0){
				pthread_mutex_lock(&mtx);
				msg->Data_Range_End=i;
				pthread_mutex_unlock(&mtx);
			}
		}
	}
	pthread_mutex_lock(&mtx);
	msg->Command=DONE_NO_DATA_FOUND;
	pthread_mutex_unlock(&mtx);		
}

int main(int argc, char *argv[])
{
	data_to_send="";
	unsigned short int local_port;
	unsigned int query_type;
	char* port_num_char;
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
	pthread_t worker_thread;
	struct addrinfo server_hints,*server_info,*p;
	pthread_mutex_init(&mtx,NULL);
	m1 = (struct message *)NULL;
	/*
     * check command line arguments
     */
    string file_name(argv[0]);
     if(argc < 3){
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
	//creating socket
	if ((client_sockfd = socket(AF_INET, SOCK_DGRAM, 0)) == -1) {
		perror("socket");
		printAndExit(file_name);
	}
	struct timeval tv;
	tv.tv_sec = 6;//timeout of 3 sec
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
	m1->Command = REQUEST_TO_JOIN;
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
			cout<<"No Response From Server after 6 second."<<endl;
		}
		else{
			recieve_buffer[numbBytesRecieved]='\0';
			m1=toMessage(recieve_buffer);
			if(m1->Command==JOB && m1->Magic==MAGIC){
				connected_to_server=true;
			}
		}
		recieve_buffer[numbBytesRecieved]='\0';
	}
	bool job_finished=false;
	struct message* m_rcv=(struct message*)NULL;
	startRage=m1->Data_Range_Start;
	endRange=m1->Data_Range_End;
	m1->Data_Range_End=startRage;
	m1->Command=ACK_JOB;
	toBytes(m1,msg_to_send_array);
	while((numbBytesSent = sendto(client_sockfd, msg_to_send_array, MAXMSGLEN, 0,
		(struct sockaddr *)&server_addr, sizeof(struct sockaddr))) == -1){}
	pthread_create(&worker_thread,NULL,mapReduceJob,(void*)m1);
	tv.tv_sec = 15;//timeout of 3 sec
	tv.tv_usec = 0;
	if (setsockopt(client_sockfd, SOL_SOCKET, SO_RCVTIMEO,&tv,sizeof(tv)) ==-1) {
    	perror("Error");
	}

	while(!job_finished){
		memset(recieve_buffer,'\0', MAXBUFLEN);
		if ((numbBytesRecieved=recvfrom(client_sockfd, recieve_buffer, MAXMSGLEN , 0,(struct sockaddr *)&msg_sender_addr, &addr_len)) == -1) {
			cout<<"No Response From Server after 15 second."<<endl;
			cout<<"Data To Send: "<<data_to_send<<endl;
			continue;
		}
		recieve_buffer[numbBytesRecieved]='\0';
		m_rcv=toMessage(recieve_buffer);
		if(m_rcv->Magic!=MAGIC){continue;}
		print_message_function((void*)m_rcv);
		if(m_rcv->Command==JOB){
			free(m1);
			m1=m_rcv;
			startRage=m1->Data_Range_Start;
			endRange=m1->Data_Range_End;
			m1->Data_Range_End=startRage;
			m1->Command=ACK_JOB;
			toBytes(m1,msg_to_send_array);
			while((numbBytesSent = sendto(client_sockfd, msg_to_send_array, MAXMSGLEN, 0,
				(struct sockaddr *)&server_addr, sizeof(struct sockaddr))) == -1){}
			pthread_create(&worker_thread,NULL,mapReduceJob,(void*)m1);
		}
		else if(m_rcv->Command==PING){
			pthread_mutex_lock(&mtx);
			strcpy(m1->Data,data_to_send.c_str());
			m1->Data_Length=strlen(m1->Data);
			toBytes(m1,msg_to_send_array);
			pthread_mutex_unlock(&mtx);
			while((numbBytesSent = sendto(client_sockfd, msg_to_send_array, MAXMSGLEN, 0,
				(struct sockaddr *)&server_addr, sizeof(struct sockaddr))) == -1){}
			free(m_rcv);
		}
		else if(m_rcv->Command==CANCEL_JOB){
			pthread_mutex_lock(&mtx);
			jobCancelled=true;
			pthread_mutex_unlock(&mtx);
			usleep(100000);
			m1->Command=DONE_NO_DATA_FOUND;
			while((numbBytesSent = sendto(client_sockfd, msg_to_send_array, MAXMSGLEN, 0,
				(struct sockaddr *)&server_addr, sizeof(struct sockaddr))) == -1){}
			free(m_rcv);
		}
	}
	/* Deallocating memmory m1, m2 */
	free(m1);
	close(client_sockfd);
	return 1;
}

