#ifndef _Q_Panic_h
#define _Q_Panic_h 1

#include "Q_Message.h"
#include "types.h"
#include "Digest.h"

class Q_Request;
class Q_Principal;

// 
// Q_Panic messages have the following format.
//
// Q_Panic_rep.extra = 4 if commit opt is on
// 

struct Q_Panic_rep : public Q_Message_rep
{
   Request_id req_id;
   int cid;
   int unused;
};

class Q_Panic : public Q_Message
{
   // 
   // Q_Panic messages
   //
public:
   Q_Panic() :
      Q_Message()
   {};

   Q_Panic(Q_Request *req);
   Q_Panic(int cid, Request_id rid);

   Q_Panic(Q_Panic_rep *contents);
   // Requires: "contents" contains a valid Q_Panic_rep.
   // Effects: Creates a message from "contents". No copy is made of
   // "contents" and the storage associated with "contents" is not
   // deallocated if the message is later deleted. Useful to create
   // messages from reps contained in other messages.

   int client_id() const;
   // Effects: Fetches the identifier of the client from the message.

   Request_id& request_id();
   // Effects: Fetches the request identifier from the message.

   Digest& panic_request_digest();
   // Effects: Fetches the request identifier from the message.

   Q_Panic* copy(int id) const;
   // Effects: Creates a new object with the same state as this but
   // with replica identifier "id"

   static bool convert(Q_Message *m, Q_Panic *&r);
   // Converts the incomming message to order-request message

private:
   Q_Panic_rep &rep() const;
};

inline Q_Panic::Q_Panic(int cid, Request_id rid)
    : Q_Message(Q_Panic_tag, Q_Max_message_size)
{
    rep().cid = cid;
    rep().req_id = rid;
    set_size(sizeof(Q_Panic_rep));
};

inline Q_Panic_rep& Q_Panic::rep() const
{
   th_assert(ALIGNED(msg), "Improperly aligned pointer");
   return *((Q_Panic_rep*)msg);
}

inline int Q_Panic::client_id() const
{
   return rep().cid;
}

inline Request_id &Q_Panic::request_id()
{
   return rep().req_id;
}

#endif // _Q_Panic_h
