/* message.c */
#include<stdio.h> 		/* Input/Output */
#include <stdlib.h> 	/* General Utilities */
#include <string.h>
#include <pthread.h>
#include "message.h"

/* Global Variables */


void *print_message_function(void *ptr) {

    struct message *m = (struct message *) ptr;
    printf("-------------------------------------------------\n");
    printf("Magic No : %d\n", m->Magic);
    printf("Client_ID : %d\n", m->Client_ID);
    printf("Command : %d\n", m->Command);
	printf("Data_Range_Start : %d\n", m->Data_Range_Start);
	printf("Data_Range_End : %d\n", m->Data_Range_End);
	printf("Next message : %d\n", m->Next_Message);
	printf("Query_Type : %d\n", m->Query_Type);
	if (m->Query_Length > 0) {
        printf("Query (%d Bytes) : %s\n", m->Query_Length, m->Query);
    }
    if (m->Data_Length > 0) {
        printf("Data (%d Bytes) : %s\n", m->Data_Length, m->Data);
    }
    printf("-------------------------------------------------\n");
    return (void *)(0);
}