#include<iostream.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include<unistd.h>

#define SHMSZ    1024

int write(int *b)
{
	char c;
	int shmid;
	key_t key;
	int *shm;
	char *s;

	/*
	 *     * We'll name our shared memory segment
	 *         * "5678".
	 *             */
	key = 5678;

	/*
	 *     * Create the segment.
	 *         */
	if ((shmid = shmget(key, SHMSZ, IPC_CREAT | 0666)) < 0) {
		cerr<<"shmget";
		exit(1);
	}

	/*
	 *     * Now we attach the segment to our data space.
	 *         */
	if ((shm = (int *)/*(char *)*/shmat(shmid, 0, 0)) == /*(char *)*/(int *) -1) {
		cerr<<"shmat";
		exit(1);
	}

	/*
	 *     * Now put some things into the memory for the
	 *         * other process to read.
	 *             */
	// s = shm;
	int i;
	for (i=0; i<=1000; i++,shm++)
		*shm = i;

	*b=0;
	cout<<"write is complited"<<endl;
	cout<<*b<<endl;
	return(*b);
}
main()
{

	char c;
	int shmid;
	key_t key;
	static int *b;

	key = 5679;

	/*
	 * Create the one bit  segment.
	 */
	if ((shmid = shmget(key, 1, IPC_CREAT | 0666)) < 0) {
		cerr<<"shmget";
		exit(1);
	}

	/*
	 * Now we attach the segment to our data space.
	 */
	if ((b = ( int *)shmat(shmid, 0, 0)) == (int *) -1) 
	{
		cerr<<"shmat";
		exit(1);

	}
	cout<<*b<<endl;
	while(1)
	{
		if(*b==1)
		{
			write(b);
		}
		else
			wait();
	}
}
