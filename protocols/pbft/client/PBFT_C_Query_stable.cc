#include "th_assert.h"
#include "PBFT_C_Message_tags.h"
#include "PBFT_C_Query_stable.h"
#include "PBFT_C_Replica.h"
#include "PBFT_C_Principal.h"

PBFT_C_Query_stable::PBFT_C_Query_stable() :
  PBFT_C_Message(PBFT_C_Query_stable_tag, sizeof(PBFT_C_Query_stable_rep) + PBFT_C_node->auth_size()) {
  rep().id = PBFT_C_node->id();
  rep().nonce = random_int();
  PBFT_C_node->gen_auth_out(contents(), sizeof(PBFT_C_Query_stable_rep));
}

void PBFT_C_Query_stable::re_authenticate(PBFT_C_Principal *p) {
	PBFT_C_node->gen_auth_out(contents(), sizeof(PBFT_C_Query_stable_rep));
}

bool PBFT_C_Query_stable::verify() {
  // PBFT_C_Query_stables must be sent by replicas.
  if (!PBFT_C_node->is_replica(id())) return false;

  // Check signature size.
  if (size()-(int)sizeof(PBFT_C_Query_stable_rep) < PBFT_C_node->auth_size(id()))
    return false;

  return PBFT_C_node->verify_auth_in(id(), contents(), sizeof(PBFT_C_Query_stable_rep));
}

bool PBFT_C_Query_stable::convert(PBFT_C_Message *m1, PBFT_C_Query_stable  *&m2) {
  if (!m1->has_tag(PBFT_C_Query_stable_tag, sizeof(PBFT_C_Query_stable_rep)))
    return false;
  m1->trim();
  m2 = (PBFT_C_Query_stable*)m1;
  return true;
}
