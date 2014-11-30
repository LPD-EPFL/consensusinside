#ifndef _TCP_network_h
#define _TCP_network_h 1

int createClientSocket(struct sockaddr_in addr);

int createNonBlockingServerSocket(int port_no);

int createServerSocket(int in_port);

int recv_all_blocking(int s, void *buf, size_t len, int flags);

int recv_all_non_blocking(int s, void *buf, size_t len, int flags);

int send_all(int s, void *buf, int *len);

void setnonblocking(int sock);

#endif
