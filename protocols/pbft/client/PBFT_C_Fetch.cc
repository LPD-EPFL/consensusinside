#include "th_assert.h"
#include "PBFT_C_Message_tags.h"
#include "Partition.h"
#include "PBFT_C_Fetch.h"
#include "PBFT_C_Node.h"
#include "PBFT_C_Replica.h"
#include "PBFT_C_Principal.h"
#include "PBFT_C_State_defs.h"

PBFT_C_Fetch::PBFT_C_Fetch(Request_id rid, Seqno lu, int level, int index,
#ifndef NO_STATE_TRANSLATION
	     int chunkn,
#endif
	     Seqno rc, int repid) :
  PBFT_C_Message(PBFT_C_Fetch_tag, sizeof(PBFT_C_Fetch_rep) + PBFT_C_node->auth_size()) {
  rep().rid = rid;
  rep().lu = lu;
  rep().level = level;
  rep().index = index;
  rep().rc = rc;
  rep().repid = repid;
  rep().id = PBFT_C_node->id();
#ifndef NO_STATE_TRANSLATION
  rep().chunk_no = chunkn;
  rep().padding = 0;
#endif
  PBFT_C_node->gen_auth_in(contents(), sizeof(PBFT_C_Fetch_rep));
}

void PBFT_C_Fetch::re_authenticate(PBFT_C_Principal *p) {
	PBFT_C_node->gen_auth_in(contents(), sizeof(PBFT_C_Fetch_rep));
}

bool PBFT_C_Fetch::verify() {
  if (!PBFT_C_node->is_replica(id()))
    return false;

  if (level() < 0 || level() >= PLevels)
    return false;

  if (index() < 0 || index() >=  PLevelSize[level()])
    return false;

  if (checkpoint() == -1 && replier() != -1)
    return false;

#ifndef NO_STATE_TRANSLATION
  if (chunk_number() < 0)
    return false;
#endif

  // Check signature size.
  if (size()-(int)sizeof(PBFT_C_Fetch_rep) < PBFT_C_node->auth_size(id()))
    return false;

  return PBFT_C_node->verify_auth_out(id(), contents(), sizeof(PBFT_C_Fetch_rep));
}


bool PBFT_C_Fetch::convert(PBFT_C_Message *m1, PBFT_C_Fetch  *&m2) {
  if (!m1->has_tag(PBFT_C_Fetch_tag, sizeof(PBFT_C_Fetch_rep)))
    return false;

  m2 = (PBFT_C_Fetch*)m1;
  m2->trim();
  return true;
}



