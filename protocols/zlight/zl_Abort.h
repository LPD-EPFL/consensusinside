#ifndef _zl_Abort_h
#define _zl_Abort_h 1

#include "types.h"
#include "Digest.h"
#include "Request_history.h"

#include "zl_Message.h"
#include "zl_Request.h"

class zl_Principal;

// 
// zl_Abort messages have the following format.
//

struct zl_Abort_rep : public zl_Message_rep
{
   int replica; // replica which generated the abort
   Request_id rid; // id of the request which made replica panic
   Digest aborted_request_digest;
   Digest digest;
   short hist_size;
   short unused;
   int unused2;
};

// Followed by a hist_size tuples [cid, rid, digest]

class zl_Abort : public zl_Message
{
   //
   // zl_Abort messages
   //
public:

   zl_Abort(int p_replica, Request_id p_rid, Req_history_log &req_h);

   zl_Abort(zl_Abort_rep *contents);
   // Requires: "contents" contains a valid zl_Abort_rep.
   // Effects: Creates a message from "contents". No copy is made of
   // "contents" and the storage associated with "contents" is not
   // deallocated if the message is later deleted. Useful to create
   // messages from reps contained in other messages.

   Request_id& request_id() const;
   // Effects: Fetches the request identifier from the message.

   Digest& digest() const;

   Digest& aborted_request_digest() const;

   int id() const;
   // Effects: Fetches the identifier of the replica that sent the message.

   unsigned short hist_size() const;

   void sign();

   bool verify();

   // bool verify_by_abstract();

   static bool convert(zl_Message *m, zl_Abort *&r);
   // Converts the incomming message to order-request message

private:
   zl_Abort_rep &rep() const;
};

inline zl_Abort_rep& zl_Abort::rep() const
{
   th_assert(ALIGNED(msg), "Improperly aligned pointer");
   return *((zl_Abort_rep*)msg);
}

inline unsigned short zl_Abort::hist_size() const
{
   return rep().hist_size;
}

inline Request_id &zl_Abort::request_id() const
{
   return rep().rid;
}

inline Digest &zl_Abort::digest() const
{
   return rep().digest;
}

inline Digest &zl_Abort::aborted_request_digest() const
{
   return rep().aborted_request_digest;
}

inline int zl_Abort::id() const
{
   return rep().replica;
}

#endif // _zl_Abort_h
