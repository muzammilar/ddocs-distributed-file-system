#ifndef MESSAGE_H
#define MESSAGE_H
/* message.h */

/* Messaging protocol constants and formats */

/* Command definition */

#define REQUEST_TO_JOIN		0
#define QUERY				1
#define JOB 				2
#define ACK_JOB		 		3
#define PING 				4
#define NOT_DONE			5
#define DONE_NO_DATA_FOUND	6
#define DONE_DATA_FOUND 	7
#define CANCEL_JOB			8

/* Query Types */

#define SEARCH_PATENTS_BY_INVENTOR		0
#define SEARCH_CITATIONS_BY_PATENT_ID	1
#define SEARCH_CITATIONS_BY_INVENTOR	2

/* Constants */
#define MAGIC               14582

#define MAX_QLEN			128    /*MAX Query length*/
#define MAX_DLEN			1884    /*MAX Query length*/


struct	message {				/* message format of Password cracker protocol	*/
	unsigned int	Magic;
	unsigned int	Client_ID;	
	unsigned int	Command;
	unsigned int	Data_Range_Start;
	unsigned int	Data_Range_End;
	unsigned int	Next_Message;
	unsigned int	Query_Type;
	unsigned int	Query_Length;
	unsigned int	Data_Length;
	char	Query[MAX_QLEN];	/* Query*/
	char	Data[MAX_DLEN];	/* Data*/

};
extern void *print_message_function( void *ptr );

#endif
