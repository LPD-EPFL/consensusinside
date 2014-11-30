#ifndef _zl_Missing_h
#define _zl_Missing_h 1

#include "types.h"
#include "Digest.h"
#include "Request_history.h"

#include "zl_Message.h"
#include "zl_Request.h"

#include "zl_Smasher.h"

class zl_Principal;

// 
// zl_Missing messages have the following format.
//

struct zl_Missing_rep : public zl_Message_rep
{
   int replica; // replica which generated the abort
   Digest digest;
   short hist_size;
   short unused;
   int unused2;
};

// Followed by a hist_size tuples [cid, rid, digest]

class zl_Missing : public zl_Message
{
   //
   // zl_Missing messages
   //
public:

   zl_Missing(int p_replica, AbortHistory *missing);

   zl_Missing(zl_Missing_rep *contents);
   // Requires: "contents" contains a valid zl_Missing_rep.
   // Effects: Creates a message from "contents". No copy is made of
   // "contents" and the storage associated with "contents" is not
   // deallocated if the message is later deleted. Useful to create
   // messages from reps contained in other messages.

   Digest& digest() const;

   int id() const;
   // Effects: Fetches the identifier of the replica that sent the message.

   unsigned short hist_size() const;

   void sign();

   bool verify();

   // bool verify_by_abstract();

   static bool convert(zl_Message *m, zl_Missing *&r);
   // Converts the incomming message to order-request message

private:
   zl_Missing_rep &rep() const;
};

inline zl_Missing_rep& zl_Missing::rep() const
{
   th_assert(ALIGNED(msg), "Improperly aligned pointer");
   return *((zl_Missing_rep*)msg);
}

inline unsigned short zl_Missing::hist_size() const
{
   return rep().hist_size;
}

inline Digest &zl_Missing::digest() const
{
   return rep().digest;
}

inline int zl_Missing::id() const
{
   return rep().replica;
}

#endif // _zl_Missing_h
