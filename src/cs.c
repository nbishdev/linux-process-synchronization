#include <errno.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>
#include "header.h"


int main(int argc, char** argv)
{
	char *out;									/* Will be used by shmat to handle messages stored in OUT */
	char *dir;									/* Path to directory with files */
	double t;									/* Start time */
	float lambda;								/* Lambda for the exponential distribution */
	int in_empty, in_full;						/* Necessary semaphore IDs for controlling the access to IN */
	int out_full;								/* Necessary semaphore ID for controlling the access to OUT */
	int pid, pid1;								/* For the fork() function */
	int IN, OUT;								/* Shared memory segment IDs for IN and OUT */
	int nof;									/* Number of text files */
	int not;									/* Number of transactions between the processes */
	struct message *in;							/* Will be used by shmat to handle messages stored in IN */
	struct timeval t0;							/* For the gettimeofday() function */
	union semun arg;
	
	srand((long) time(NULL));					/* Feed for rand() */
	
	if (argc < 5)
	{
		printf("Usage: cs [directory] [number of files] [number of transactions] [lambda]\n");
		exit(EXIT_FAILURE);
	}
	else
	{
		char *pEnd;
		dir = argv[1];
		nof = strtol(argv[2],&pEnd,10);
		not = strtol(argv[3],&pEnd,10);
		lambda = strtod(argv[4],&pEnd);
	}
	if (nof <= 0 || not <= 0 || lambda <= 0)
	{
		printf("The numeric arguments must be positive numbers\n");
		exit(EXIT_FAILURE);
	}
	
	printf("\nExecuting this application with %d clones of each process, using %d files, with lambda = %.3f\n\n", not, nof, lambda);
	
	if ((IN = shmget(4321, sizeof(struct message), 0600 | IPC_CREAT)) < 0) {/* Creation of shared memory segment IN */
		perror("shmget");
		exit(EXIT_FAILURE);
	}
	printf("Created shared memory segment IN\n");
	
	if ((OUT = shmget(4322, 80*sizeof(char), 0600 | IPC_CREAT)) < 0) {		/* Creation of shared memory segment OUT */
		perror("shmget");
		exit(EXIT_FAILURE);
	}
	printf("Created shared memory segment OUT\n");
	
	if ((in_empty = semget(9874, 1, 0600 | IPC_CREAT)) < 0) {				/* One semaphore which shows whether IN is empty */
		perror("semget");
		exit(EXIT_FAILURE);
	}
	if ((in_full = semget(9875, 1, 0600 | IPC_CREAT)) < 0) {				/* One semaphore which shows whether IN is empty */
		perror("semget");
		exit(EXIT_FAILURE);
	}
	if ((out_full = semget(9876, 1, 0600 | IPC_CREAT)) < 0) {				/* One semaphore which shows whether IN is empty */
		perror("semget");
		exit(EXIT_FAILURE);
	}
	printf("Created three semaphores to control the processes\n");
	
	arg.val = 1;
	if (semctl(in_empty, 0, SETVAL, arg) < 0) {								/* Initialize in_empty to one, enough space to write on IN */
		perror("semctl");
		exit(EXIT_FAILURE);
	}
	arg.val = 0;
	if (semctl(in_full, 0, SETVAL, arg) < 0) {								/* Initialize in_empty to one, enough space to write on IN */
		perror("semctl");
		exit(EXIT_FAILURE);
	}
	arg.val = 1;
	if (semctl(out_full, 0, SETVAL, arg) < 0) {								/* Initialize in_empty to one, enough space to write on IN */
		perror("semctl");
		exit(EXIT_FAILURE);
	}
	printf("Initialized the IN_EMPTY semaphore to one, IN_FULL to zero, OUT_FULL to one\n");
	
	if ((in = shmat(IN, NULL, 0)) == (struct message *) -1) {				/* Pointer on which shared memory segment IN is attached */
		perror("shmat");
		exit(EXIT_FAILURE);
	}
	if ((out = shmat(OUT, NULL, 0)) == (char *) -1) {						/* Pointer on which shared memory segment OUT is attached */
		perror("shmat");
		exit(EXIT_FAILURE);
	}
	printf("Attached the two memory segments\n\n");
	
	gettimeofday(&t0, NULL);												/* Gets the start time */
	t = (double) t0.tv_sec * 1000.0 + (double) t0.tv_usec / 1000.0;			/* sec and usec to ms */
	
	
	pid = fork();
	
	
	if (pid == 0)
	{	/* C */
		while (not--)
		{
			pid1 = fork();
			
			if (pid1 == 0)
			{/* C' */
				char *logfilepath;					/* Path to logfile */
				double st, rt;						/* Send time, Receive time in ms */
				FILE *logfile;						/* File stream which will point to log.txt */
				int pid_sem;						/* Semaphore ID for communication between C' and S' */
				struct message sent;				/* Outgoing message from C' */
				struct response received;			/* Incoming message from S' */
				struct timeval t1, t2;				/* For the gettimeofday() function */
				
				logfilepath = malloc((strlen(dir) + 8 + 1) * sizeof(char));
				strcpy(logfilepath,dir);
				strcat(logfilepath,"/log.txt");
				
				sent.pid = getpid();										/* Saves the ID of the process that will send this message, in the message */
				sent.file_no = ((rand() % nof) + 1);						/* Saves the random text file ID, beween 1 and N, in the message */
				
				if ((pid_sem = semget(sent.pid, 1, 0600 | IPC_CREAT)) < 0) {/* Creation of the necessary semaphore for communication between C' and S' */
					perror("semget");
					exit(EXIT_FAILURE);
				}
				
				arg.val = 0;												/* For pid semaphore's initialization */
				if (semctl(pid_sem, 0, SETVAL, arg) < 0) {					/* Initialize pid to zero */
					perror("semctl");
					exit(EXIT_FAILURE);
				}
				
				if (down(in_empty) == -1) {									/* Process C' will be blocked if there is already one message in IN */
					perror("semop");
					exit(EXIT_FAILURE);
				}
				
				memcpy(in, &sent, sizeof(struct message));					/* Stores the message in IN */
				gettimeofday(&t1, NULL);									/* Gets the storage time */
				printf("C' stored the message in IN\n");
				
				if (up(in_full) == -1) {									/* Allow S to read from IN */
					perror("semop");
					exit(EXIT_FAILURE);
				}
				
				if (down(pid_sem) == -1) {									/* Process C' will be blocked until the answer message is stored in OUT */
					perror("semop");
					exit(EXIT_FAILURE);
				}
				
				memcpy(&received, out, sizeof(struct response));			/* Retrieves the answer message from OUT */
				gettimeofday(&t2, NULL);									/* Gets the answer time */
				printf("C' retrieved the answer message from OUT\n");
				
				if (up(out_full) == -1) {									/* Allow S' to write to OUT */
					perror("semop");
					exit(EXIT_FAILURE);
				}
				
				st = (double) t1.tv_sec * 1000.0 + (double) t1.tv_usec / 1000.0;		/* sec and usec to ms */
				st -= t;
				rt = (double) t2.tv_sec * 1000.0 + (double) t2.tv_usec / 1000.0;		/* sec and usec to ms */
				rt -= t;
				
				if ((logfile = fopen(logfilepath, "a")) == NULL) {			/* Opens the log file */
					perror("fopen");
					exit(EXIT_FAILURE);
				}
				
				fprintf(logfile, "String: %sSend time (ms): %g\nAnswer time (ms): %g\n\n", received.answer, st, rt);	/* Updates the log file */
				fclose(logfile);											/* Closes the log file */
				printf("Log file updated\n");
				
				if (semctl(pid_sem, 0, IPC_RMID, NULL) < 0) {				/* Deletes the pid semaphore */
					perror("semctl");
					exit(EXIT_FAILURE);
				}
				
				free(logfilepath);
				
				exit(EXIT_SUCCESS);											/* Termination of C' */
			}
			else if (pid1 == -1)
			{
				perror("fork");
				exit(EXIT_FAILURE);
			}
			else
			{
				double sleep_time = (double) log((double) rand() / (double) RAND_MAX) * (double) (-1.0 / (double) lambda) + 1.0; /* Expotential distributon */
				printf("Process C is going into sleep state for %g seconds\n\n", sleep_time);
				sleep(sleep_time);						/* Process C halts before the creation of it's new clone */
				
				while (waitpid(pid1, NULL, WNOHANG) == 0);
			}
		}
		
		exit(EXIT_SUCCESS);								/* Termination of C */
	}
	else if (pid == -1)
	{
		perror("fork");
		exit(EXIT_FAILURE);
	}
	
	
	/* S */
	while (not--)
	{
		int pid1;
		struct message received;			/* Incoming message from C' */
		
		if (down(in_full) == -1) {			/* Process S will be blocked if there is no message to read from IN */
			perror("semop");
			exit(EXIT_FAILURE);
		}
		
		memcpy(&received, in, sizeof(struct message));	/* Retrieves the message from IN */
		printf("S retrieved the message from IN\n");
		
		if (up(in_empty) == -1) {						/* S' gives other C' processes the permission to write to IN */
			perror("semop");
			exit(EXIT_FAILURE);
		}
		
		pid1 = fork();
		
		if (pid1 == 0)
		{/* S' */
			struct response sent;			/* Outgoing message from S' */
			FILE *f;						/* File stream which will point to the file that will be opened for reading */
			char *file_name;				/* File that will be opened for reading */
			char *file_path;				/* Path to the file that will be opened for reading */
			int pid_sem;					/* Semaphore ID for communication between C' and S' */
			int chars;						/* Number of characters needed to be allocated for file names/paths */
			
			chars = snprintf(NULL, 0, "/%d.txt", received.file_no) + 1;
			file_name = malloc(chars * sizeof(char));
			
			chars += snprintf(NULL, 0, "%s", dir);
			file_path = malloc(chars * sizeof(char));
			
			sprintf(file_name, "/%d.txt", received.file_no);			/* Transforms the text file's ID to a full ID.txt name */
			strcpy(file_path,dir);
			strcat(file_path,file_name);
			
			if ((f = fopen(file_path, "r")) == NULL) {					/* Opens the text file for reading */
				perror("fopen");
				exit(EXIT_FAILURE);
			}
			
			fgets(sent.answer, 80, f);									/* Reads from the text file */
			fclose(f);													/* Closes the text file */
			sent.answer[80] = '\0';
			
			if ((pid_sem = semget(received.pid, 1, 0600)) < 0) {		/* Reference to the pid sem */
				perror("semget");
				exit(EXIT_FAILURE);
			}
			
			if (down(out_full) == -1) {									/* Process S' will be blocked if there is already a message in OUT */
				perror("semop");
				exit(EXIT_FAILURE);
			}
			
			memcpy(out, &sent, sizeof(struct response));				/* Stores the answer message in OUT */
			printf("S' stored the answer message in OUT\n");
			
			if (up(pid_sem) == -1) {									/* S' gives C' the permission to continue */
				perror("semop");
				exit(EXIT_FAILURE);
			}
			
			free(file_path);
			free(file_name);
			
			exit(EXIT_SUCCESS);						/* Termination of S' */
		}
		else if (pid1 == -1) {
			perror("fork");
			exit(EXIT_FAILURE);
		}
		else
		{
			while (waitpid(pid1, NULL, WNOHANG) == 0);
		}
	}
	
	while (waitpid(pid, NULL, WNOHANG) == 0);
	
	shmdt(in);									/* Detaches the pointer on which shared memory segment IN is attached */
	shmdt(out);									/* Detaches the pointer on which shared memory segment OUT is attached */
	printf("\nDetached the two memory segments\n");
	
	if (shmctl(IN, IPC_RMID, NULL) < 0) {		/* Deletes the shared memory segment IN */
		perror("shmctl");
		exit(EXIT_FAILURE);
	}
	printf("Deleted shared memory segment IN\n");
	
	if (shmctl(OUT, IPC_RMID, NULL) < 0) {		/* Deletes the shared memory segment OUT */
		perror("shmctl");
		exit(EXIT_FAILURE);
	}
	printf("Deleted shared memory segment OUT\n");
	
	if (semctl(in_empty, 0, IPC_RMID, NULL) < 0) {							/* Deletes the in_empty semaphore */
		perror("semctl");
		exit(EXIT_FAILURE);
	}
	if (semctl(in_full, 0, IPC_RMID, NULL) < 0) {							/* Deletes the in_full semaphore */
		perror("semctl");
		exit(EXIT_FAILURE);
	}
	if (semctl(out_full, 0, IPC_RMID, NULL) < 0) {							/* Deletes the out_full semaphore */
		printf("HEREEE\n");
		perror("semctl");
		exit(EXIT_FAILURE);
	}
	printf("Deleted all three semaphores\n");
	
	exit(EXIT_SUCCESS);									/* Termination of S */
}
