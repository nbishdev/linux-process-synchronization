#ifndef __HEADER__H__
#define __HEADER__H__

#include <sys/sem.h>

int up(int);									/* up(sem) */
int down(int);									/* down(sem) */

union semun
{
    int              val;						/* Value for SETVAL */
    struct semid_ds *buf;						/* Buffer for IPC_STAT, IPC_SET */
    unsigned short  *array;					/* Array for GETALL, SETALL */
};


struct message
{
	int file_no;								/* File No. */
	int pid;									/* Client's PID */
};

struct response
{
	char answer[81];
};

#endif
