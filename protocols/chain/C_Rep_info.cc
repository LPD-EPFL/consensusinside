#include <stdlib.h>
#include <string.h>

#include "C_Rep_info.h"
#include "C_Replica.h"
#include "C_Reply.h"

C_Rep_info::C_Rep_info(int n)
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
          C_Reply_rep* rr = (C_Reply_rep*) (mem + (i + 1) * Max_rep_size);
          rr->tag = C_Reply_tag;
          rr->reply_size = -1;
          rr->rid = 0;
        }
      *((Long*) mem) = nps;
    }

  reps = (C_Reply**) malloc(nps * sizeof(C_Reply*));
  for (int i = 0; i < nps; i++)
    {
      C_Reply_rep *rr = (C_Reply_rep*) (mem + (i + 1) * Max_rep_size);
      th_assert(rr->tag == C_Reply_tag, "Corrupt memory");
      reps[i] = new C_Reply(rr);
    }
}

C_Rep_info::~C_Rep_info()
{
  for (int i = 0; i < nps; i++)
    {
      delete reps[i];
    }
  delete reps;
}

char*
C_Rep_info::new_reply(int pid, int &sz)
{
  C_Reply* r = reps[pid];

  r->rep().reply_size = -1;
  sz = Max_rep_size - sizeof(C_Reply_rep) - (C_node->f() + 1) * (C_UMAC_size
      + C_UNonce_size);
  return r->contents() + sizeof(C_Reply_rep);
}

C_Reply_rep *
C_Rep_info::end_reply(int pid, Request_id rid, int sz)
{
  C_Reply* r = reps[pid];
  th_assert(r->rep().reply_size == -1, "Invalid state");

  C_Reply_rep& rr = r->rep();
  rr.v = 0;
  rr.replica = 0;

  rr.rid = rid;
  rr.reply_size = sz;
  rr.cid = -1;
  rr.digest = Digest(r->contents() + sizeof(C_Reply_rep), sz);

  int old_size = sizeof(C_Reply_rep) + rr.reply_size;
  r->set_size(old_size + (C_node->f() + 1) * (C_UMAC_size + C_UNonce_size));
  bzero(r->contents() + old_size, (C_node->f() + 1) * (C_UMAC_size + C_UNonce_size));

  return (C_Reply_rep *) r->contents();
}

void
C_Rep_info::send_reply(int pid, int socket, char *MACs)
{
  C_Reply *r = reps[pid];
  C_Reply_rep& rr = r->rep();
  int old_size = sizeof(C_Reply_rep) + rr.reply_size;
  rr.cid = -1;
  th_assert(rr.reply_size != -1, "Invalid state");
  th_assert(rr.extra == 0&& rr.v == 0&& rr.replica == 0, "Invalid state");

  memcpy(r->contents() + old_size, MACs, (C_node->f() + 1) * (C_UNonce_size
      + C_UMAC_size));

  C_node->send(r, socket);

}
