#include <stdlib.h>
#include <string.h>

#include "PBFT_C_Rep_info.h"
#include "PBFT_C_Replica.h"
#include "PBFT_C_Reply.h"
#include "PBFT_C_Req_queue.h"

#include "PBFT_C_Statistics.h"
#include "PBFT_C_State_defs.h"

#include "Array.h"

#ifndef NO_STATE_TRANSLATION

PBFT_C_Rep_info::PBFT_C_Rep_info(int n) {
  th_assert(n != 0, "Invalid argument");

  nps = n;
  mem = (char *)valloc(size());

#else

PBFT_C_Rep_info::PBFT_C_Rep_info(char *m, int sz, int n) {
  th_assert(n != 0, "Invalid argument");

  nps = n;
  mem = m;

  if (sz < (nps+1)*Max_rep_size)
    th_fail("Memory is too small to hold replies for all principals");

#endif

  int old_nps = *((Long*)mem);
  if (old_nps != 0) {
    // Memory has already been initialized.
    if (nps != old_nps)
      th_fail("Changing number of principals. Not implemented yet");
  } else {
    // Initialize memory.
    bzero(mem, (nps+1)*Max_rep_size);
    for (int i=0; i < nps; i++) {
      // Wasting first page just to store the number of principals.
      PBFT_C_Reply_rep* rr = (PBFT_C_Reply_rep*)(mem+(i+1)*Max_rep_size);
      rr->tag = PBFT_C_Reply_tag;
      rr->reply_size = -1;
      rr->rid = 0;
    }
    *((Long*)mem) = nps;
  }

  struct Rinfo ri;
  ri.tentative = true;
  ri.lsent = zeroPBFT_C_Time();

  for (int i=0; i < nps; i++) {
    PBFT_C_Reply_rep *rr = (PBFT_C_Reply_rep*)(mem+(i+1)*Max_rep_size);
    th_assert(rr->tag == PBFT_C_Reply_tag, "Corrupt memory");
    reps.append(new PBFT_C_Reply(rr));
    ireps.append(ri);
  }
}


PBFT_C_Rep_info::~PBFT_C_Rep_info() {
  for (int i=0; i < nps; i++)
    delete reps[i];
}


char* PBFT_C_Rep_info::new_reply(int pid, int &sz) {
  PBFT_C_Reply* r = reps[pid];

#ifndef NO_STATE_TRANSLATION
  for(int i=(r->contents()-mem)/Block_size;
      i<=(r->contents()+Max_rep_size-1-mem)/Block_size;i++) {
    //    fprintf(stderr,"modifying reply: mem %d begin %d end %d \t page %d size %d\n",mem, r->contents(), r->contents()+Max_rep_size-1, i,size());
    replica->modify_index_replies(i);
  }
#else
  PBFT_C_replica->modify(r->contents(), Max_rep_size);
#endif
  ireps[pid].tentative = true;
  ireps[pid].lsent = zeroPBFT_C_Time();
  r->rep().reply_size = -1;
  sz = Max_rep_size-sizeof(PBFT_C_Reply_rep)-MAC_size;
  return r->contents()+sizeof(PBFT_C_Reply_rep);
}


void PBFT_C_Rep_info::end_reply(int pid, Request_id rid, int sz) {
  PBFT_C_Reply* r = reps[pid];
  th_assert(r->rep().reply_size == -1, "Invalid state");

  PBFT_C_Reply_rep& rr = r->rep();
  rr.rid = rid;
  rr.reply_size = sz;
  rr.digest = Digest(r->contents()+sizeof(PBFT_C_Reply_rep), sz);

  int old_size = sizeof(PBFT_C_Reply_rep)+rr.reply_size;
  r->set_size(old_size+MAC_size);
  bzero(r->contents()+old_size, MAC_size);
}

void PBFT_C_Rep_info::send_reply(int pid, View v, int id, bool tentative) {
  PBFT_C_Reply *r = reps[pid];
  PBFT_C_Reply_rep& rr = r->rep();
  int old_size = sizeof(PBFT_C_Reply_rep)+rr.reply_size;

  th_assert(rr.reply_size != -1, "Invalid state");
  th_assert(rr.extra == 0 && rr.v == 0 && rr.replica == 0, "Invalid state");

  if (!tentative && ireps[pid].tentative) {
    ireps[pid].tentative = false;
    ireps[pid].lsent = zeroPBFT_C_Time();
  }

  PBFT_C_Time cur;
  PBFT_C_Time& lsent = ireps[pid].lsent;
  if (lsent != 0) {
    cur = currentPBFT_C_Time();
    if (diffPBFT_C_Time(cur, lsent) <= 10000)
      return;

    lsent = cur;
  }

  if (ireps[pid].tentative) rr.extra = 1;
  rr.v = v;
  rr.replica = id;
  PBFT_C_Principal *p = PBFT_C_node->i_to_p(pid);

  INCR_OP(reply_auth);
  START_CC(reply_auth_cycles);
  p->gen_mac_out(r->contents(), sizeof(PBFT_C_Reply_rep), r->contents()+old_size);
  STOP_CC(reply_auth_cycles);

  PBFT_C_node->send(r, pid);

  // Undo changes. To ensure state matches across all replicas.
  rr.extra = 0;
  rr.v = 0;
  rr.replica = 0;
  bzero(r->contents()+old_size, MAC_size);
}


bool PBFT_C_Rep_info::new_state(PBFT_C_Req_queue *rset) {
  bool first=false;
  for (int i=0; i < nps; i++) {
    commit_reply(i);

    // Remove requests from rset with stale timestamps.
    if (rset->remove(i, req_id(i)))
      first = true;
  }
  return first;
}
