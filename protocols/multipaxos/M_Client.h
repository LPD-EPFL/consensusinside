#ifndef _M_Client_h
#define _M_Client_h 1

#include <stdio.h>
#include "types.h"
#include "M_Node.h"
#include "M_Request.h"
#include "M_Reply.h"
#include "M_Certificate.h"

//Added by Maysam Yabandeh
#include <vector>

extern void M_rtimeT_handler();

class M_Client: public M_Node {
public:
	M_Client(FILE *config_file, char *host_name, short port = 0);
	// Effects: Creates a new Client object using the information in
	// "config_file". The line of config assigned to
	// this client is the first one with the right host address (if
	// port==0) or the first with the right host address and port equal
	// to "port".

	virtual ~M_Client();
	// Effects: Deallocates all storage associated with this.

	bool send_request(M_Request *req, int size, bool ro);
	// Effects: Sends request m to the primary. Returns FALSE iff two
	// consecutive request were made without waiting for a reply between
	// them.

	M_Reply *recv_reply();
	// Effects: Blocks until it receives enough reply messages for
	// the previous request. returns a pointer to the reply. The caller is
	// responsible for deallocating the request and reply messages.

private:
	Request_id out_rid; // Identifier of the outstanding request
	M_Request *out_req; // Outstanding request

	M_Certificate<M_Reply> r_reps; // Certificate with application replies
	bool is_panicking;
	bool bail_out;

	//Added by Maysam Yabandeh
	bool  responders[100];
	void processReceivedMessage(M_Message* m, int sock);
};

#endif // _M_Client_h
