//Added by Maysam Yabandeh
//Include codes for mutual execution of operations among multiple processes

#ifndef _MY_SEM_H_
#define _MY_SEM_H_ 1


#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <stdio.h>

//#define maysam_dbg

//called by the server
void sem_init(int KEY)
{
	int id;
	union semun {
		int val;
		struct semid_ds *buf;
		ushort * array;
	} argument;


	//Added by Maysam Yabandeh - the initial value
	argument.val = 1;

	/* Create the semaphore with external key KEY if it doesn't already 
	 *       exists. Give permissions to the world. */

	id = semget(KEY, 1, 0666 | IPC_CREAT);

	/* Always check system returns. */

	if(id < 0)
	{
		fprintf(stderr, "Unable to obtain semaphore.\n");
		exit(0);
	}

	/* What we actually get is an array of semaphores. The second 
	 *       argument to semget() was the array dimension - in our case
	 *             1. */

	/* Set the value of the number 0 semaphore in semaphore array
	 *       # id to the value 0. */

	if( semctl(id, 0, SETVAL, argument) < 0)
	{
		fprintf( stderr, "Cannot set semaphore value.\n");
	}
	else
	{
		fprintf(stderr, "Semaphore %d initialized.\n", KEY);
	}
}

//----------------------------------------
void sem_wait(int KEY)//key is the listen port (unique id for the listener
{
	int id;  /* Internal identifier of the semaphore. */
	struct sembuf operations[1];
	/* An "array" of one operation to perform on the semaphore. */

	int retval; /* Return value from semop() */

	/* Get the index for the semaphore with external name KEY. */
	id = semget(KEY, 1, 0666);
	if(id < 0)
		/* Semaphore does not exist. */
	{
		fprintf(stderr, "Program wait sem cannot find semaphore %d, exiting.\n", KEY);
		exit(0);
	}

	/* Do a semaphore V-operation. */
#ifdef maysam_dbg
	printf("Program wait sem about to do a V-operation. \n");
#endif

	/* Set up the sembuf structure. */
	/* Which semaphore in the semaphore array : */
	operations[0].sem_num = 0;
	/* Which operation? Subtract 1 from semaphore value, wait if value is not positive : */
	operations[0].sem_op = -1;
	/* Set the flag so we will wait : */   
	operations[0].sem_flg = 0;

	/* So do the operation! */
	retval = semop(id, operations, 1);

	if(retval == 0)
	{
#ifdef maysam_dbg
		printf("Successful V-operation by program wait sem.\n");
#endif
	}
	else
	{
		printf("wait sem: V-operation did not succeed.\n");
		perror("REASON");
	}
}


//------------------------------------------------

void sem_release(int KEY)
{
	int id;  /* Internal identifier of the semaphore. */
	struct sembuf operations[1];
	/* An "array" of one operation to perform on the semaphore. */

	int retval; /* Return value from semop() */

	/* Get the index for the semaphore with external name KEY. */
	id = semget(KEY, 1, 0666);
	if(id < 0)
		/* Semaphore does not exist. */
	{
		fprintf(stderr, "Program release sem cannot find semaphore, exiting.\n");
		exit(0);
	}

	/* Do a semaphore P-operation. */
#ifdef maysam_dbg
	printf("Program release sem about to do a P-operation. \n");
	printf("Process id is %d\n", getpid());
#endif

	/* Set up the sembuf structure. */
	/* Which semaphore in the semaphore array : */
	operations[0].sem_num = 0;
	/* Which operation? Add 1 to semaphore value : */
	operations[0].sem_op = 1;
	/* Set the flag so we will wait : */   
	operations[0].sem_flg = 0;

	/* So do the operation! */
	retval = semop(id, operations, 1);

	if(retval == 0)
	{
		printf("Successful P-operation by program release sem.\n");
		printf("Process id is %d\n", getpid());
	}
	else
	{
#ifdef maysam_dbg
		printf("release sem: P-operation did not succeed.\n");
#endif
	}
}

#endif
