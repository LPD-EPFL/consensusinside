#include "th_assert.h"
#include "PBFT_C_Message_tags.h"
#include "PBFT_C_Reply_stable.h"
#include "PBFT_C_Replica.h"
#include "PBFT_C_Principal.h"

PBFT_C_Reply_stable::PBFT_C_Reply_stable(Seqno lc, Seqno lp, int n, PBFT_C_Principal* p) :
  PBFT_C_Message(PBFT_C_Reply_stable_tag, sizeof(PBFT_C_Reply_stable_rep) + MAC_size) {
  rep().lc = lc;
  rep().lp = lp;
  rep().id = PBFT_C_node->id();
  rep().nonce = n;

  p->gen_mac_out(contents(), sizeof(PBFT_C_Reply_stable_rep),
		 contents()+sizeof(PBFT_C_Reply_stable_rep));
}


void PBFT_C_Reply_stable::re_authenticate(PBFT_C_Principal *p) {
  p->gen_mac_out(contents(), sizeof(PBFT_C_Reply_stable_rep),
		 contents()+sizeof(PBFT_C_Reply_stable_rep));
}


bool PBFT_C_Reply_stable::verify() {
  // PBFT_C_Reply_stables must be sent by replicas.
  if (!PBFT_C_node->is_replica(id())) return false;

  if (rep().lc%checkpoint_interval != 0 || rep().lc > rep().lp) return false;

  // Check size.
  if (size()-(int)sizeof(PBFT_C_Reply_stable_rep) < MAC_size)
    return false;

  // Check MAC
  PBFT_C_Principal *p = PBFT_C_node->i_to_p(id());
  if (p) {
    return p->verify_mac_in(contents(), sizeof(PBFT_C_Reply_stable_rep),
			    contents()+sizeof(PBFT_C_Reply_stable_rep));
  }

  return false;
}


bool PBFT_C_Reply_stable::convert(PBFT_C_Message *m1, PBFT_C_Reply_stable  *&m2) {
  if (!m1->has_tag(PBFT_C_Reply_stable_tag, sizeof(PBFT_C_Reply_stable_rep)))
    return false;
  m1->trim();
  m2 = (PBFT_C_Reply_stable*)m1;
  return true;
}
