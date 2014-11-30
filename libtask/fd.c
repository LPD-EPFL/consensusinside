#include "taskimpl.h"
#include <sys/poll.h>
#include <fcntl.h>

#include <stdio.h>

#include "../mpass/src/mpass/connection.h"

#ifdef USE_SHM
//Added by Maysam Yabandeh
inline void mpass::copy_msg_to_queue(mpass::CacheLine *item, void *data) {
	simple_msg *snd_msg = (simple_msg *)data;
	char* ptr = (char*) item;
	size_t* size = (size_t*)ptr;
	*size = snd_msg->size;
	ptr += sizeof(size_t);
	memcpy(ptr, snd_msg->data, snd_msg->size);
}
inline void mpass::copy_msg_from_queue(mpass::CacheLine *item, void *data) {
	simple_msg *rcv_msg = (simple_msg *)data;
	char* ptr = (char*) item;
	size_t* size = (size_t*)ptr;
	rcv_msg->size = *size;
	ptr += sizeof(size_t);
	memcpy(rcv_msg->data, ptr, rcv_msg->size);
}
#endif


enum
{
	MAXFD = 1024
};

static struct pollfd pollfd[MAXFD];
static Task *polltask[MAXFD];
static int npollfd;
static int startedfdtask;
static Tasklist sleeping;
static int sleepingcounted;
static uvlong nsec(void);

void
fdtask(void *v)
{
	int i, ms;
	Task *t;
	uvlong now;
	
	tasksystem();
	taskname("fdtask");
	for(;;){
		//fprintf(stderr,"pooling0\n");
		/* let everyone else run */
	   taskyield();
		//fprintf(stderr,"\n after yield %d\n", maysamYieldRet);
		//while(taskyield() > 0);
		/* we're the only one runnable - poll for i/o */
		errno = 0;
		taskstate("poll");
		//Added by Maysam Yabandeh
		//taskname("fdtask(%d)",npollfd);
		if((t=sleeping.head) == nil)
			ms = -1;
		else{
			/* sleep at most 5s */
			now = nsec();
			if(now >= t->alarmtime)
				ms = 0;
			else if(now+5*1000*1000*1000LL >= t->alarmtime)
				ms = (t->alarmtime - now)/1000000;
			else
				ms = 5000;
		}
		//Added by Maysam Yabandeh
		//if (ms == -1 && maysamYieldRet == 0) ms = 0;
		if (ms == -1) ms = 0;
		//fprintf(stderr,"pooling ms is %d npollfd is %d\n", ms, npollfd);
#ifndef USE_SHM
		if(poll(pollfd, npollfd, ms) < 0){
		//fprintf(stderr,"pooling error\n");
			if(errno == EINTR)
				continue;
			fprint(2, "poll: %s\n", strerror(errno));
			taskexitall(0);
		}

		//fprintf(stderr,"pooling2\n");
		/* wake up the guys who deserve it */
		for(i=0; i<npollfd; i++){
			while(i < npollfd && pollfd[i].revents){
		//fprintf(stderr,"pooling3\n");
				taskready(polltask[i]);
				--npollfd;
				pollfd[i] = pollfd[npollfd];
				polltask[i] = polltask[npollfd];
			}
		}
#else
		/* wake up the guys who deserve it */
		//extern mpass::MessageEndPoint msg_end_point;
		mpass::MessageEndPoint *end_point = &mpass::msg_end_point;
		for(i=0; i<npollfd; i++){
		   int &fd = pollfd[i].fd;
			bool read = pollfd[i].events & POLLIN;
			mpass::Connection *conn = &end_point->conn_array[fd];
			if ( (read && !conn->rcv_q->is_empty()) || (!read && !conn->snd_q->is_full()) )
			{
				taskready(polltask[i]);
				--npollfd;
				pollfd[i] = pollfd[npollfd];
				polltask[i] = polltask[npollfd];
			}
		}
#endif
		
		//fprintf(stderr,"pooling4\n");
		now = nsec();
		while((t=sleeping.head) && now >= t->alarmtime){
		//fprintf(stderr,"pooling5\n");
			deltask(&sleeping, t);
			if(!t->system && --sleepingcounted == 0)
				taskcount--;
			taskready(t);
		}
	}
}

uint
taskdelay(uint ms)
{
	uvlong when, now;
	Task *t;
	
	if(!startedfdtask){
		startedfdtask = 1;
		taskcreate(fdtask, 0, 32768);
	}

	now = nsec();
	when = now+(uvlong)ms*1000000;
	for(t=sleeping.head; t!=nil && t->alarmtime < when; t=t->next)
		;

	if(t){
		taskrunning->prev = t->prev;
		taskrunning->next = t;
	}else{
		taskrunning->prev = sleeping.tail;
		taskrunning->next = (Task*)nil;
	}
	
	t = taskrunning;
	t->alarmtime = when;
	if(t->prev)
		t->prev->next = t;
	else
		sleeping.head = t;
	if(t->next)
		t->next->prev = t;
	else
		sleeping.tail = t;

	if(!t->system && sleepingcounted++ == 0)
		taskcount++;
	taskswitch();

	return (nsec() - now)/1000000;
}

void
fdwait(int fd, int rw)
{
	int bits;

#ifdef maysam_dbg
	fprintf(stderr,"fdwait on fd=%d 1 startedfdtask=%d\n",fd, startedfdtask);
#endif
	if(!startedfdtask){
		startedfdtask = 1;
		taskcreate(fdtask, 0, 32768);
	}
#ifdef maysam_dbg
	fprintf(stderr,"fdwait on fd=%d 2\n",fd);
#endif

	if(npollfd >= MAXFD){
		fprint(2, "too many poll file descriptors\n");
		abort();
	}
	
#ifdef maysam_dbg
	fprintf(stderr,"fdwait on fd=%d 3\n",fd);
#endif
	taskstate("fdwait for %s", rw=='r' ? "read" : rw=='w' ? "write" : "error");
	bits = 0;
	switch(rw){
	case 'r':
		bits |= POLLIN;
		break;
	case 'w':
		bits |= POLLOUT;
		break;
	}

#ifdef maysam_dbg
	fprintf(stderr,"fdwait on fd=%d 4\n",fd);
#endif
	polltask[npollfd] = taskrunning;
	pollfd[npollfd].fd = fd;
	pollfd[npollfd].events = bits;
	pollfd[npollfd].revents = 0;
	npollfd++;
#ifdef maysam_dbg
	fprintf(stderr,"fdwait on fd=%d before taskswitch\n");
#endif
	taskswitch();
}

/* Like fdread but always calls fdwait before reading. */
int
fdread1(int fd, void *buf, int n)
{
	int m;
	
	do
		fdwait(fd, 'r');
	while((m = read(fd, buf, n)) < 0 && errno == EAGAIN);
	return m;
}

#ifndef USE_SHM
int
fdread(int fd, void *buf, int n)
{
	int m;
	
	while((m=read(fd, buf, n)) < 0 && errno == EAGAIN)
	{
	   //fprintf(stderr, "read failed on socket %d\n", fd);
		fdwait(fd, 'r');
	}
	return m;
}
#else
int
fdread(int fd, void *buf, int n)
{
	mpass::MessageEndPoint *end_point = &mpass::msg_end_point;
	mpass::Connection *conn = &end_point->conn_array[fd];
#ifdef maysam_dbg
	fprintf(stderr,"fdread on fd=%d : before is_empty\n", fd);
#endif
	while(conn->rcv_q->is_empty())
	{
#ifdef maysam_dbg
		fprintf(stderr,"fdread on fd=%d: before fdwait\n", fd);
#endif
		fdwait(fd, 'r');
	}
#ifdef maysam_dbg
	fprintf(stderr,"fdread on fd=%d: before dequeue\n", fd);
#endif

	mpass::simple_msg msg_buf(buf, n);
	conn->rcv_q->dequeue(mpass::copy_msg_from_queue, &msg_buf);
	int size = msg_buf.size;
#ifdef maysam_dbg
	fprintf(stderr,"fdread on fd=%d: after dequeue size is %d\n", fd, size);
#endif
	return size;
}
#endif

#ifndef USE_SHM
int
fdwrite(int fd, void *buf, int n)
{
	int m, tot;
	
	for(tot=0; tot<n; tot+=m){
		while((m=write(fd, (char*)buf+tot, n-tot)) < 0 && errno == EAGAIN)
			fdwait(fd, 'w');
		if(m < 0)
			return m;
		if(m == 0)
			break;
	}
	return tot;
}
#else
int
fdwrite(int fd, void *msg, int size)
{
#ifdef maysam_dbg
	fprintf(stderr,"fdwrite on fd=%d msg=%s size=%d: starting\n", fd, msg, size);
#endif
	void *buf = msg;//malloc(size);
#ifdef maysam_dbg
	fprintf(stderr,"fdwrite 1\n", fd, msg, size);
#endif
	memcpy(buf, msg, size);
#ifdef maysam_dbg
	fprintf(stderr,"fdwrite 2\n", fd, msg, size);
#endif
	mpass::Connection *conn = &mpass::msg_end_point.conn_array[fd];
#ifdef maysam_dbg
	fprintf(stderr,"fdwrite 3\n", fd, msg, size);
#endif
	mpass::simple_msg to_snd(buf, size);
#ifdef maysam_dbg
	fprintf(stderr,"fdwrite on fd=%d msg=%s size=%d: before fdwait\n", fd, msg, size);
#endif
	while(conn->snd_q->is_full())
		fdwait(fd, 'w');
#ifdef maysam_dbg
	fprintf(stderr,"fdwrite on fd=%d msg=%s size=%d: before enqueue\n", fd, msg, size);
#endif
	conn->snd_q->enqueue(mpass::copy_msg_to_queue, &to_snd);
	return size;
}
#endif

int
fdnoblock(int fd)
{
#ifndef USE_SHM
	return fcntl(fd, F_SETFL, fcntl(fd, F_GETFL)|O_NONBLOCK);
#else
   return 0;
#endif
}

static uvlong
nsec(void)
{
	struct timeval tv;

	if(gettimeofday(&tv, 0) < 0)
		return -1;
	return (uvlong)tv.tv_sec*1000*1000*1000 + tv.tv_usec*1000;
}

