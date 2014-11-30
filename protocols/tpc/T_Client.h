#ifndef _T_Client_h
#define _T_Client_h 1

#include <stdio.h>
#include "types.h"
#include "T_Node.h"
#include "T_Request.h"
#include "T_Reply.h"
#include "T_Certificate.h"

//Added by Maysam Yabandeh
#include <vector>

extern void T_rtimeT_handler();

class T_Client: public T_Node {
public:
	T_Client(FILE *config_file, char *host_name, short port = 0);
	// Effects: Creates a new Client object using the information in
	// "config_file". The line of config assigned to
	// this client is the first one with the right host address (if
	// port==0) or the first with the right host address and port equal
	// to "port".

	virtual ~T_Client();
	// Effects: Deallocates all storage associated with this.

	bool send_request(T_Request *req, int size, bool ro);
	// Effects: Sends request m to the primary. Returns FALSE iff two
	// consecutive request were made without waiting for a reply between
	// them.

	T_Reply *recv_reply();
	// Effects: Blocks until it receives enough reply messages for
	// the previous request. returns a pointer to the reply. The caller is
	// responsible for deallocating the request and reply messages.

private:
	Request_id out_rid; // Identifier of the outstanding request
	T_Request *out_req; // Outstanding request

	T_Certificate<T_Reply> r_reps; // Certificate with application replies
	bool is_panicking;
	bool bail_out;

	//Added by Maysam Yabandeh
	bool  responders[100];
	void processReceivedMessage(T_Message* m, int sock);
};

#endif // _T_Client_h
