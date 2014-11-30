#ifndef _PBFT_R_Client_h
#define _PBFT_R_Client_h 1

#include <stdio.h>
#include "types.h"
#include "PBFT_R_Node.h"
#include "PBFT_R_Certificate.h"

class PBFT_R_Reply;
class PBFT_R_Request;
class PBFT_R_ITimer;
extern void PBFT_R_rtimePBFT_R_handler();

class PBFT_R_Client : public PBFT_R_Node {
public:
  PBFT_R_Client(FILE *config_file, FILE *config_priv, char *host_name, short port=0);
  // Effects: Creates a new PBFT_R_Client object using the information in
  // "config_file" and "config_priv". The line of config assigned to
  // this client is the first one with the right host address (if
  // port==0) or the first with the right host address and port equal
  // to "port".

  virtual ~PBFT_R_Client();
  // Effects: Deallocates all storage associated with this.

  bool send_request(PBFT_R_Request *req);
  // Effects: Sends request m to the service. Returns FALSE iff two
  // consecutive request were made without waiting for a reply between
  // them.

  PBFT_R_Reply *recv_reply();
  // Effects: Blocks until it receives enough reply messages for
  // the previous request. returns a pointer to the reply. The caller is
  // responsible for deallocating the request and reply messages.

  Request_id get_rid() const;
  // Effects: Returns the current outstanding request identifier. The request
  // identifier is updated to a new value when the previous message is
  // delivered to the user.

  void reset();
  // Effects: Resets client state to ensure independence of experimental
  // points.

private:
  PBFT_R_Request *out_req;     // Outstanding request
  bool need_auth;       // Whether to compute new authenticator for out_req
  Request_id out_rid;   // Identifier of the outstanding request
  int n_retrans;        // Number of retransmissions of out_req
  int rtimeout;         // PBFT_R_Timeout period in msecs

  // Maximum retransmission timeout in msecs
  static const int Max_rtimeout = 1000;

  // Minimum retransmission timeout after retransmission 
  // in msecs
  static const int Min_rtimeout = 10;

  PBFT_R_Cycle_counter latency; // Used to measure latency.

  // Multiplier used to obtain retransmission timeout from avg_latency
  static const int Rtimeout_mult = 4; 

  PBFT_R_Certificate<PBFT_R_Reply> t_reps; // PBFT_R_Certificate with tentative replies (size 2f+1)
  PBFT_R_Certificate<PBFT_R_Reply> c_reps; // PBFT_R_Certificate with committed replies (size f+1)

  friend void PBFT_R_rtimePBFT_R_handler();
  PBFT_R_ITimer *rtimer;       // Retransmission timer

  void retransmit();
  // Effects: Retransmits any outstanding request and last new-key message.

  void send_new_key();
  // Effects: Calls PBFT_R_Node's send_new_key, and cleans up stale replies in
  // certificates.
};

inline Request_id PBFT_R_Client::get_rid() const { return out_rid; } 

#endif // _PBFT_R_Client_h

