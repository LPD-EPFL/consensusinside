#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/param.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>

//#include "crypt.h"
//#include "rabin.h"

#include "th_assert.h"
#include "parameters.h"
#include "M_Message.h"
#include "M_Principal.h"
#include "M_Node.h"

#include "M_Time.h"


//#define TRACES
#define USE_AS_A_UTILITY_SERVICE

#include <task.h>
#include <errno.h>
// Pointer to global node instance.
M_Node *M_node = 0;
M_Principal *M_my_principal = 0;

M_Node::M_Node(FILE *config_file, FILE *config_priv, char *host_name, short req_port)
{
	M_node = this;

	init_clock_mhz();

	fprintf(stderr, "M_Node: clock_mhz is %llu\n", clock_mhz);

	// Read public configuration file:
	// read max_faulty and compute derived variables
	fscanf(config_file, "%d\n", &max_faulty);
	//Added by Maysam Yabandeh
	fscanf(config_file, "%d \n", &num_replicas);
	//num_replicas = 3 * max_faulty + 1;
	if (num_replicas > Max_num_replicas)
	{
		th_fail("Invalid number of replicas");
	}

	// read in all the principals
	char addr_buff[100];
	char pk[1024];
	short port;

	fscanf(config_file, "%d\n", &num_principals);
	if (num_replicas > num_principals)
	{
		th_fail("Invalid argument");
	}

	// read in group principal's address
	fscanf(config_file, "%256s %hd\n", addr_buff, &port);
	Addr a;
	bzero((char*)&a, sizeof(a));
	a.sin_family = AF_INET;
	a.sin_addr.s_addr = inet_addr(addr_buff);
	a.sin_port = htons(port);
	group = new M_Principal(num_principals+1, num_principals, a, NULL);

	// read in remaining principals' addresses and figure out my principal
	//char host_name[MAXHOSTNAMELEN+1];
	//if (gethostname(host_name, MAXHOSTNAMELEN))
	//{
	//	perror("Unable to get hostname");
	//	exit(1);
	//}

	struct hostent *hent = gethostbyname(host_name);
	if (hent == 0)
	{
		th_fail("Could not get hostent");
	}

	struct in_addr my_address = *((in_addr*)hent->h_addr_list[0]);
	node_id = -1;

	principals = (M_Principal**)malloc(num_principals*sizeof(M_Principal*));

#ifdef USE_AS_A_UTILITY_SERVICE
//Added by Maysam Yabandeh
//not to interfere with one acceptor that is using this as a service
if (req_port != 0)
req_port -= 100;
#endif

	fprintf(stderr, "num_principals = %d\n", num_principals);
	char temp_host_name[MAXHOSTNAMELEN+1];
	for (int i=0; i < num_principals; i++)
	{
		fscanf(config_file, "%256s %32s %hd %1024s \n", temp_host_name,
				addr_buff, &port, pk);

#ifdef USE_AS_A_UTILITY_SERVICE
//Added by Maysam Yabandeh
//not to interfere with one acceptor that is using this as a service
port -= 100;
#endif
		a.sin_addr.s_addr = inet_addr(addr_buff);
		a.sin_port = htons(port);

		principals[i] = new M_Principal(i, num_principals, a, addr_buff, port, pk);
		//principals[i] = new M_Principal(i, num_principals, a, pk);
		//fprintf(stderr, "We have parsed entry %s\n", temp_host_name);

		//Added by Maysam Yabandeh - use this when port is unique and there are multiple NIC
		//if (node_id == -1
		if (my_address.s_addr == a.sin_addr.s_addr && node_id == -1
				&& (req_port == 0 || req_port == port))
		{
			node_id = i;
			M_my_principal = principals[i];
			fprintf(stderr, "We have parsed entry %s and assigned node_id = %d\n", temp_host_name, node_id);
		}
	}

	if (node_id < 0)
	{
		th_fail("Could not find my principal");
	}

	//if (config_priv != NULL) {
	    //// Read private configuration file:
	    //char pk1[1024], pk2[1024];
	    //fscanf(config_priv, "%s %s\n", pk1, pk2);
	    //bigint n1(pk1, 16);
	    //bigint n2(pk2, 16);
	    //if (n1 >= n2)
		//th_fail("Invalid private file: first number >= second number");
//
	    //priv_key = new rabin_priv(n1, n2);
	//} else {
	    //fprintf(stderr, "M_Node: Will not init priv_key\n");
	    //priv_key = NULL;
	//}

	// Initialize memory allocator for messages.
//	M_Message::init();

	// Initialize socket.
	//sock = socket(AF_INET, SOCK_DGRAM, 0);

	// name the socket
	Addr tmp;
	tmp.sin_family = AF_INET;
	tmp.sin_addr.s_addr = htonl(INADDR_ANY);
	tmp.sin_port = principals[node_id]->address()->sin_port;
	//int error = bind(sock, (struct sockaddr*)&tmp, sizeof(Addr));
	//int sock = netannounce(UDP, 0, principals[node_id]->address()->sin_port); 
	//if (error < 0)
	//{
		//perror("Unable to name socket");
		//exit(1);
	//}
	//fdnoblock(sock);

	/*
	// Set TTL larger than 1 to enable multicast across routers.
	u_char i = 20;
	error = setsockopt(sock, IPPROTO_IP, IP_MULTICAST_TTL, (char *)&i,
			sizeof(i));
	if (error < 0)
	{
		perror("unable to change TTL value");
		exit(1);
	}

	error = fcntl(sock, F_SETFL, M_NONBLOCK);
	if (error < 0)
	{
		perror("unable to set socket to asynchronous mode");
		exit(1);
	}
	*/

	// Sleep for more than a second to ensure strictly increasing
	// timestamps.
	sleep(2);

	// Compute new timestamp for cur_rid
	new_tstamp();
}

M_Node::~M_Node()
{
	for (int i=0; i < num_principals; i++)
	{
		delete principals[i];
	}

	free(principals);
	delete group;
}

void M_Node::justDial(int i)
{
	th_assert(i == M_All_replicas || (i >= 0&& i < num_principals),
			"Invalid argument");

	if (i == M_All_replicas)
	{
		//send it to all replicas except me
		for (int x=0; x<num_replicas; x++)
			justDial(x);
		return;
	}

   int &remotefd = principals[i]->remotefd;
	//Added by Maysam Yabandeh
	//address the problem of concurrect call to netdial
	while (remotefd == -2) taskyield();
	if (remotefd < 0)
	{
	   remotefd = -2;
		if((remotefd = netdial(TCP, principals[i]->ip, principals[i]->port)) < 0){
			fprintf(stderr, "Error: netdial failed on calling %d", i);
			remotefd = -1;
			close(remotefd);
			return;
		}
#ifdef TRACES
		fprintf(stderr, "netdial socket %d\n", remotefd);
#endif
		fdnoblock(remotefd);
		taskcreate(M_recvtask, (void*)(remotefd), STACK);
	}
}

void M_Node::send(M_Message *m, int i)
{
	th_assert(i == M_All_replicas || (i >= 0&& i < num_principals),
			"Invalid argument");

	if (i == M_All_replicas)
	{
		//send it to all replicas except me
		for (int x=0; x<num_replicas; x++)
			//if (x != id())
				send(m,x);
		return;
	}

   //fprintf(stderr, "send a msg to %d\n", i);

   int &remotefd = principals[i]->remotefd;
	//Added by Maysam Yabandeh
	//address the problem of concurrect call to netdial
	while (remotefd == -2) taskyield();
	if (remotefd < 0)
	{
	   remotefd = -2;
		if((remotefd = netdial(TCP, principals[i]->ip, principals[i]->port)) < 0){
			fprintf(stderr, "Error: netdial failed on calling %d", i);
			remotefd = -1;
			close(remotefd);
			return;
		}
#ifdef TRACES
		fprintf(stderr, "netdial socket %d\n", remotefd);
#endif
		fdnoblock(remotefd);
		taskcreate(M_recvtask, (void*)(remotefd), STACK);
		//taskyield();
	}

	int error = 0;
	while (error < (int)m->size())
	{
		//error = sendto(sock, m->contents(), m->size(), 0, (struct sockaddr*)to,	sizeof(Addr));
		error = fdwrite(remotefd, m->contents(), m->size());
#ifdef TRACES
		fprintf(stderr, "=%d fdwrite on %d error is %d\n", id(), remotefd, error);
#endif
		//int ret = fdread(remotefd, m->contents(), m->msize());
		//fprintf(stderr, "%d ======================================== %d \n", id(), ret);
	}
}

void M_recvtask(void* v)
{
	long tmp = (long) v;
	int remotefd = tmp;
   //int remotefd = (int) v;
	//taskname("M_recvtask(%d)",remotefd);
//fprintf(stderr,"=M_recvtask on socket %d\n", remotefd);
	while (1)
	{
#ifdef TRACES
		fprintf(stderr, "M_Node::M_recvtask(): reading the socket fd=%d\n", remotefd);
#endif
		M_Message* m= new M_Message(M_Max_message_size);
		//int ret = recvfrom(M_node->sock, m->contents(), m->msize(), 0, 0, 0);
		int ret = fdread(remotefd, m->contents(), m->msize());
		if (ret >= (int)sizeof(M_Message_rep) && ret >= (int)m->size())
		{
#ifdef TRACES
			fprintf(stderr, "M_Node::M_recvtask(): one message received on the socket fd=%d\n", remotefd);
#endif
			//M_ITimer::handle_timeouts();
			M_node->processReceivedMessage(m, remotefd);
#ifdef TRACES
			fprintf(stderr, "M_Node::M_recvtask(): processing message is finished fd=%d\n", remotefd);
#endif
		}
		else
		{
#ifdef TRACES
			fprintf(stderr, "Error at M_Node::M_recvtask(): fdread id=%d ret=%d\n", M_node->id(), ret);
#endif
		delete m;
		}
	}
}

void M_listentask(void* v)
{
   int cfd,fd;
	long tmp = (long)v;
	int rport;
	int lport=tmp;
	//taskname("M_listentask(%d)",lport);
	char remote[16];
	if((fd = netannounce(TCP, 0, lport)) < 0){
		fprintf(stderr, "cannot announce on tcp port %d: %s\n", lport, strerror(errno));
		taskexitall(1);
	}
	fprintf(stderr, "listening on tcp port %d\n", lport);
	fdnoblock(fd);
	while((cfd = netaccept(fd, remote, &rport)) >= 0){
		//fprintf(stderr, "connection from %s:%d in socket %d\n", remote, rport, cfd);
		fdnoblock(cfd);
		taskcreate(M_recvtask, (void*)cfd, STACK);
	}
}

void M_Node::serverListen()
{
	int lport = principals[id()]->port;
	taskcreate(M_listentask, (void*)(lport), STACK);
}

bool M_Node::has_messages(long to)
{
th_assert(0, "Why I am here? M_Node::has_messages");
	struct timeval timeout;
	timeout.tv_sec = 0;
	timeout.tv_usec = to;
	fd_set fdset;
	FD_ZERO(&fdset);
	FD_SET(sock, &fdset);
	int ret = select(sock+1, &fdset, 0, 0, &timeout);
	if (ret > 0&& FD_ISSET(sock, &fdset))
	{
		return true;
	}
	return false;
}

M_Message* M_Node::recv()
{
th_assert(0, "Why I am here? M_Node::recv");
fprintf(stderr,"=%d M_Node::recv\n", id());
	M_Message* m= new M_Message(M_Max_message_size);
	while (1)
	{
#ifdef TRACES
		fprintf(stderr, "=%d M_Node::recv(): reading the socket\n", id());
#endif
		int ret = recvfrom(sock, m->contents(), m->msize(), 0, 0, 0);
		if (ret >= (int)sizeof(M_Message_rep) && ret >= (int)m->size())
		{
#ifdef TRACES
			fprintf(stderr, "=%d M_Node::recv(): one message received on the socket\n", id());
#endif
			return m;
		}
		while (!has_messages(20000))
			;
	}
}



Request_id M_Node::new_rid()
{
	if ((unsigned)cur_rid == (unsigned)0xffffffff)
	{
		new_tstamp();
	}
	return ++cur_rid;
}

void M_Node::new_tstamp()
{
	struct timeval t;
	gettimeofday(&t, 0);
	//th_assert(sizeof(t.tv_sec) <= sizeof(int), "tv_sec is too big");
	//Added by Maysam Yabandeh - problem with 64-bit arch
	//sound no big deal, it just dicardt the upper part TODO check it
	Long tstamp = t.tv_sec;
	long int_bits = sizeof(int)*8;
	cur_rid = tstamp << int_bits;
}
