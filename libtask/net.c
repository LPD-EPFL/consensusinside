#include "taskimpl.h"
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <sys/poll.h>

#include "../mpass/src/mpass/connection.h"
#ifdef USE_SHM
#include <unistd.h>
#include "my_sem.h"
//Added by Maysam Yabandeh
//just to have a unique id per process
int pid() {return getpid();}
#endif

#ifdef USE_SHM
//Added by Maysam Yabandeh
//the last port address used. This is to provide unique port addresses
mpass::MessageEndPoint mpass::msg_end_point;
#endif

#ifndef USE_SHM
int
netannounce(int istcp, char *server, int port)
{
	int fd, n, proto;
	struct sockaddr_in sa;
	socklen_t sn;
	uint32_t ip;

	taskstate("netannounce");
	proto = istcp ? SOCK_STREAM : SOCK_DGRAM;
	memset(&sa, 0, sizeof sa);
	sa.sin_family = AF_INET;
	if(server != nil && strcmp(server, "*") != 0){
		if(netlookup(server, &ip) < 0){
			taskstate("netlookup failed");
			return -1;
		}
		memmove(&sa.sin_addr, &ip, 4);
	}
	sa.sin_port = htons(port);
	if((fd = socket(AF_INET, proto, 0)) < 0){
		taskstate("socket failed");
		return -1;
	}
	
	/* set reuse flag for tcp */
	if(istcp && getsockopt(fd, SOL_SOCKET, SO_TYPE, (void*)&n, &sn) >= 0){
		n = 1;
		setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, (char*)&n, sizeof n);
	}

	if(bind(fd, (struct sockaddr*)&sa, sizeof sa) < 0){
		taskstate("bind failed");
		close(fd);
		return -1;
	}

	if(proto == SOCK_STREAM)
		listen(fd, 16);

	fdnoblock(fd);
	taskstate("netannounce succeeded");
	return fd;
}
#else
int netannounce(int istcp, char *server, int port)
{
   //create queue i_listen
	int shmfd;
	mpass::MessageQueue *listen_q = new(shmfd, port) mpass::MessageQueue();
	sem_init(port);
	//extern mpass::MessageEndPoint msg_end_point;
	mpass::MessageEndPoint *mepnt = &mpass::msg_end_point;
	int i = mepnt->conn_array.get_size();
	mepnt->conn_array.append();
	mepnt->conn_array[i].snd_q = NULL;
	mepnt->conn_array[i].rcv_q = listen_q;
	return i;
}
#endif

#ifndef USE_SHM
int
netaccept(int fd, char *server, int *port)
{
	int cfd, one;
	struct sockaddr_in sa;
	uchar *ip;
	socklen_t len;
	
	fdwait(fd, 'r');

	taskstate("netaccept");
	len = sizeof sa;
	if((cfd = accept(fd, (void*)&sa, &len)) < 0){
		taskstate("accept failed");
		return -1;
	}
	if(server){
		ip = (uchar*)&sa.sin_addr;
		snprint(server, 16, "%d.%d.%d.%d", ip[0], ip[1], ip[2], ip[3]);
	}
	if(port)
		*port = ntohs(sa.sin_port);
	fdnoblock(cfd);
	one = 1;
	setsockopt(cfd, IPPROTO_TCP, TCP_NODELAY, (char*)&one, sizeof one);
	taskstate("netaccept succeeded");
	return cfd;
}
#else
int
netaccept(int fd, char *server, int *port)
{
#ifdef maysam_dbg
   fprintf(stderr, "netaccept starting\n");
#endif
	taskstate("netaccept");
	char rcv_buf[512];
	size_t rcv_size = 0;//the value does not matter
#ifdef maysam_dbg
   fprintf(stderr, "netaccept before calling fdread\n");
#endif
	rcv_size = fdread(fd, rcv_buf, rcv_size);
#ifdef maysam_dbg
	fprintf(stderr, "netaccept after calling fdread\n");
#endif
	int myport;
	sscanf(rcv_buf, "%d %d\n", &myport, port);
#ifdef maysam_dbg
   fprintf(stderr, "netaccept fdread %d %d size=%d\n", myport, *port, rcv_size);
#endif
	//send just a hi, the content is not important
	rcv_size = 0;//strlen(rcv_buf);
#ifdef maysam_dbg
   fprintf(stderr, "netaccept before calling fdwrite\n");
#endif
	int shmfd; //just a unique id in the OS, filled by any of the following calls
	mpass::MessageQueue *rcv_q = (mpass::MessageQueue *) mpass::MessageQueue::createShmObj(sizeof(mpass::MessageQueue), shmfd, *port, myport);
	mpass::MessageQueue *snd_q = (mpass::MessageQueue *) mpass::MessageQueue::createShmObj(sizeof(mpass::MessageQueue), shmfd, myport, *port);
	mpass::MessageEndPoint *mepnt = &mpass::msg_end_point;
	int i = mepnt->conn_array.get_size();
	mepnt->conn_array.append();
	mepnt->conn_array[i].snd_q = snd_q;
	mepnt->conn_array[i].rcv_q = rcv_q;
	int datafd = i;
	//extern mpass::MessageEndPoint msg_end_point;
	fdwrite(datafd, rcv_buf, rcv_size);
	server = NULL;
	taskstate("netaccept succeeded");
	return datafd;
}
#endif

#define CLASS(p) ((*(unsigned char*)(p))>>6)
static int
parseip(char *name, uint32_t *ip)
{
	unsigned char addr[4];
	char *p;
	int i, x;

	p = name;
	for(i=0; i<4 && *p; i++){
		x = strtoul(p, &p, 0);
		if(x < 0 || x >= 256)
			return -1;
		if(*p != '.' && *p != 0)
			return -1;
		if(*p == '.')
			p++;
		addr[i] = x;
	}

	switch(CLASS(addr)){
	case 0:
	case 1:
		if(i == 3){
			addr[3] = addr[2];
			addr[2] = addr[1];
			addr[1] = 0;
		}else if(i == 2){
			addr[3] = addr[1];
			addr[2] = 0;
			addr[1] = 0;
		}else if(i != 4)
			return -1;
		break;
	case 2:
		if(i == 3){
			addr[3] = addr[2];
			addr[2] = 0;
		}else if(i != 4)
			return -1;
		break;
	}
	*ip = *(uint32_t*)addr;
	return 0;
}

int
netlookup(char *name, uint32_t *ip)
{
	struct hostent *he;

	if(parseip(name, ip) >= 0)
		return 0;
	
	/* BUG - Name resolution blocks.  Need a non-blocking DNS. */
	taskstate("netlookup");
	if((he = gethostbyname(name)) != 0){
		*ip = *(uint32_t*)he->h_addr;
		taskstate("netlookup succeeded");
		return 0;
	}
	
	taskstate("netlookup failed");
	return -1;
}

#ifndef USE_SHM
int
netdial(int istcp, char *server, int port)
{
	int proto, fd, n;
	uint32_t ip;
	struct sockaddr_in sa;
	socklen_t sn;
	
	if(netlookup(server, &ip) < 0)
		return -1;

	taskstate("netdial");
	proto = istcp ? SOCK_STREAM : SOCK_DGRAM;
	if((fd = socket(AF_INET, proto, 0)) < 0){
		taskstate("socket failed");
		return -1;
	}
	fdnoblock(fd);

	/* for udp */
	if(!istcp){
		n = 1;
		setsockopt(fd, SOL_SOCKET, SO_BROADCAST, &n, sizeof n);
	}
	
	/* start connecting */
	memset(&sa, 0, sizeof sa);
	memmove(&sa.sin_addr, &ip, 4);
	sa.sin_family = AF_INET;
	sa.sin_port = htons(port);
	if(connect(fd, (struct sockaddr*)&sa, sizeof sa) < 0 && errno != EINPROGRESS){
		taskstate("connect failed");
		close(fd);
		return -1;
	}

	/* wait for finish */	
	fdwait(fd, 'w');
	sn = sizeof sa;
	if(getpeername(fd, (struct sockaddr*)&sa, &sn) >= 0){
		taskstate("connect succeeded");
		return fd;
	}
	
	/* report error */
	sn = sizeof n;
	getsockopt(fd, SOL_SOCKET, SO_ERROR, (void*)&n, &sn);
	if(n == 0)
		n = ECONNREFUSED;
	close(fd);
	taskstate("connect failed");
	errno = n;
	return -1;
}
#else
int
netdial(int istcp, char *server, int port)
{
#ifdef maysam_dbg
	fprintf(stderr, "at netdial\n");
#endif
   //create queue j->i and i->j
   //map queue i_listen
	int shmfd; //just a unique id in the OS, filled by any of the following calls
	mpass::MessageQueue *listen_q = (mpass::MessageQueue *) mpass::MessageQueue::createShmObj(sizeof(mpass::MessageQueue), shmfd, port);
	mpass::MessageEndPoint *mepnt = &mpass::msg_end_point;
	int i = mepnt->conn_array.get_size();
	mepnt->conn_array.append();
	mepnt->conn_array[i].snd_q = listen_q;
	mepnt->conn_array[i].rcv_q = NULL;
	int listen_fd = i;

	int myport = pid();
	mpass::MessageQueue *rcv_q = new(shmfd, port, myport) mpass::MessageQueue();
	mpass::MessageQueue *snd_q = new(shmfd, myport, port) mpass::MessageQueue();
	//extern mpass::MessageEndPoint msg_end_point;
	i = mepnt->conn_array.get_size();
	mepnt->conn_array.append();
	mepnt->conn_array[i].snd_q = snd_q;
	mepnt->conn_array[i].rcv_q = rcv_q;
	//send hi to j_listen
	int recv_fd = i;
	char rcv_buf[512];
	size_t rcv_size;
	sprintf(rcv_buf, "%d %d\n", port, myport);
	rcv_size = strlen(rcv_buf);
#ifdef maysam_dbg
	fprintf(stderr, "before calling fdwrite at netdial\n");
#endif
   sem_wait(port);
	fdwrite(listen_fd, rcv_buf, rcv_size);
	sem_release(port);
#ifdef maysam_dbg
	fprintf(stderr, "before calling fdread at netdial\n");
#endif
	fdread(recv_fd, rcv_buf, rcv_size);
	//whatever is the message, it means that the peer is ready
	return recv_fd;
}
#endif


