#ifndef _Q_Missing_h
#define _Q_Missing_h 1

#include "types.h"
#include "Digest.h"
#include "Request_history.h"

#include "Q_Message.h"
#include "Q_Request.h"

#include "Q_Smasher.h"

class Q_Principal;

// 
// Q_Missing messages have the following format.
//

struct Q_Missing_rep : public Q_Message_rep
{
   int replica; // replica which generated the abort
   Digest digest;
   short hist_size;
   short unused;
   int unused2;
};

// Followed by a hist_size tuples [cid, rid, digest]

class Q_Missing : public Q_Message
{
   //
   // Q_Missing messages
   //
public:

   Q_Missing(int p_replica, AbortHistory *missing);

   Q_Missing(Q_Missing_rep *contents);
   // Requires: "contents" contains a valid Q_Missing_rep.
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

   static bool convert(Q_Message *m, Q_Missing *&r);
   // Converts the incomming message to order-request message

private:
   Q_Missing_rep &rep() const;
};

inline Q_Missing_rep& Q_Missing::rep() const
{
   th_assert(ALIGNED(msg), "Improperly aligned pointer");
   return *((Q_Missing_rep*)msg);
}

inline unsigned short Q_Missing::hist_size() const
{
   return rep().hist_size;
}

inline Digest &Q_Missing::digest() const
{
   return rep().digest;
}

inline int Q_Missing::id() const
{
   return rep().replica;
}

#endif // _Q_Missing_h
