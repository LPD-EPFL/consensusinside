#include "th_assert.h"
#include "PBFT_R_Message_tags.h"
#include "PBFT_R_Commit.h"
#include "PBFT_R_Node.h"
#include "PBFT_R_Replica.h"
#include "PBFT_R_Principal.h"

PBFT_R_Commit::PBFT_R_Commit(View v, Seqno s) : 
  PBFT_R_Message(PBFT_R_Commit_tag, sizeof(PBFT_R_Commit_rep) + PBFT_R_node->auth_size()) {
    rep().view = v;
    rep().seqno = s;
    rep().id = PBFT_R_node->id(); 
    rep().padding = 0;
    PBFT_R_node->gen_auth_out(contents(), sizeof(PBFT_R_Commit_rep));
}


PBFT_R_Commit::PBFT_R_Commit(PBFT_R_Commit_rep *contents) : PBFT_R_Message(contents) {}

void PBFT_R_Commit::re_authenticate(PBFT_R_Principal *p) {
  PBFT_R_node->gen_auth_out(contents(), sizeof(PBFT_R_Commit_rep));
}

bool PBFT_R_Commit::verify() {
  // PBFT_R_Commits must be sent by PBFT_R_replicas.
  if (!PBFT_R_node->is_PBFT_R_replica(id()) || id() == PBFT_R_node->id()) return false;

  // Check signature size.
  if (size()-(int)sizeof(PBFT_R_Commit_rep) < PBFT_R_node->auth_size(id())) 
    return false;

  return PBFT_R_node->verify_auth_in(id(), contents(), sizeof(PBFT_R_Commit_rep));
}


bool PBFT_R_Commit::convert(PBFT_R_Message *m1, PBFT_R_Commit  *&m2) {
  if (!m1->has_tag(PBFT_R_Commit_tag, sizeof(PBFT_R_Commit_rep)))
    return false;

  m2 = (PBFT_R_Commit*)m1;
  m2->trim();
  return true;
}

bool PBFT_R_Commit::convert(char *m1, unsigned max_len, PBFT_R_Commit &m2) {
  // First check if we can use m1 to create a PBFT_R_Commit.
  if (!PBFT_R_Message::convert(m1, max_len, PBFT_R_Commit_tag, sizeof(PBFT_R_Commit_rep),m2)) 
    return false;
  return true;
}
 
