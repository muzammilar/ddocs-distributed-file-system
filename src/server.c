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
#include <algorithm> 
using namespace std;
void sendMessageOrReply(char* senderAddress, unsigned short int senderPort, struct message* msg);

//globals
int global_client_id=1432;
pthread_mutex_t mtx;
int citations_size;
int inventors_size;
int citation_inc=3000000;
int inventor_inc=1000000;
pthread_mutex_t mtx_jobs;
vector<worker_job> my_workers;
pthread_mutex_t mtx_workers;
vector<job> my_jobs;
pthread_mutex_t mtx_clients;
vector<client_life> my_clients;
//assignment is understood using Beej's guide
void printAndExit(string x){
		cout<<"usage: "<<x<<" <port>\n";
		exit(1);
}
//I have a condition that clients will come before workers

void sendCancelJob(unsigned int id){
	for (vector<worker_job>::iterator iter = my_workers.begin(); iter != my_workers.end(); ++iter){
		if (iter->worker_id==id){
			struct message m2;
			m2.Magic=MAGIC;
			m2.Command=CANCEL_JOB;
			m2.Client_ID=id;
			m2.Next_Message = 0;
			m2.Query_Type = 1;
			strcpy(m2.Query, "");
			m2.Query_Length = 0;
			m2.Data_Length = 0;
			strcpy(m2.Data, "");
			m2.Data_Range_Start=0;
			m2.Data_Range_End=0;
			sendMessageOrReply((char*)((iter->ip).c_str()),iter->port,&m2);
		}
	}
}


void* serverKillerThread(void* xyc){
	while(true){
		pthread_mutex_lock(&mtx_jobs);//cout<<"locking jobs"<<endl;
		pthread_mutex_lock(&mtx_clients);//cout<<"locking clients"<<endl;
		pthread_mutex_lock(&mtx_workers);//cout<<"locking workers"<<endl;
		//cancel jobs where client life =0
		for (vector<client_life>::iterator cli_it = my_clients.begin(); cli_it != my_clients.end();)
		{
			if(cli_it->life==0){
				//send cancel job to all workers
				for (vector<job>::iterator job_iter = my_jobs.begin(); job_iter != my_jobs.end();)
				{
					if(cli_it->client_id==job_iter->requester_id){
						sendCancelJob(job_iter->worker_id);
						job_iter=my_jobs.erase(job_iter);
					}else{
						++job_iter;
					}
				}
				cli_it=my_clients.erase(cli_it);
			}
			else{
				 ++cli_it;
			}
		}
		//delete those workers where life=0;
		for (vector<worker_job>::iterator cli_it = my_workers.begin(); cli_it != my_workers.end();)
		{
			if(cli_it->life==0){
				//send cancel job to all workers
				for (vector<job>::iterator job_iter = my_jobs.begin(); job_iter != my_jobs.end();)
				{
					if(cli_it->worker_id==job_iter->worker_id){
						job_iter->worker_id=0;
					}else{
						++job_iter;
					}
				}
				cli_it=my_workers.erase(cli_it);
			}
			else{
				 ++cli_it;
			}
		}



		//make life of everyone to zero
		for (vector<client_life>::iterator cli_it = my_clients.begin(); cli_it != my_clients.end(); ++cli_it){
			cli_it->life=0;
		}
		for (vector<worker_job>::iterator iter = my_workers.begin(); iter != my_workers.end(); ++iter){
			iter->life=0;
		}
		pthread_mutex_unlock(&mtx_workers);
		pthread_mutex_unlock(&mtx_clients);
		pthread_mutex_unlock(&mtx_jobs);

		//go to sleep
		usleep(15000000);
	}
}

void* serverPingerThread(void* xyz){
	char* worker_ip=NULL;
	struct message m1;
	while(true){
		pthread_mutex_lock(&mtx_jobs);//cout<<"locking jobs"<<endl;
		pthread_mutex_lock(&mtx_workers);//cout<<"locking workers"<<endl;
		for (vector<worker_job>::iterator iter = my_workers.begin(); iter != my_workers.end(); ++iter)
		{
			m1.Magic=MAGIC;
			m1.Command=PING;
			m1.Client_ID=iter->worker_id;
			m1.Data_Range_Start =0;
			m1.Data_Range_End = 0;
			m1.Next_Message = 0;
			if(iter->msg==NULL){
				m1.Query_Type = 1;
				strcpy(m1.Query, "");
				m1.Query_Length = 0;
				m1.Data_Length = 0;
				strcpy(m1.Data, "");
			}
			else{
				m1.Query_Type = iter->msg->Query_Type;
				strcpy(m1.Query, iter->msg->Query);
				m1.Query_Length = strlen(m1.Query);
				m1.Data_Length = 0;
				strcpy(m1.Data, "");
			}
	//		cout<<"Sending Reply"<<endl;
			sendMessageOrReply((char*)((iter->ip).c_str()),iter->port,&m1);
		}	
		pthread_mutex_unlock(&mtx_workers);
		pthread_mutex_unlock(&mtx_jobs);
		usleep(5000000);
	}
}

void sendMessageOrReply(char* senderAddress, unsigned short int senderPort, struct message* msg){
	int sockfd;
	struct sockaddr_in reciever_addr;
	struct hostent* he;
	he=gethostbyname(senderAddress);
	sockfd = socket(PF_INET, SOCK_DGRAM, 0);//no error checking :P
	reciever_addr.sin_family=AF_INET;
	reciever_addr.sin_port=htons(senderPort);
	reciever_addr.sin_addr=*((struct in_addr *)he->h_addr);
	memset(&(reciever_addr.sin_zero), '\0', 8);
	char c[MAXMSGLEN];
	toBytes(msg,c);
	sendto(sockfd, c, MAXMSGLEN, 0,(struct sockaddr *)&reciever_addr, sizeof(struct sockaddr));
	close(sockfd);
}

//pinger, killer,job_assigner thread
void printCurrentClients(){
	pthread_mutex_lock(&mtx_clients);//cout<<"locking clients"<<endl;
    printf("\n\n-------------------------------------------------\n");
	cout<<"//////Current Clients"<<endl;
	for (vector<client_life>::iterator iter = my_clients.begin(); iter != my_clients.end(); ++iter)
	{
		cout<<"Client ID: "<<iter->client_id<<endl;
		cout<<"Life: "<<iter->life<<endl;
		cout<<"Data To Send: "<<iter->data_to_send<<endl;
	}
    printf("-------------------------------------------------\n");
	pthread_mutex_unlock(&mtx_clients);
}
void printCurrentJobs(){
	pthread_mutex_lock(&mtx_jobs);//cout<<"locking jobs"<<endl;
    printf("\n\n-------------------------------------------------\n");
	cout<<"Priniting Total Jobs"<<endl;
	for (vector<job>::iterator iter = my_jobs.begin(); iter != my_jobs.end(); ++iter)
	{
		cout<<"Client ID: "<<iter->requester_id<<endl;
		cout<<"Worker ID: "<<iter->worker_id<<endl;
		print_message_function((void*)( iter->msg));
	}
    printf("-------------------------------------------------\n");
	pthread_mutex_unlock(&mtx_jobs);
}

void printCurrentWorkers(){
	pthread_mutex_lock(&mtx_workers);//cout<<"locking workers"<<endl;
    printf("-------------------------------------------------\n");
	cout<<"Printing Workers"<<endl;
	for (vector<worker_job>::iterator iter = my_workers.begin(); iter != my_workers.end(); ++iter)
	{
		cout<<"Worker ID: "<<iter->worker_id<<endl;
		cout<<"life: "<<iter->life<<endl;
		cout<<"IP: "<<iter->ip<<endl;
		cout<<"Port: "<<iter->port<<endl;
		if(iter->msg!=NULL)
			print_message_function((void*)( iter->msg));
	}	
    printf("-------------------------------------------------\n");
	pthread_mutex_unlock(&mtx_workers);
}

void addWorkerClient(unsigned int id,message_and_sender* msg){
	struct worker_job new_worker;
	new_worker.worker_id=id;
	new_worker.life=10;
	new_worker.msg=(struct message*)NULL;
	new_worker.port=msg->senderPort;
	new_worker.ip=msg->senderAddress;
	my_workers.push_back(new_worker);
}

void giveWorkerSomeJob(unsigned int id,char* senderAddress, unsigned short int senderPort){
	//before looking into the vector random shuffle it!
	random_shuffle(my_workers.begin(),my_workers.end());
	//now look into the vector
	for (vector<worker_job>::iterator iter = my_workers.begin(); iter != my_workers.end(); ++iter)
	{
		if (iter->worker_id==id){
			iter->msg=(struct message*)NULL;
			for (vector<job>::iterator jiter =my_jobs.begin(); jiter != my_jobs.end(); ++jiter){
				if(jiter->worker_id==0){
					jiter->worker_id=id;
					iter->msg=jiter->msg;
					jiter->msg->Client_ID=id;
					sendMessageOrReply(senderAddress,senderPort,jiter->msg);
					return;
				}
			}
		}
	}
}

void incrementWorkerLife(unsigned int id){
	for (vector<worker_job>::iterator iter = my_workers.begin(); iter != my_workers.end(); ++iter)
	{
		if (iter->worker_id==id){
			iter->life+=1;
		}
	}	
}

void incrementClientLife(unsigned int id, struct message_and_sender* msg_and_sender){
	for (vector<client_life>::iterator iter = my_clients.begin(); iter != my_clients.end(); ++iter)
	{
		if(iter->client_id==id){
			iter->life+=1;
			struct message m1;
			m1.Magic=MAGIC;
			m1.Command=NOT_DONE;
			m1.Client_ID=id;
			m1.Data_Range_Start =0;
			m1.Data_Range_End = 0;
			m1.Next_Message = 0;
			m1.Query_Type = msg_and_sender->msg->Query_Type;
			strcpy(m1.Query, msg_and_sender->msg->Query);
			m1.Query_Length = strlen(m1.Query);
			strcpy(m1.Data, (iter->data_to_send).c_str());
			iter->data_to_send="";
			m1.Data_Length = strlen(m1.Data);
			int number_left=0;
			for (vector<job>::iterator jiter =my_jobs.begin(); jiter != my_jobs.end(); ++jiter){
				if(jiter->requester_id==id){
					number_left+=1;
				}
			}
			if(number_left==0){
				m1.Command=DONE_DATA_FOUND;
			}
	//		cout<<"Sending Reply"<<endl;
			sendMessageOrReply(msg_and_sender->senderAddress,msg_and_sender->senderPort,&m1);
			iter->data_to_send="";
		}
	}
}

void* processMessage(void* msg_and_sender2){
	struct message_and_sender* msg_and_sender=(struct message_and_sender*) msg_and_sender2;
	cout<<"Message from "<<msg_and_sender->senderAddress<<","<<msg_and_sender->senderPort<<" recieved.\n";
	//print_message_function((void*) msg_and_sender->msg);
	struct message m1;		
	struct message* m2;
	if(msg_and_sender->msg->Command==QUERY){
		pthread_mutex_lock(&mtx_jobs);//cout<<"locking jobs"<<endl;
		global_client_id+=231;
		pthread_mutex_unlock(&mtx_jobs);
		m1.Magic=MAGIC;
		m1.Command=ACK_JOB;
		m1.Client_ID=global_client_id;
		m1.Data_Range_Start =0;
		m1.Data_Range_End = 0;
		m1.Next_Message = 0;
		m1.Query_Type = msg_and_sender->msg->Query_Type;
		strcpy(m1.Query, msg_and_sender->msg->Query);
		m1.Query_Length = strlen(m1.Query);
		m1.Data_Length = 0;
		strcpy(m1.Data, "");
//		cout<<"Sending Reply"<<endl;
		sendMessageOrReply(msg_and_sender->senderAddress,msg_and_sender->senderPort,&m1);
		//make pieces 
		int total_range=0;
		int tot_increment=0;
		if(m1.Query_Type==SEARCH_CITATIONS_BY_PATENT_ID){
			total_range=citations_size;
			tot_increment=citation_inc;
		}
		else{
			total_range=inventors_size;
			tot_increment=inventor_inc;			
		}
		int iter_tot;
		struct job my_job;
		my_job.requester_id=m1.Client_ID;
		my_job.worker_id=0;
		//lock that message.
		pthread_mutex_lock(&mtx_jobs);//cout<<"locking jobs"<<endl;
		for (iter_tot=1; iter_tot<=(total_range-tot_increment);iter_tot+=tot_increment){
			m2=new message();
			m2->Magic=MAGIC;
			m2->Command=JOB;
			m2->Client_ID=0;
			m2->Next_Message = 0;
			m2->Query_Type = msg_and_sender->msg->Query_Type;
			strcpy(m2->Query, msg_and_sender->msg->Query);
			m2->Query_Length = strlen(m2->Query);
			m2->Data_Length = 0;
			strcpy(m2->Data, "");
			m2->Data_Range_Start=iter_tot;
			m2->Data_Range_End=iter_tot+tot_increment;
			my_job.msg=m2;
			my_jobs.push_back(my_job);
		}
		//add message from iter_tot to max value;
		m2=new message();
		m2->Magic=MAGIC;
		m2->Command=JOB;
		m2->Client_ID=0;
		m2->Next_Message = 0;
		m2->Query_Type = msg_and_sender->msg->Query_Type;
		strcpy(m2->Query, msg_and_sender->msg->Query);
		m2->Query_Length = strlen(m2->Query);
		m2->Data_Length = 0;
		strcpy(m2->Data, "");
		m2->Data_Range_Start=iter_tot;
		m2->Data_Range_End=total_range;
		my_job.msg=m2;
		my_jobs.push_back(my_job);
		pthread_mutex_unlock(&mtx_jobs);
		// add life.
		struct client_life my_new_client;
		my_new_client.client_id=m1.Client_ID;
    	my_new_client.life=10;
    	my_new_client.data_to_send="";
		pthread_mutex_lock(&mtx_clients);//cout<<"locking clients"<<endl;
		my_clients.push_back(my_new_client);
    	pthread_mutex_unlock(&mtx_clients);
	}
	else if(msg_and_sender->msg->Command==REQUEST_TO_JOIN){
		if (my_jobs.size()==0){return (void*)0;}//simply don't add a client if you don't have a job.
		//now assign this guy a job!
		pthread_mutex_lock(&mtx_jobs);//cout<<"locking jobs"<<endl;
		pthread_mutex_lock(&mtx_workers);//cout<<"locking workers"<<endl;
		global_client_id+=114;
		//add this guy to your system.
		addWorkerClient(global_client_id,msg_and_sender);
		giveWorkerSomeJob(global_client_id,msg_and_sender->senderAddress,msg_and_sender->senderPort);
		pthread_mutex_unlock(&mtx_workers);
		pthread_mutex_unlock(&mtx_jobs);
	}
	else if(msg_and_sender->msg->Command==ACK_JOB){
		pthread_mutex_lock(&mtx_workers);		
		incrementWorkerLife(msg_and_sender->msg->Client_ID);
		pthread_mutex_unlock(&mtx_workers);
	}//nothing, lets ignore it. Cos 
	else if(msg_and_sender->msg->Command==PING){
		pthread_mutex_lock(&mtx_jobs);//cout<<"locking jobs"<<endl;
		pthread_mutex_lock(&mtx_clients);//cout<<"locking clients"<<endl;
		pthread_mutex_lock(&mtx_workers);//cout<<"locking workers"<<endl;
		incrementClientLife(msg_and_sender->msg->Client_ID,msg_and_sender);		
		pthread_mutex_unlock(&mtx_workers);
		pthread_mutex_unlock(&mtx_clients);
		pthread_mutex_unlock(&mtx_jobs);
	}
	else if(msg_and_sender->msg->Command==NOT_DONE){
		pthread_mutex_lock(&mtx_jobs);//cout<<"locking jobs"<<endl;
		pthread_mutex_lock(&mtx_clients);//cout<<"locking clients"<<endl;
		pthread_mutex_lock(&mtx_workers);	
		incrementWorkerLife(msg_and_sender->msg->Client_ID);
		pthread_mutex_unlock(&mtx_workers);
		vector<job> my_jobs2;
		if(msg_and_sender->msg->Data_Length>0){
			//now add this to data to send.
			for (vector<job>::iterator jiter =my_jobs.begin(); jiter != my_jobs.end(); ++jiter){
				if(jiter->worker_id==msg_and_sender->msg->Client_ID){
					if(msg_and_sender->msg->Query_Type==1 || msg_and_sender->msg->Query_Type==0){

						for (vector<client_life>::iterator iter = my_clients.begin(); iter != my_clients.end(); ++iter){
							if(jiter->requester_id==iter->client_id){
								(iter->data_to_send).append(msg_and_sender->msg->Data);
							}
							}	
					}else{						
						//for 3;
							string my_new_data=msg_and_sender->msg->Data;
							unsigned int myqt=SEARCH_CITATIONS_BY_PATENT_ID;
							string new_query; size_t pos=0;
							struct message* new_msg;
							int iter_tot;
							struct job my_job_new;
							my_job_new.requester_id= jiter->requester_id;
							my_job_new.worker_id=0;
							while((pos=my_new_data.find("\n"))!=string::npos){
								new_query=my_new_data.substr(0,pos);
								my_new_data.erase(0,pos+1);
								if(new_query.length()==0) continue;
								//make a new struct and add a query!
								int total_range=citations_size;
								int tot_increment=citation_inc;
								for (iter_tot=1; iter_tot<=(total_range-tot_increment);iter_tot+=tot_increment){
									new_msg=new message();
									new_msg->Magic=MAGIC;
									new_msg->Command=JOB;
									new_msg->Client_ID=0;
									new_msg->Next_Message = 0;
									new_msg->Query_Type = myqt;
									strcpy(new_msg->Query, new_query.c_str());
									new_msg->Query_Length = strlen(new_msg->Query);
									new_msg->Data_Length = 0;
									strcpy(new_msg->Data, "");
									new_msg->Data_Range_Start=iter_tot;
									new_msg->Data_Range_End=iter_tot+tot_increment;
									my_job_new.msg=new_msg;
									my_jobs2.push_back(my_job_new);
								}
								//add message from iter_tot to max value;
								new_msg=new message();
								new_msg->Magic=MAGIC;
								new_msg->Command=JOB;
								new_msg->Client_ID=0;
								new_msg->Next_Message = 0;
								new_msg->Query_Type = myqt;
								strcpy(new_msg->Query, new_query.c_str());
								new_msg->Query_Length = strlen(new_msg->Query);
								new_msg->Data_Length = 0;
								strcpy(new_msg->Data, "");
								new_msg->Data_Range_Start=iter_tot;
								new_msg->Data_Range_End=total_range;
								my_job_new.msg=new_msg;
								my_jobs2.push_back(my_job_new);
							}
							//now parse this data
						}
				}
			}
			//now copy
			struct job my_msg;struct message* m3;
			for (vector<job>::iterator jiter =my_jobs2.begin(); jiter != my_jobs2.end();){
				m3=new message();

				m3->Magic=MAGIC;
				m3->Command=JOB;
				m3->Client_ID=0;
				m3->Next_Message = 0;
				m3->Query_Type = SEARCH_CITATIONS_BY_PATENT_ID;
				strcpy(m3->Query, jiter->msg->Query);
				m3->Query_Length = strlen(m3->Query);
				m3->Data_Length = 0;
				strcpy(m3->Data, "");
				m3->Data_Range_Start=jiter->msg->Data_Range_Start;
				m3->Data_Range_End=jiter->msg->Data_Range_End;

	    		my_msg.requester_id=jiter->requester_id;
	    		my_msg.worker_id=jiter->worker_id;
	    		my_msg.msg=m3;
	   			my_jobs.push_back(my_msg);
	   			free(jiter->msg);
	   			jiter=my_jobs2.erase(jiter);
			}
		}
		pthread_mutex_unlock(&mtx_clients);
		pthread_mutex_unlock(&mtx_jobs);
	}
	else if(msg_and_sender->msg->Command==DONE_NO_DATA_FOUND || msg_and_sender->msg->Command==DONE_DATA_FOUND){
		pthread_mutex_lock(&mtx_jobs);//cout<<"locking jobs"<<endl;
		pthread_mutex_lock(&mtx_clients);//cout<<"locking clients"<<endl;
		pthread_mutex_lock(&mtx_workers);
		vector<job> my_jobs2;
		incrementWorkerLife(msg_and_sender->msg->Client_ID);
		//now add this to data to send.
		for (vector<job>::iterator jiter =my_jobs.begin(); jiter != my_jobs.end();)
		{
			if(jiter->worker_id==msg_and_sender->msg->Client_ID){
				// for 1 and 2 
				if(msg_and_sender->msg->Query_Type==1 || msg_and_sender->msg->Query_Type==0){
					for (vector<client_life>::iterator iter = my_clients.begin(); iter != my_clients.end(); ++iter){
						if(jiter->requester_id==iter->client_id){
							if(msg_and_sender->msg->Data_Length>0){
								(iter->data_to_send).append(msg_and_sender->msg->Data);
							}
						}
					}
				}else{						
				//for 3;
					string my_new_data=msg_and_sender->msg->Data;
					unsigned int myqt=SEARCH_CITATIONS_BY_PATENT_ID;
					string new_query; size_t pos=0;
					struct message* new_msg;
					int iter_tot;
					struct job my_job_new;
					my_job_new.requester_id= jiter->requester_id;
					my_job_new.worker_id=0;
					while((pos=my_new_data.find("\n"))!=string::npos){
						new_query=my_new_data.substr(0,pos);
						my_new_data.erase(0,pos+1);
						if(new_query.length()==0) continue;
						//make a new struct and add a query!
						int total_range=citations_size;
						int tot_increment=citation_inc;
						for (iter_tot=1; iter_tot<=(total_range-tot_increment);iter_tot+=tot_increment){
							new_msg=new message();
							new_msg->Magic=MAGIC;
							new_msg->Command=JOB;
							new_msg->Client_ID=0;
							new_msg->Next_Message = 0;
							new_msg->Query_Type = myqt;
							strcpy(new_msg->Query, new_query.c_str());
							new_msg->Query_Length = strlen(new_msg->Query);
							new_msg->Data_Length = 0;
							strcpy(new_msg->Data, "");
							new_msg->Data_Range_Start=iter_tot;
							new_msg->Data_Range_End=iter_tot+tot_increment;
							my_job_new.msg=new_msg;
							my_jobs2.push_back(my_job_new);
						}
						//add message from iter_tot to max value;
						new_msg=new message();
						new_msg->Magic=MAGIC;
						new_msg->Command=JOB;
						new_msg->Client_ID=0;
						new_msg->Next_Message = 0;
						new_msg->Query_Type = myqt;
						strcpy(new_msg->Query, new_query.c_str());
						new_msg->Query_Length = strlen(new_msg->Query);
						new_msg->Data_Length = 0;
						strcpy(new_msg->Data, "");
						new_msg->Data_Range_Start=iter_tot;
						new_msg->Data_Range_End=total_range;
						my_job_new.msg=new_msg;
						my_jobs2.push_back(my_job_new);
					}
					//now parse this data
				}
				//now remove this job!
				free(jiter->msg);
				jiter=my_jobs.erase(jiter);
				//give new job to worker
			}
			else{
				++jiter;
			}
		}
		//now copy
			struct job my_msg;struct message* m3;
			for (vector<job>::iterator jiter =my_jobs2.begin(); jiter != my_jobs2.end();){
				m3=new message();

				m3->Magic=MAGIC;
				m3->Command=JOB;
				m3->Client_ID=0;
				m3->Next_Message = 0;
				m3->Query_Type = SEARCH_CITATIONS_BY_PATENT_ID;
				strcpy(m3->Query, jiter->msg->Query);
				m3->Query_Length = strlen(m3->Query);
				m3->Data_Length = 0;
				strcpy(m3->Data, "");
				m3->Data_Range_Start=jiter->msg->Data_Range_Start;
				m3->Data_Range_End=jiter->msg->Data_Range_End;

	    		my_msg.requester_id=jiter->requester_id;
	    		my_msg.worker_id=jiter->worker_id;
	    		my_msg.msg=m3;
	   			my_jobs.push_back(my_msg);
	   			free(jiter->msg);
				jiter=my_jobs2.erase(jiter);
			}
		giveWorkerSomeJob(msg_and_sender->msg->Client_ID,msg_and_sender->senderAddress,msg_and_sender->senderPort);
		pthread_mutex_unlock(&mtx_workers);
		pthread_mutex_unlock(&mtx_clients);
		pthread_mutex_unlock(&mtx_jobs);		
	}
	free(msg_and_sender->msg);
	free(msg_and_sender);
	return (void *)0;
}


int main(int argc, char *argv[])
{
	citation_inc=2*3400000;
	inventor_inc=2*900000;
	inventors_size=4301231;
	citations_size=16522440;
	/*
	ifstream fin;
	string lb;
	fin.open("Citations.txt");
	while(!fin.eof()){
		getline(fin,lb);
		citations_size+=1;
	}
	fin.close();
	cout<<citations_size<<endl;
	fin.open("Inventors.txt");
	while(!fin.eof()){
		getline(fin,lb);
		inventors_size+=1;
	}
	cout<<inventors_size<<endl;
	fin.close();
	*/
	global_client_id=1432;
	unsigned short int port_num;//server port number. our port number is auto generated
	unsigned short int local_port;
	unsigned int query_type;
	char* port_num_char;
	char* server_hostname;
	struct hostent* server_ent;
	struct sockaddr_in server_addr;
	struct sockaddr_in client_addr;
	socklen_t addr_len;
	struct message *m1;
	pthread_t thread1;
	pthread_t pinger_thread;
	pthread_t killer_thread;
	int numbBytesRecieved;
	char recieve_buffer[MAXBUFLEN];
	int server_sockfd;
	m1 = (struct message *)NULL;
	/*
     * check command line arguments
     */
    string file_name(argv[0]);
     if(argc != 2){
		perror("Arguments");
		printAndExit(file_name);
	}
	port_num_char=argv[1];
	port_num=(short)(atoi(port_num_char));
	if(port_num> 65535){
		perror("Port Number");
		printAndExit(file_name);
	}
	if(port_num==0){
		cout<<"Sorry! We can't bind to a random port. Please specify one."<<endl;
		printAndExit(file_name);
	}
	//creating socket
	if ((server_sockfd = socket(PF_INET, SOCK_DGRAM, 0)) == -1) {
		perror("socket");
		printAndExit(file_name);
	}

	//send msg to server and wait for response
	server_addr.sin_family=AF_INET;
	server_addr.sin_port=htons((short)port_num);
	server_addr.sin_addr.s_addr=htonl(INADDR_ANY);
	memset(&(server_addr.sin_zero), '\0', 8); // zero the rest of the struct

	//bind to a local port
	while(bind(server_sockfd,(struct sockaddr *)&server_addr, sizeof(struct sockaddr))==-1){
		port_num+=1;
		server_addr.sin_port=htons((short)port_num);
	}
	pthread_create(&pinger_thread,NULL,serverPingerThread,(void*)NULL);
	pthread_create(&killer_thread,NULL,serverKillerThread,(void*)NULL);

	//cout<<"The local IP address is: "<<inet_ntoa(server_addr.sin_addr)<<endl;
	cout<<"The local port bound for connection: "<<port_num<<endl;
	//request and connect the server
	struct message_and_sender* msg_and_sender=(struct message_and_sender*)NULL;
	addr_len = sizeof(struct sockaddr);
	while(true){
		memset(recieve_buffer,'\0', MAXBUFLEN);
		if ((numbBytesRecieved=recvfrom(server_sockfd, recieve_buffer, MAXMSGLEN , 0,(struct sockaddr *)&client_addr, &addr_len)) == -1) {
			perror("Could not recieve incoming message:");
		}
		recieve_buffer[numbBytesRecieved]='\0';
		msg_and_sender= new message_and_sender();
		msg_and_sender->msg=toMessage(recieve_buffer);
		msg_and_sender->senderAddress=inet_ntoa(client_addr.sin_addr);
		msg_and_sender->senderPort=ntohs(client_addr.sin_port);
		pthread_create(&thread1,NULL,processMessage,(void*)msg_and_sender);
		printCurrentJobs();
	//	printCurrentClients();
	//	printCurrentWorkers();
	}
	

	/* Deallocating memmory m1, m2 */
	free(m1);	
	close(server_sockfd);
	return 1;
}

