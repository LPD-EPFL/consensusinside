#ifndef _O_Client_h
#define _O_Client_h 1

#include <stdio.h>
#include "types.h"
#include "O_Node.h"
#include "O_Request.h"
#include "O_Reply.h"
#include "O_Certificate.h"
#include "O_Abandon.h"

//Added by Maysam Yabandeh
#include <vector>

extern void O_rtimeT_handler();

class O_Client: public O_Node {
public:
	O_Client(FILE *config_file, char *host_name, short port = 0);
	// Effects: Creates a new Client object using the information in
	// "config_file". The line of config assigned to
	// this client is the first one with the right host address (if
	// port==0) or the first with the right host address and port equal
	// to "port".

	virtual ~O_Client();
	// Effects: Deallocates all storage associated with this.

	bool send_request(O_Request *req, int size, bool ro);
	// Effects: Sends request m to the primary. Returns FALSE iff two
	// consecutive request were made without waiting for a reply between
	// them.

	O_Reply *recv_reply();
	// Effects: Blocks until it receives enough reply messages for
	// the previous request. returns a pointer to the reply. The caller is
	// responsible for deallocating the request and reply messages.

private:
	Request_id out_rid; // Identifier of the outstanding request
	O_Request *out_req; // Outstanding request

	O_Certificate<O_Reply> r_reps; // Certificate with application replies
	bool is_panicking;
	bool bail_out;

	//Added by Maysam Yabandeh
	bool  responders[100];
	void processReceivedMessage(O_Message* m, int sock);
	void handle(O_Abandon *m, int sock);
	uint8_t currentLeader;
public:
   const Request_id& get_out_rid() { return out_rid; }
   O_Request* get_out_req() { return out_req; }
   const int& get_num_replicas() { return num_replicas; }
   const uint8_t& get_currentLeader() { return currentLeader; }
   void set_currentLeader(int l) { currentLeader = l; }
};

#endif // _O_Client_h
