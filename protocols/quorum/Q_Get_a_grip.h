#ifndef _Q_Get_a_grip_h
#define _Q_Get_a_grip_h 1

#include "types.h"
#include "Q_Message.h"
#include "Digest.h"
#include "Q_Request.h"
#include "Q_Reply.h"

class Q_Principal;

//
// Q_Get_a_grip messages have the following format.
//
//

struct Q_Get_a_grip_rep : public Q_Message_rep
{
   Request_id req_id;
   int cid;
   int replica;
   Seqno seqno;
};

class Q_Get_a_grip : public Q_Message
{
   //
   // Q_Get_a_grip messages
   //
public:
   Q_Get_a_grip() : Q_Message() {}

   Q_Get_a_grip(int cid, Request_id req_id, int replica, Seqno s, Q_Request *r);

   Q_Get_a_grip(Q_Get_a_grip_rep *contents);
   // Requires: "contents" contains a valid Q_Get_a_grip_rep.
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
   // Effects: Returns the reply that is contained in this Q_Get_a_grip message

   Q_Get_a_grip* copy(int id) const;
   // Effects: Creates a new object with the same state as this but
   // with replica identifier "id"

   static bool convert(Q_Message *m, Q_Get_a_grip *&r);
   // Converts the incomming message to order-request message

private:
   Q_Get_a_grip_rep &rep() const;
};

inline Q_Get_a_grip_rep& Q_Get_a_grip::rep() const
{
   th_assert(ALIGNED(msg), "Improperly aligned pointer");
   return *((Q_Get_a_grip_rep*)msg);
}

inline char* Q_Get_a_grip::stored_request()
{
   char *ret = contents()+sizeof(Q_Get_a_grip_rep);
   th_assert(ALIGNED(ret), "Improperly aligned pointer");
   return ret;
}

inline int Q_Get_a_grip::client_id() const
{
   return rep().cid;
}

inline int Q_Get_a_grip::id() const
{
   return rep().replica;
}

inline Request_id &Q_Get_a_grip::request_id()
{
   return rep().req_id;
}

#endif // _Q_Get_a_grip_h
