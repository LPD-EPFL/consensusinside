#include "th_assert.h"
#include "PBFT_C_Message_tags.h"
#include "PBFT_C_Checkpoint.h"
#include "PBFT_C_Replica.h"
#include "PBFT_C_Principal.h"

PBFT_C_Checkpoint::PBFT_C_Checkpoint(Seqno s, Digest &d, bool stable) :
#ifndef USE_PKEY
  PBFT_C_Message(PBFT_C_Checkpoint_tag, sizeof(PBFT_C_Checkpoint_rep) + PBFT_C_node->auth_size()) {
#else
  PBFT_C_Message(PBFT_C_Checkpoint_tag, sizeof(PBFT_C_Checkpoint_rep) + PBFT_C_node->sig_size()) {
#endif
    rep().extra = (stable) ? 1 : 0;
    rep().seqno = s;
    rep().digest = d;
    rep().id = PBFT_C_node->id();
    rep().padding = 0;

#ifndef USE_PKEY
    PBFT_C_node->gen_auth_out(contents(), sizeof(PBFT_C_Checkpoint_rep));
#else
    PBFT_C_node->gen_signature(contents(), sizeof(PBFT_C_Checkpoint_rep),
		      contents()+sizeof(PBFT_C_Checkpoint_rep));
#endif
}

void PBFT_C_Checkpoint::re_authenticate(PBFT_C_Principal *p, bool stable) {
#ifndef USE_PKEY
  if (stable) rep().extra = 1;
  PBFT_C_node->gen_auth_out(contents(), sizeof(PBFT_C_Checkpoint_rep));
#else
  if (rep().extra != 1 && stable) {
    rep().extra = 1;
    node->gen_signature(contents(), sizeof(PBFT_C_Checkpoint_rep),
			contents()+sizeof(PBFT_C_Checkpoint_rep));
  }
#endif
}

bool PBFT_C_Checkpoint::verify() {
  // PBFT_C_Checkpoints must be sent by replicas.
  if (!PBFT_C_node->is_replica(id())) return false;

  // Check signature size.
#ifndef USE_PKEY
  if (size()-(int)sizeof(PBFT_C_Checkpoint_rep) < PBFT_C_node->auth_size(id()))
    return false;

  return PBFT_C_node->verify_auth_in(id(), contents(), sizeof(PBFT_C_Checkpoint_rep));
#else
  if (size()-(int)sizeof(PBFT_C_Checkpoint_rep) < PBFT_C_node->sig_size(id()))
    return false;

  return PBFT_C_node->i_to_p(id())->verify_signature(contents(), sizeof(PBFT_C_Checkpoint_rep),
					      contents()+sizeof(PBFT_C_Checkpoint_rep));
#endif
}

bool PBFT_C_Checkpoint::convert(PBFT_C_Message *m1, PBFT_C_Checkpoint  *&m2) {
  if (!m1->has_tag(PBFT_C_Checkpoint_tag, sizeof(PBFT_C_Checkpoint_rep)))
    return false;
  m1->trim();
  m2 = (PBFT_C_Checkpoint*)m1;
  return true;
}
