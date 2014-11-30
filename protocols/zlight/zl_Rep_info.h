#ifndef _zl_Rep_info_h
#define _zl_Rep_info_h 1

#include <sys/time.h>
#include "types.h"
#include "Digest.h"
#include "zl_Reply.h"

class Req_queue;

class zl_Rep_info
{
   //
   // Holds the last replies sent to each principal.
   //
public:
   zl_Rep_info(int nps);
   // Effects: Creates a new object that stores data for "nps"
   // principals.

   ~zl_Rep_info();

   int size() const;
   // Effects: Returns the actual number of bytes (a multiple of the
   // virtual memory page size) that was consumed by this from the
   // start of the "mem" argument supplied to the constructor.

   Request_id req_id(int pid);
   // Requires: "pid" is a valid principal identifier.
   // Effects: Returns the timestamp in the last message sent to
   // principal "pid".

   Digest &digest(int pid);
   // Requires: "pid" is a valid principal identifier.
   // Effects: Returns a reference to the digest of the last reply
   // value sent to pid.

   zl_Reply* reply(int pid);
   // Requires: "pid" is a valid principal identifier.
   // Effects: Returns a pointer to the last reply value sent to "pid"
   // or 0 if no such reply was sent.

   char *new_reply(int pid, int &size);
   // Requires: "pid" is a valid principal identifier.
   // Effects: Returns a pointer to a buffer where the new reply value
   // for principal "pid" can be placed. The length of the buffer in bytes
   // is returned in "size". Sets the reply to tentative.

   void end_reply(int pid, Request_id rid, int size, const Digest &d);
   // Requires: "pid" is a valid principal identifier.
   // Effects: Completes the construction of a new reply value: this is
   // informed that the reply value is size bytes long and computes its
   // digest.

   void send_reply(int dest_pid, View v, int src_pid, bool tentative=true);
   // Requires: "dest_pid" is a valid principal identifier and end_reply was
   // called after the last call to new_reply for "dest_pid"
   // Effects: Sends a reply message to "dest_pid" for view "v" from replica
   // "src_pid" with the latest reply value stored in the buffer returned by
   // new_reply. If tentative is omitted or true, it sends the reply as
   // tentative unless it was previously committed

private:
   int nps;
   char *mem;
   zl_Reply** reps; // Array of replies indexed by principal id.
   static const int Max_rep_size = 8192;
};

inline int zl_Rep_info::size() const
{
   return (nps+1)*Max_rep_size;
}

inline Request_id zl_Rep_info::req_id(int pid)
{
   return reps[pid]->request_id();
}

inline Digest& zl_Rep_info::digest(int pid)
{
   return reps[pid]->digest();
}

inline zl_Reply* zl_Rep_info::reply(int pid)
{
   return reps[pid];
}

#endif // _Rep_info_h
