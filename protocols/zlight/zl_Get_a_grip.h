#ifndef _zl_Get_a_grip_h
#define _zl_Get_a_grip_h 1

#include "types.h"
#include "zl_Message.h"
#include "Digest.h"
#include "zl_Request.h"
#include "zl_Reply.h"

class zl_Principal;

//
// zl_Get_a_grip messages have the following format.
//
//

struct zl_Get_a_grip_rep : public zl_Message_rep
{
   Request_id req_id;
   int cid;
   int replica;
   Seqno seqno;
};

class zl_Get_a_grip : public zl_Message
{
   //
   // zl_Get_a_grip messages
   //
public:
   zl_Get_a_grip() : zl_Message() {}

   zl_Get_a_grip(int cid, Request_id req_id, int replica, Seqno s, zl_Request *r);

   zl_Get_a_grip(zl_Get_a_grip_rep *contents);
   // Requires: "contents" contains a valid zl_Get_a_grip_rep.
   // Effects: Creates a message from "contents". No copy is made of
   // "contents" and the storage associated with "contents" is not
   // deallocated if the message is later deleted. Useful to create
   // messages from reps contained in other messages.

   int client_id() const;
   // Effects: Fetches the identifier of the client from the message.

   int id() const;
   // Effects: Fetches the identifier of the replica that sent the message.

   Request_id& request_id();
   // Effects: Fetches the request identifier from the message.

   Seqno seqno() { return rep().seqno; }

   char* stored_request();
   // Effects: Returns the reply that is contained in this zl_Get_a_grip message

   zl_Get_a_grip* copy(int id) const;
   // Effects: Creates a new object with the same state as this but
   // with replica identifier "id"

   static bool convert(zl_Message *m, zl_Get_a_grip *&r);
   // Converts the incomming message to order-request message

private:
   zl_Get_a_grip_rep &rep() const;
};

inline zl_Get_a_grip_rep& zl_Get_a_grip::rep() const
{
   th_assert(ALIGNED(msg), "Improperly aligned pointer");
   return *((zl_Get_a_grip_rep*)msg);
}

inline char* zl_Get_a_grip::stored_request()
{
   char *ret = contents()+sizeof(zl_Get_a_grip_rep);
   th_assert(ALIGNED(ret), "Improperly aligned pointer");
   return ret;
}

inline int zl_Get_a_grip::client_id() const
{
   return rep().cid;
}

inline int zl_Get_a_grip::id() const
{
   return rep().replica;
}

inline Request_id &zl_Get_a_grip::request_id()
{
   return rep().req_id;
}

#endif // _zl_Get_a_grip_h
