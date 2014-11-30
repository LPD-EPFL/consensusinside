#ifndef _C_Client_h
#define _C_Client_h 1

#include <pthread.h>

#include <stdio.h>
#include "types.h"
#include "C_Node.h"
#include "C_Request.h"

extern "C"
{
#include "TCP_network.h"
}

class C_Reply;
class C_Request;

class C_Client : public C_Node
{
	public:
		C_Client(FILE *config_file, char *host_name, short port=0);
		// Effects: Creates a new C_Client object using the information in
		// "config_file". The line of config assigned to
		// this client is the first one with the right host address (if
		// port==0) or the first with the right host address and port equal
		// to "port".

		virtual ~C_Client();
		// Effects: Deallocates all storage associated with this.

		bool send_request(C_Request *req, int size, bool ro);
		// Effects: Sends request m to the service. Returns FALSE iff two
		// consecutive request were made without waiting for a reply between
		// them.

		C_Reply *recv_reply();
		// Effects: Blocks until it receives enough reply messages for
		// the previous request. returns a pointer to the reply. The caller is
		// responsible for deallocating the request and reply messages.

		Request_id get_rid() const;
		// Effects: Returns the current outstanding request identifier. The request
		// identifier is updated to a new value when the previous message is
		// delivered to the user.

		int nb_received_requests;
		C_Node *node;
		int timedout;

		// Threads
		pthread_t receive_resp_thread_id;
		pthread_t replies_handler_thread;
		void replies_handler();
		int socket_to_primary;
#ifndef REPLY_BY_PRIMARY
		int socket_to_last;
#endif

	protected:
		Request_id out_rid; // Identifier of the outstanding request
		C_Request *out_req; // Outstanding request
	private:
};

inline Request_id C_Client::get_rid() const
{
	return out_rid;
}

#endif // _Client_h
