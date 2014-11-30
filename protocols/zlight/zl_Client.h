#ifndef _zl_Client_h
#define _zl_Client_h 1

#include <stdio.h>
#include "types.h"
#include "zl_Node.h"
#include "zl_Request.h"
#include "zl_Reply.h"
#include "zl_Certificate.h"
#include "zl_ITimer.h"

extern void zl_rtimezl_handler();

class zl_Client: public zl_Node {
public:
	zl_Client(FILE *config_file, char *host_name, short port = 0);
	// Effects: Creates a new Client object using the information in
	// "config_file". The line of config assigned to
	// this client is the first one with the right host address (if
	// port==0) or the first with the right host address and port equal
	// to "port".

	virtual ~zl_Client();
	// Effects: Deallocates all storage associated with this.

	bool send_request(zl_Request *req, int size, bool ro);
	// Effects: Sends request m to the service. Returns FALSE iff two
	// consecutive request were made without waiting for a reply between
	// them.

	zl_Reply *recv_reply();
	// Effects: Blocks until it receives enough reply messages for
	// the previous request. returns a pointer to the reply. The caller is
	// responsible for deallocating the request and reply messages.

	void retransmit();
	// Effects: Retransmits any outstanding request

private:
	Request_id out_rid; // Identifier of the outstanding request
	zl_Request *out_req; // Outstanding request

	int n_retrans;        // Number of retransmissions of out_req
	int rtimeout;         // PBFT_R_Timeout period in msecs
	// Maximum retransmission timeout in msecs
	static const int Max_rtimeout = 1000;
	// Minimum retransmission timeout after retransmission 
	// in msecs
	static const int Min_rtimeout = 10;
	zl_ITimer *rtimer;       // Retransmission timer

	zl_Certificate<zl_Reply> r_reps; // Certificate with application replies
};

#endif // _zl_Client_h
