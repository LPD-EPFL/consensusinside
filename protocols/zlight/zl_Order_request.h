#ifndef _Order_request_h
#define _Order_request_h 1

#include "types.h"
#include "zl_Message.h"
#include "Digest.h"
#include "zl_Request.h"

class zl_Principal;

// 
// Order_request messages have the following format.
//
// Order_request_rep.extra = 4 if commit opt is on
// 

struct zl_Order_request_rep : public zl_Message_rep
{
   View v; // Current view
   Seqno seq_num; // Sequence number
   Request_id req_id;
   int cid;
   uint32_t unused;
};

class zl_Order_request : public zl_Message
{
   //
   // Order_request messages
   //
public:
   zl_Order_request() :
      zl_Message()
   {
   }

   zl_Order_request(View view, Seqno s, zl_Request *req);

   zl_Order_request(zl_Order_request_rep *contents);
   // Requires: "contents" contains a valid Order_request_rep.
   // Effects: Creates a message from "contents". No copy is made of
   // "contents" and the storage associated with "contents" is not
   // deallocated if the message is later deleted. Useful to create
   // messages from reps contained in other messages.

   int client_id() const;
   // Effects: Fetches the identifier of the client from the message.

   bool verify();

   Request_id& request_id();
   // Effects: Fetches the request identifier from the message.

   Seqno& seqno();
   // Effects: Fetches the sequence number associated with this message.

   char* stored_request();
   // Effects: Returns the request that is contained in this message

   zl_Order_request* copy(int id) const;
   // Effects: Creates a new object with the same state as this but
   // with replica identifier "id"

   static bool convert(zl_Message *m, zl_Order_request *&r);
   // Converts the incomming message to order-request message

private:
   zl_Order_request_rep &rep() const;
};

inline zl_Order_request_rep& zl_Order_request::rep() const
{
   th_assert(ALIGNED(msg), "Improperly aligned pointer");
   return *((zl_Order_request_rep*)msg);
}

inline int zl_Order_request::client_id() const
{
   return rep().cid;
}

inline Request_id &zl_Order_request::request_id()
{
   return rep().req_id;
}

inline Seqno &zl_Order_request::seqno()
{
   return rep().seq_num;
}

inline char* zl_Order_request::stored_request()
{
   char *ret = contents()+sizeof(zl_Order_request_rep) + zl_node->auth_size();
   th_assert(ALIGNED(ret), "Improperly aligned pointer");
   return ret;
}

#endif // _Order_request_h
