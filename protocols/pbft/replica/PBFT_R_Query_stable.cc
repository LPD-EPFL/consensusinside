#include "th_assert.h"
#include "PBFT_R_Message_tags.h"
#include "PBFT_R_Query_stable.h"
#include "PBFT_R_Replica.h"
#include "PBFT_R_Principal.h"
 
PBFT_R_Query_stable::PBFT_R_Query_stable() : 
  PBFT_R_Message(PBFT_R_Query_stable_tag, sizeof(PBFT_R_Query_stable_rep) + PBFT_R_node->auth_size()) {
  rep().id = PBFT_R_node->id();
  rep().nonce = PBFT_PBFT_R_random_int();
  PBFT_R_node->gen_auth_out(contents(), sizeof(PBFT_R_Query_stable_rep));
}

void PBFT_R_Query_stable::re_authenticate(PBFT_R_Principal *p) {
  PBFT_R_node->gen_auth_out(contents(), sizeof(PBFT_R_Query_stable_rep));
}

bool PBFT_R_Query_stable::verify() {
  // PBFT_R_Query_stables must be sent by PBFT_R_replicas.
  if (!PBFT_R_node->is_PBFT_R_replica(id())) return false;
  
  // Check signature size.
  if (size()-(int)sizeof(PBFT_R_Query_stable_rep) < PBFT_R_node->auth_size(id())) 
    return false;

  return PBFT_R_node->verify_auth_in(id(), contents(), sizeof(PBFT_R_Query_stable_rep));
}

bool PBFT_R_Query_stable::convert(PBFT_R_Message *m1, PBFT_R_Query_stable  *&m2) {
  if (!m1->has_tag(PBFT_R_Query_stable_tag, sizeof(PBFT_R_Query_stable_rep)))
    return false;
  m1->trim();
  m2 = (PBFT_R_Query_stable*)m1;
  return true;
}
