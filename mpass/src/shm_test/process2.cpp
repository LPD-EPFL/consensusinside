#include<iostream.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include<unistd.h>

#define SHMSZ    1024

int read(int *b)
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
	if ((shm = (int *)shmat(shmid, 0, 0)) == (int *) -1) {
		cerr<<"shmat";
		exit(1);
	}


	/*now read what the server put in the memory.
	 *     */
	int i;
	for (i=0; i<=1000; i++,shm++)
		cout<<*shm<<endl;
	cout<<"read is completed"<<endl;
	*b=1;
	cout<<"*b is "<<*b<<endl;
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
	 *     * Create the one bit  segment.
	 *         */
	if ((shmid = shmget(key, 1, IPC_CREAT | 0666)) < 0) {
		cerr<<"shmget";
		exit(1);
	}

	/*
	 *     * Now we attach the segment to our data space.
	 *         */
	if ((b = (int *)shmat(shmid, 0, 0)) == (int *) -1) {
		cerr<<"shmat";
		exit(1);

	}

	cout<<" *b is "<<*b<<endl;

	while(1)
	{
		if(*b==0)
		{
			read(b);
		}
		else
			wait();
	}
}
