#ifndef _R_Client_h
#define _R_Client_h 1

#include <pthread.h>

#include <stdio.h>
#include "types.h"
#include "R_Node.h"
#include "R_Request.h"

extern "C"
{
#include "TCP_network.h"
}

class R_Reply;
class R_Request;

class R_Client : public R_Node
{
	public:
		R_Client(FILE *config_file, char *host_name, short port=0);
		// Effects: Creates a new R_Client object using the information in
		// "config_file". The line of config assigned to
		// this client is the first one with the right host address (if
		// port==0) or the first with the right host address and port equal
		// to "port".

		virtual ~R_Client();
		// Effects: Deallocates all storage associated with this.

		bool send_request(R_Request *req, int size, bool ro);
		// Effects: Sends request m to the service. Returns FALSE iff two
		// consecutive request were made without waiting for a reply between
		// them.

		R_Reply *recv_reply();
		// Effects: Blocks until it receives enough reply messages for
		// the previous request. returns a pointer to the reply. The caller is
		// responsible for deallocating the request and reply messages.

		Request_id get_rid() const;
		// Effects: Returns the current outstanding request identifier. The request
		// identifier is updated to a new value when the previous message is
		// delivered to the user.

		int nb_received_requests;
		R_Node *node;
		int timedout;

		// Threads
		pthread_t receive_resp_thread_id;
		pthread_t replies_handler_thread;
		void replies_handler();

		int socket_to_my_primary;
		int socket_to_my_last;
		// Each node will connect to primary which has the id equal to
		// client id % n

	protected:
		Request_id out_rid; // Identifier of the outstanding request
		R_Request *out_req; // Outstanding request
	private:
};

inline Request_id R_Client::get_rid() const
{
	return out_rid;
}

#endif // _Client_h
