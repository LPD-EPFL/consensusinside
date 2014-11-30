#include "th_assert.h"
#include "PBFT_R_Message_tags.h"
#include "PBFT_R_Prepare.h"
#include "PBFT_R_Node.h"
#include "PBFT_R_Replica.h"
#include "PBFT_R_Principal.h"

PBFT_R_Prepare::PBFT_R_Prepare(View v, Seqno s, Digest &d, PBFT_R_Principal* dst) :
  PBFT_R_Message(PBFT_R_Prepare_tag, sizeof(PBFT_R_Prepare_rep)
#ifndef USE_PKEY
	  + ((dst) ? MAC_size : PBFT_R_node->auth_size())) {
#else
          + ((dst) ? MAC_size : PBFT_R_node->sig_size())) {
#endif
    rep().extra = (dst) ? 1 : 0;
    rep().view = v;
    rep().seqno = s;
    rep().digest = d;
    rep().id = PBFT_R_node->id();
    rep().padding = 0;
    if (!dst) {
#ifndef USE_PKEY
      PBFT_R_node->gen_auth_out(contents(), sizeof(PBFT_R_Prepare_rep));
#else
      PBFT_R_node->gen_signature(contents(), sizeof(PBFT_R_Prepare_rep),
			  contents()+sizeof(PBFT_R_Prepare_rep));
#endif
    } else {
      dst->gen_mac_out(contents(), sizeof(PBFT_R_Prepare_rep),
		       contents()+sizeof(PBFT_R_Prepare_rep));
    }
}


void PBFT_R_Prepare::re_authenticate(PBFT_R_Principal *p) {
  if (rep().extra == 0) {
#ifndef USE_PKEY
    PBFT_R_node->gen_auth_out(contents(), sizeof(PBFT_R_Prepare_rep));
#endif
  } else
    p->gen_mac_out(contents(), sizeof(PBFT_R_Prepare_rep),
		   contents()+sizeof(PBFT_R_Prepare_rep));
}


bool PBFT_R_Prepare::verify() {
  // This type of message should only be sent by a PBFT_R_replica other than me
  // and different from the primary
  if (!PBFT_R_node->is_PBFT_R_replica(id()) || id() == PBFT_R_node->id())
    return false;

  if (rep().extra == 0) {
    // Check signature size.
#ifndef USE_PKEY
    if (PBFT_R_replica->primary(view()) == id() ||
	size()-(int)sizeof(PBFT_R_Prepare_rep) < PBFT_R_node->auth_size(id()))
      return false;

    return PBFT_R_node->verify_auth_in(id(), contents(), sizeof(PBFT_R_Prepare_rep));
#else
    if (PBFT_R_replica->primary(view()) == id() ||
	size()-(int)sizeof(PBFT_R_Prepare_rep) < PBFT_R_node->sig_size(id()))
      return false;

    return PBFT_R_node->i_to_p(id())->verify_signature(contents(), sizeof(PBFT_R_Prepare_rep),
						contents()+sizeof(PBFT_R_Prepare_rep));
#endif


  } else {
    if (size()-(int)sizeof(PBFT_R_Prepare_rep) < MAC_size)
      return false;

    return PBFT_R_node->i_to_p(id())->verify_mac_in(contents(), sizeof(PBFT_R_Prepare_rep),
				      contents()+sizeof(PBFT_R_Prepare_rep));
  }

  return false;
}


bool PBFT_R_Prepare::convert(PBFT_R_Message *m1, PBFT_R_Prepare  *&m2) {
  if (!m1->has_tag(PBFT_R_Prepare_tag, sizeof(PBFT_R_Prepare_rep)))
    return false;

  m2 = (PBFT_R_Prepare*)m1;
  m2->trim();
  return true;
}



