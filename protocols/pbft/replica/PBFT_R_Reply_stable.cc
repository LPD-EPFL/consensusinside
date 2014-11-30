#include "th_assert.h"
#include "PBFT_R_Message_tags.h"
#include "PBFT_R_Reply_stable.h"
#include "PBFT_R_Replica.h"
#include "PBFT_R_Principal.h"
 
PBFT_R_Reply_stable::PBFT_R_Reply_stable(Seqno lc, Seqno lp, int n, PBFT_R_Principal* p) :
  PBFT_R_Message(PBFT_R_Reply_stable_tag, sizeof(PBFT_R_Reply_stable_rep) + MAC_size) {
  rep().lc = lc;
  rep().lp = lp;
  rep().id = PBFT_R_node->id();
  rep().nonce = n;

  p->gen_mac_out(contents(), sizeof(PBFT_R_Reply_stable_rep), 
		 contents()+sizeof(PBFT_R_Reply_stable_rep));
}


void PBFT_R_Reply_stable::re_authenticate(PBFT_R_Principal *p) {
  p->gen_mac_out(contents(), sizeof(PBFT_R_Reply_stable_rep), 
		 contents()+sizeof(PBFT_R_Reply_stable_rep));
}


bool PBFT_R_Reply_stable::verify() {
  // PBFT_R_Reply_stables must be sent by PBFT_R_replicas.
  if (!PBFT_R_node->is_PBFT_R_replica(id())) return false;
  
  if (rep().lc%checkpoint_interval != 0 || rep().lc > rep().lp) return false;

  // Check size.
  if (size()-(int)sizeof(PBFT_R_Reply_stable_rep) < MAC_size) 
    return false;

  // Check MAC
  PBFT_R_Principal *p = PBFT_R_node->i_to_p(id());
  if (p) {
    return p->verify_mac_in(contents(), sizeof(PBFT_R_Reply_stable_rep), 
			    contents()+sizeof(PBFT_R_Reply_stable_rep));
  }

  return false;
}


bool PBFT_R_Reply_stable::convert(PBFT_R_Message *m1, PBFT_R_Reply_stable  *&m2) {
  if (!m1->has_tag(PBFT_R_Reply_stable_tag, sizeof(PBFT_R_Reply_stable_rep)))
    return false;
  m1->trim();
  m2 = (PBFT_R_Reply_stable*)m1;
  return true;
}
