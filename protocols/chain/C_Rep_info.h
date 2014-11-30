#ifndef _C_Rep_info_h
#define _C_Rep_info_h 1

#include <sys/time.h>
#include "types.h"
#include "Digest.h"
#include "C_Reply.h"

class C_Req_queue;

class C_Rep_info
{
  //
  // Holds the last replies sent to each principal.
  //
public:
  C_Rep_info(int nps);
  // Effects: Creates a new object that stores data for "nps"
  // principals.

  ~C_Rep_info();

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

  C_Reply*
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

  C_Reply_rep *
  end_reply(int pid, Request_id rid, int size);

  void
  send_reply(int pid, int socket, char *MACs);
private:
  int nps;
  char *mem;
  C_Reply** reps; // Array of replies indexed by principal id.
  static const int Max_rep_size = 8192;
};

inline int
C_Rep_info::size() const
{
  return (nps + 1) * Max_rep_size;
}

inline Request_id
C_Rep_info::req_id(int pid)
{
  return reps[pid]->request_id();
}

inline Digest&
C_Rep_info::digest(int pid)
{
  return reps[pid]->digest();
}

inline C_Reply*
C_Rep_info::reply(int pid)
{
  return reps[pid];
}

#endif // _C_Rep_info_h
