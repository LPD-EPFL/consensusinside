#ifndef _zl_Panic_h
#define _zl_Panic_h 1

#include "zl_Message.h"
#include "types.h"
#include "Digest.h"

class zl_Request;
class zl_Principal;

// 
// zl_Panic messages have the following format.
//
// zl_Panic_rep.extra = 4 if commit opt is on
// 

struct zl_Panic_rep : public zl_Message_rep
{
   Request_id req_id;
   int cid;
   int unused;
};

class zl_Panic : public zl_Message
{
   // 
   // zl_Panic messages
   //
public:
   zl_Panic() :
      zl_Message()
   {};

   zl_Panic(zl_Request *req);
   zl_Panic(int cid, Request_id rid);

   zl_Panic(zl_Panic_rep *contents);
   // Requires: "contents" contains a valid zl_Panic_rep.
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

   zl_Panic* copy(int id) const;
   // Effects: Creates a new object with the same state as this but
   // with replica identifier "id"

   static bool convert(zl_Message *m, zl_Panic *&r);
   // Converts the incomming message to order-request message

private:
   zl_Panic_rep &rep() const;
};

inline zl_Panic::zl_Panic(int cid, Request_id rid)
    : zl_Message(zl_Panic_tag, zl_Max_message_size)
{
    rep().cid = cid;
    rep().req_id = rid;
    set_size(sizeof(zl_Panic_rep));
};

inline zl_Panic_rep& zl_Panic::rep() const
{
   th_assert(ALIGNED(msg), "Improperly aligned pointer");
   return *((zl_Panic_rep*)msg);
}

inline int zl_Panic::client_id() const
{
   return rep().cid;
}

inline Request_id &zl_Panic::request_id()
{
   return rep().req_id;
}

#endif // _zl_Panic_h
