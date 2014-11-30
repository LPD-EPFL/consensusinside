#include "th_assert.h"
#include "PBFT_R_Message_tags.h"
#include "PBFT_R_Checkpoint.h"
#include "PBFT_R_Replica.h"
#include "PBFT_R_Principal.h"
 
PBFT_R_Checkpoint::PBFT_R_Checkpoint(Seqno s, Digest &d, bool stable) : 
#ifndef USE_PKEY
  PBFT_R_Message(PBFT_R_Checkpoint_tag, sizeof(PBFT_R_Checkpoint_rep) + PBFT_R_node->auth_size()) {
#else
  PBFT_R_Message(PBFT_R_Checkpoint_tag, sizeof(PBFT_R_Checkpoint_rep) + PBFT_R_node->sig_size()) {
#endif
    rep().extra = (stable) ? 1 : 0;
    rep().seqno = s;
    rep().digest = d;
    rep().id = PBFT_R_node->id();
    rep().padding = 0;

#ifndef USE_PKEY
    PBFT_R_node->gen_auth_out(contents(), sizeof(PBFT_R_Checkpoint_rep));
#else
    PBFT_R_node->gen_signature(contents(), sizeof(PBFT_R_Checkpoint_rep), 
		      contents()+sizeof(PBFT_R_Checkpoint_rep));
#endif
}

void PBFT_R_Checkpoint::re_authenticate(PBFT_R_Principal *p, bool stable) {
#ifndef USE_PKEY
  if (stable) rep().extra = 1;
  PBFT_R_node->gen_auth_out(contents(), sizeof(PBFT_R_Checkpoint_rep));
#else
  if (rep().extra != 1 && stable) {
    rep().extra = 1;
    PBFT_R_node->gen_signature(contents(), sizeof(PBFT_R_Checkpoint_rep), 
			contents()+sizeof(PBFT_R_Checkpoint_rep));
  }
#endif
}

bool PBFT_R_Checkpoint::verify() {
  // PBFT_R_Checkpoints must be sent by PBFT_R_replicas.
  if (!PBFT_R_node->is_PBFT_R_replica(id())) return false;
  
  // Check signature size.
#ifndef USE_PKEY
  if (size()-(int)sizeof(PBFT_R_Checkpoint_rep) < PBFT_R_node->auth_size(id())) 
    return false;

  return PBFT_R_node->verify_auth_in(id(), contents(), sizeof(PBFT_R_Checkpoint_rep));
#else
  if (size()-(int)sizeof(PBFT_R_Checkpoint_rep) < PBFT_R_node->sig_size(id())) 
    return false;

  return PBFT_R_node->i_to_p(id())->verify_signature(contents(), sizeof(PBFT_R_Checkpoint_rep),
					      contents()+sizeof(PBFT_R_Checkpoint_rep));
#endif
}

bool PBFT_R_Checkpoint::convert(PBFT_R_Message *m1, PBFT_R_Checkpoint  *&m2) {
  if (!m1->has_tag(PBFT_R_Checkpoint_tag, sizeof(PBFT_R_Checkpoint_rep)))
    return false;
  m1->trim();
  m2 = (PBFT_R_Checkpoint*)m1;
  return true;
}
