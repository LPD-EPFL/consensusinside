#ifndef _Q_Client_h
#define _Q_Client_h 1

#include <stdio.h>
#include "types.h"
#include "Q_Node.h"
#include "Q_Request.h"
#include "Q_Reply.h"
#include "Q_Certificate.h"
#include "Q_ITimer.h"

//Added by Maysam Yabandeh
#include <vector>

extern void Q_rtimeQ_handler();

class Q_Client: public Q_Node {
public:
	Q_Client(FILE *config_file, char *host_name, short port = 0);
	// Effects: Creates a new Client object using the information in
	// "config_file". The line of config assigned to
	// this client is the first one with the right host address (if
	// port==0) or the first with the right host address and port equal
	// to "port".

	virtual ~Q_Client();
	// Effects: Deallocates all storage associated with this.

	bool send_request(Q_Request *req, int size, bool ro);
	// Effects: Sends request m to the primary. Returns FALSE iff two
	// consecutive request were made without waiting for a reply between
	// them.

	bool send_ordered_request(Q_Request *req, int size, bool ro);
	// Effects: Sends the ordered request received from the primary to 
	// the other replicas. Returns FALSE iff two consecutive request were
	// made without waiting for a reply between them.

	Q_Reply *recv_reply();
	// Effects: Blocks until it receives enough reply messages for
	// the previous request. returns a pointer to the reply. The caller is
	// responsible for deallocating the request and reply messages.

	void retransmit();
	// Effects: Retransmits any outstanding request

private:
	Request_id out_rid; // Identifier of the outstanding request
	Q_Request *out_req; // Outstanding request

	int n_retrans;        // Number of retransmissions of out_req
	int rtimeout;         // PBFT_R_Timeout period in msecs
	// Maximum retransmission timeout in msecs
	static const int Max_rtimeout = 1000;
	// Minimum retransmission timeout after retransmission 
	// in msecs
	static const int Min_rtimeout = 10;
	Q_ITimer *rtimer;       // Retransmission timer

	Q_Certificate<Q_Reply> r_reps; // Certificate with application replies
	bool is_panicking;
	bool bail_out;

	//Added by Maysam Yabandeh
	static bool recievedOrder;
	static int order;
	static int lastRequestSize;
   //TODO 100 -> num_replicas
	bool  responders[100];
	void processReceivedMessage(Q_Message* m, int sock);
};

#endif // _Q_Client_h
