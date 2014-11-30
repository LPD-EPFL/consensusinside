#ifndef _Request_history_h
#define _Request_history_h 1

#include "Array.h"
#include "MD5.h"
#include "Digest.h"
#include "types.h"
#include "ARequest.h"
#include "DSum.h"

class Rh_entry
{
public:
   inline Rh_entry()
   {
   }

   inline Rh_entry(ARequest *r, Seqno s, Digest d)
   {
      req = r;
      seq = s;
      d_h = d;
   }

   inline ~Rh_entry()
   {
   }
   ARequest *req;
   Seqno seq;
   Digest d_h;

   inline Request_id& request_id() const
   {
       return req->request_id();
   }

   inline Seqno seqno() const
   {
       return seq;
   }

   inline Digest& digest()
   {
       return d_h;
   }
};

class Req_history_log
{

public:
   // Overview : This is a request history log with request entries ordered
   // by the assigned sequence numbers.

   // How many requests will we purge
   static const int TRUNCATE_SIZE = 128;

   Req_history_log();
   // Effects: Creates an empty table.

   ~Req_history_log();
   // Effects: Deletes table

   bool add_request(ARequest *req, Seqno seq, Digest &d);
   // Creates an entry for the request in the request history log
   // Returns true if the request has been correctly added.

   bool truncate_history(Seqno seq);
   // purges the history for amount entries up to rid

   bool should_checkpoint();
   // returns true if truncation threshold is reached, and we're not waiting for the confirmation

   Seqno get_top_seqno() const;
   // returns the Request_id at the top

   Digest rh_digest();

   int size() const;

   Array<Rh_entry>& array();

   Rh_entry* find(int cid, Request_id rid);

private:
   Array<Rh_entry> rh;
   DSum rh_d; // Adhash sum of the history
   DSum* others;
   DSum* my_dsum;
};
#endif
