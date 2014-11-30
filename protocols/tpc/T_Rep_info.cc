#include <stdlib.h>
#include <string.h>

#include "T_Rep_info.h"
#include "T_Replica.h"
#include "T_Reply.h"

#define T_MAC_size 0

T_Rep_info::T_Rep_info(int n)
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
          T_Reply_rep* rr = (T_Reply_rep*) (mem + (i + 1) * Max_rep_size);
          rr->tag = T_Reply_tag;
          rr->reply_size = -1;
          rr->rid = 0;
          rr->cid = -1;
        }
      *((Long*) mem) = nps;
    }

  reps = (T_Reply**) malloc(nps * sizeof(T_Reply*));
  for (int i = 0; i < nps; i++)
    {
      T_Reply_rep *rr = (T_Reply_rep*) (mem + (i + 1) * Max_rep_size);
      th_assert(rr->tag == T_Reply_tag, "Corrupt memory");
      reps[i] = new T_Reply(rr);
    }
}

T_Rep_info::~T_Rep_info()
{
  for (int i = 0; i < nps; i++)
    {
      delete reps[i];
    }
  delete reps;
}

char*
T_Rep_info::new_reply(int pid, int &sz)
{
  T_Reply* r = reps[pid];

  r->rep().reply_size = -1;
  sz = Max_rep_size - sizeof(T_Reply_rep) - T_MAC_size;
  return r->contents() + sizeof(T_Reply_rep);
}

void
T_Rep_info::end_reply(int pid, Request_id rid, int sz)//, const Digest &d)
{
  T_Reply* r = reps[pid];
  th_assert(r->rep().reply_size == -1, "Invalid state");

  T_Reply_rep& rr = r->rep();
  rr.rid = rid;
  rr.reply_size = sz;
  rr.cid = -1;
  //rr.rh_digest = d;
  //rr.digest = Digest(r->contents() + sizeof(T_Reply_rep), sz);

  int old_size = sizeof(T_Reply_rep) + rr.reply_size;
  r->set_size(old_size + T_MAC_size);
  bzero(r->contents() + old_size, T_MAC_size);
}

void
T_Rep_info::send_reply(int dest_pid, View v, int src_pid, bool tentative)
{
  T_Reply *r = reps[dest_pid];
  T_Reply_rep& rr = r->rep();
  int old_size = sizeof(T_Reply_rep) + rr.reply_size;

  th_assert(rr.reply_size != -1, "Invalid state");
  th_assert(rr.extra == 0&& rr.v == 0&& rr.replica == 0&&rr.cid == -1,
      "Invalid state");

  rr.v = v;
  rr.replica = src_pid;
  rr.cid = -1;
  //T_my_principal->gen_mac(r->contents(), sizeof(T_Reply_rep), r->contents()
      //+ old_size, dest_pid);

  T_node->send(r, dest_pid);

  // Undo changes. To ensure state matches across all replicas.
  rr.v = 0;
  rr.replica = 0;
  rr.cid = -1;
  bzero(r->contents() + old_size, T_MAC_size);
}
