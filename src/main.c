/* main.c */
#include<stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include <pthread.h>
#include "message.h"
#include <vector>

int main(int argc, char *argv[])
{
	int port_num;
	int i;	
	struct message *m1, *m2;
	pthread_t thread1, thread2;
	int iret1, iret2;
	
	m1 = (struct message *)NULL;
	m2 = (struct message *)NULL;
			
	/*
     * check command line arguments
     */
	if(argc < 2)
	{
		printf("usage: %s <port>\n",argv[0]);
		return 0;
	}
	
	port_num=atoi(argv[1]);
	
	m1 = (struct message *) malloc(sizeof(struct message));
	m2 = (struct message *) malloc(sizeof(struct message));

	
	m1->Magic = MAGIC;
	m1->Client_ID = 0;
	m1->Command = JOB;
	m1->Data_Range_Start = 10000;
	m1->Data_Range_End = 12000;
	m1->Next_Message = 0;
	m1->Query_Type = SEARCH_PATENTS_BY_INVENTOR;
	strcpy(m1->Query, "Wilson,Pryce");
	m1->Query_Length = strlen(m1->Query);
	m1->Data_Length = 0;
	strcpy(m1->Data, "");




	m2->Magic = MAGIC;
	m2->Client_ID = 0;
	m2->Command = JOB;
	m2->Data_Range_Start = 10000;
	m2->Data_Range_End = 12000;
	m2->Next_Message = 0;
	m2->Query_Type = SEARCH_PATENTS_BY_INVENTOR;
	strcpy(m2->Query, "Wilson,Pryce");
	m2->Query_Length = strlen(m2->Query);
	strcpy(m2->Data, "1234;56789");
	m2->Data_Length = strlen(m2->Data);
	

	iret1 = pthread_create( &thread1, NULL, print_message_function, (void*) m1);  
    iret2 = pthread_create( &thread2, NULL, print_message_function, (void*) m2);  
	
	pthread_join( thread1, NULL);  
	pthread_join( thread2, NULL);   
	printf("Thread 1 returns: %d\n",iret1);  
	printf("Thread 2 returns: %d\n",iret2);  
	
	/* Deallocating memmory m1, m2 */
	free(m1);
	free(m2);
	
	return 1;
}
