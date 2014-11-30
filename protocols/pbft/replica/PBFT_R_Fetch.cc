#include "th_assert.h"
#include "PBFT_R_Message_tags.h"
#include "PBFT_R_Partition.h"
#include "PBFT_R_Fetch.h"
#include "PBFT_R_Node.h"
#include "PBFT_R_Replica.h"
#include "PBFT_R_Principal.h"
#include "PBFT_R_State_defs.h"

PBFT_R_Fetch::PBFT_R_Fetch(Request_id rid, Seqno lu, int level, int index,
#ifndef NO_STATE_TRANSLATION
	     int chunkn,
#endif
	     Seqno rc, int repid) :
  PBFT_R_Message(PBFT_R_Fetch_tag, sizeof(PBFT_R_Fetch_rep) + PBFT_R_node->auth_size()) {
  rep().rid = rid;
  rep().lu = lu;
  rep().level = level;
  rep().index = index;
  rep().rc = rc;
  rep().repid = repid;
  rep().id = PBFT_R_node->id();
#ifndef NO_STATE_TRANSLATION
  rep().chunk_no = chunkn;
  rep().padding = 0;
#endif
  PBFT_R_node->gen_auth_in(contents(), sizeof(PBFT_R_Fetch_rep));
}

void PBFT_R_Fetch::re_authenticate(PBFT_R_Principal *p) {
  PBFT_R_node->gen_auth_in(contents(), sizeof(PBFT_R_Fetch_rep));
}

bool PBFT_R_Fetch::verify() {
  if (!PBFT_R_node->is_PBFT_R_replica(id())) 
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
  if (size()-(int)sizeof(PBFT_R_Fetch_rep) < PBFT_R_node->auth_size(id())) 
    return false;

  return PBFT_R_node->verify_auth_out(id(), contents(), sizeof(PBFT_R_Fetch_rep));
}


bool PBFT_R_Fetch::convert(PBFT_R_Message *m1, PBFT_R_Fetch  *&m2) {
  if (!m1->has_tag(PBFT_R_Fetch_tag, sizeof(PBFT_R_Fetch_rep)))
    return false;

  m2 = (PBFT_R_Fetch*)m1;
  m2->trim();
  return true;
}
 


