#include <stdlib.h>
#include <string.h>

#include "R_Rep_info.h"
#include "R_Replica.h"
#include "R_Reply.h"

R_Rep_info::R_Rep_info(int n)
{
  th_assert(n != 0, "Invalid argument");

  nps = n;
  mem = (char *) valloc(size());

  int old_nps = *((Long*) mem);
  if (old_nps != 0)
    {
      // Memory has already been initialized.
      if (nps != old_nps)
        {
          th_fail("Changing number of principals. Not implemented yet");
        }
    }
  else
    {
      // Initialize memory.
      bzero(mem, (nps + 1) * Max_rep_size);
      for (int i = 0; i < nps; i++)
        {
          // Wasting first page just to store the number of principals.
          R_Reply_rep* rr = (R_Reply_rep*) (mem + (i + 1) * Max_rep_size);
          rr->tag = R_Reply_tag;
          rr->reply_size = -1;
          rr->rid = 0;
        }
      *((Long*) mem) = nps;
    }

  reps = (R_Reply**) malloc(nps * sizeof(R_Reply*));
  for (int i = 0; i < nps; i++)
    {
      R_Reply_rep *rr = (R_Reply_rep*) (mem + (i + 1) * Max_rep_size);
      th_assert(rr->tag == R_Reply_tag, "Corrupt memory");
      reps[i] = new R_Reply(rr);
    }
}

R_Rep_info::~R_Rep_info()
{
  for (int i = 0; i < nps; i++)
    {
      delete reps[i];
    }
  delete reps;
}

char*
R_Rep_info::new_reply(int pid, int &sz)
{
  R_Reply* r = reps[pid];

  r->rep().reply_size = -1;
  sz = Max_rep_size - sizeof(R_Reply_rep) - (R_node->f() + 1) * (R_UMAC_size
      + R_UNonce_size);
  return r->contents() + sizeof(R_Reply_rep);
}

R_Reply_rep *
R_Rep_info::end_reply(int pid, int entry_replica, Request_id rid, Seqno seqno, int sz)
{
  R_Reply* r = reps[pid];
  th_assert(r->rep().reply_size == -1, "Invalid state");

  R_Reply_rep& rr = r->rep();
  rr.replica = entry_replica;

  rr.rid = rid;
  rr.reply_size = sz;
  rr.digest = Digest(r->contents() + sizeof(R_Reply_rep), sz);

  int old_size = sizeof(R_Reply_rep) + rr.reply_size;
  r->set_size(old_size + (R_node->f() + 1) * (R_UMAC_size + R_UNonce_size));

  return (R_Reply_rep *) r->contents();
}

void
R_Rep_info::send_reply(int pid, int socket, char *MACs)
{
  R_Reply *r = reps[pid];
  R_Reply_rep& rr = r->rep();
  int old_size = sizeof(R_Reply_rep) + rr.reply_size;
  //rr.cid = -1;
  th_assert(rr.reply_size != -1, "Invalid state");
  th_assert(rr.extra == 0, "Invalid state");

#if USE_MACS
  memcpy(r->contents() + old_size, MACs, (R_node->f() + 1) * (R_UNonce_size
      + R_UMAC_size));
#endif

  R_node->send(r, socket);

}
