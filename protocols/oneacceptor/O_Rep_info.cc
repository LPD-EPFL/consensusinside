#include <stdlib.h>
#include <string.h>

#include "O_Rep_info.h"
#include "O_Replica.h"
#include "O_Reply.h"

#define O_MAC_size 0

O_Rep_info::O_Rep_info(int n)
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
          O_Reply_rep* rr = (O_Reply_rep*) (mem + (i + 1) * Max_rep_size);
          rr->tag = O_Reply_tag;
          rr->reply_size = -1;
          rr->rid = 0;
          rr->cid = -1;
        }
      *((Long*) mem) = nps;
    }

  reps = (O_Reply**) malloc(nps * sizeof(O_Reply*));
  for (int i = 0; i < nps; i++)
    {
      O_Reply_rep *rr = (O_Reply_rep*) (mem + (i + 1) * Max_rep_size);
      th_assert(rr->tag == O_Reply_tag, "Corrupt memory");
      reps[i] = new O_Reply(rr);
    }
}

O_Rep_info::~O_Rep_info()
{
  for (int i = 0; i < nps; i++)
    {
      delete reps[i];
    }
  delete reps;
}

char*
O_Rep_info::new_reply(int pid, int &sz)
{
  O_Reply* r = reps[pid];

  r->rep().reply_size = -1;
  sz = Max_rep_size - sizeof(O_Reply_rep) - O_MAC_size;
  return r->contents() + sizeof(O_Reply_rep);
}

void
O_Rep_info::end_reply(int pid, Request_id rid, int sz)//, const Digest &d)
{
  O_Reply* r = reps[pid];
  th_assert(r->rep().reply_size == -1, "Invalid state");

  O_Reply_rep& rr = r->rep();
  rr.rid = rid;
  rr.reply_size = sz;
  rr.cid = -1;
  //rr.rh_digest = d;
  //rr.digest = Digest(r->contents() + sizeof(O_Reply_rep), sz);

  int old_size = sizeof(O_Reply_rep) + rr.reply_size;
  r->set_size(old_size + O_MAC_size);
  bzero(r->contents() + old_size, O_MAC_size);
}

void
O_Rep_info::send_reply(int dest_pid, View v, int src_pid, bool tentative)
{
  O_Reply *r = reps[dest_pid];
  O_Reply_rep& rr = r->rep();
  int old_size = sizeof(O_Reply_rep) + rr.reply_size;

  th_assert(rr.reply_size != -1, "Invalid state");
  th_assert(rr.extra == 0&& rr.v == 0&& rr.replica == 0&&rr.cid == -1,
      "Invalid state");

  rr.v = v;
  rr.replica = src_pid;
  rr.cid = -1;
  //O_my_principal->gen_mac(r->contents(), sizeof(O_Reply_rep), r->contents()
      //+ old_size, dest_pid);

  O_node->send(r, dest_pid);

  // Undo changes. To ensure state matches across all replicas.
  rr.v = 0;
  rr.replica = 0;
  rr.cid = -1;
  bzero(r->contents() + old_size, O_MAC_size);
}
