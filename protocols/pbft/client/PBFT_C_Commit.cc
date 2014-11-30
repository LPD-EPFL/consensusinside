#include "th_assert.h"
#include "PBFT_C_Message_tags.h"
#include "PBFT_C_Commit.h"
#include "PBFT_C_Node.h"
#include "PBFT_C_Replica.h"
#include "PBFT_C_Principal.h"

PBFT_C_Commit::PBFT_C_Commit(View v, Seqno s) :
  PBFT_C_Message(PBFT_C_Commit_tag, sizeof(PBFT_C_Commit_rep) + PBFT_C_node->auth_size()) {
    rep().view = v;
    rep().seqno = s;
    rep().id = PBFT_C_node->id();
    rep().padding = 0;
    PBFT_C_node->gen_auth_out(contents(), sizeof(PBFT_C_Commit_rep));
}


PBFT_C_Commit::PBFT_C_Commit(PBFT_C_Commit_rep *contents) : PBFT_C_Message(contents) {}

void PBFT_C_Commit::re_authenticate(PBFT_C_Principal *p) {
	PBFT_C_node->gen_auth_out(contents(), sizeof(PBFT_C_Commit_rep));
}

bool PBFT_C_Commit::verify() {
  // PBFT_C_Commits must be sent by replicas.
  if (!PBFT_C_node->is_replica(id()) || id() == PBFT_C_node->id()) return false;

  // Check signature size.
  if (size()-(int)sizeof(PBFT_C_Commit_rep) < PBFT_C_node->auth_size(id()))
    return false;

  return PBFT_C_node->verify_auth_in(id(), contents(), sizeof(PBFT_C_Commit_rep));
}


bool PBFT_C_Commit::convert(PBFT_C_Message *m1, PBFT_C_Commit  *&m2) {
  if (!m1->has_tag(PBFT_C_Commit_tag, sizeof(PBFT_C_Commit_rep)))
    return false;

  m2 = (PBFT_C_Commit*)m1;
  m2->trim();
  return true;
}

bool PBFT_C_Commit::convert(char *m1, unsigned max_len, PBFT_C_Commit &m2) {
  // First check if we can use m1 to create a PBFT_C_Commit.
  if (!PBFT_C_Message::convert(m1, max_len, PBFT_C_Commit_tag, sizeof(PBFT_C_Commit_rep),m2))
    return false;
  return true;
}

