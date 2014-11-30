#ifndef _R_Rep_info_h
#define _R_Rep_info_h 1

#include <sys/time.h>
#include "types.h"
#include "Digest.h"
#include "R_Reply.h"

class R_Req_queue;

class R_Rep_info
{
  //
  // Holds the last replies sent to each principal.
  //
public:
  R_Rep_info(int nps);
  // Effects: Creates a new object that stores data for "nps"
  // principals.

  ~R_Rep_info();

  int
  size() const;
  // Effects: Returns the actual number of bytes (a multiple of the
  // virtual memory page size) that was consumed by this from the
  // start of the "mem" argument supplied to the constructor.

  Request_id
  req_id(int pid);
  // Requires: "pid" is a valid principal identifier.
  // Effects: Returns the timestamp in the last message sent to
  // principal "pid".

  Digest &
  digest(int pid);
  // Requires: "pid" is a valid principal identifier.
  // Effects: Returns a reference to the digest of the last reply
  // value sent to pid.

  R_Reply*
  reply(int pid);
  // Requires: "pid" is a valid principal identifier.
  // Effects: Returns a pointer to the last reply value sent to "pid"
  // or 0 if no such reply was sent.

  char *
  new_reply(int pid, int &size);
  // Requires: "pid" is a valid principal identifier.
  // Effects: Returns a pointer to a buffer where the new reply value
  // for principal "pid" can be placed. The length of the buffer in bytes
  // is returned in "size". Sets the reply to tentative.

  R_Reply_rep *
  end_reply(int pid, int entry_replica, Request_id rid, Seqno seqno, int size);

  void
  send_reply(int pid, int socket, char *MACs);
private:
  int nps;
  char *mem;
  R_Reply** reps; // Array of replies indexed by principal id.
  static const int Max_rep_size = 8192;
};

inline int
R_Rep_info::size() const
{
  return (nps + 1) * Max_rep_size;
}

inline Request_id
R_Rep_info::req_id(int pid)
{
  return reps[pid]->request_id();
}

inline Digest&
R_Rep_info::digest(int pid)
{
  return reps[pid]->digest();
}

inline R_Reply*
R_Rep_info::reply(int pid)
{
  return reps[pid];
}

#endif // _R_Rep_info_h
