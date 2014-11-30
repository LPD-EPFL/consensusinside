#include "th_assert.h"
#include "PBFT_C_Message_tags.h"
#include "PBFT_C_Prepare.h"
#include "PBFT_C_Node.h"
#include "PBFT_C_Replica.h"
#include "PBFT_C_Principal.h"

PBFT_C_Prepare::PBFT_C_Prepare(View v, Seqno s, Digest &d, PBFT_C_Principal* dst) :
  PBFT_C_Message(PBFT_C_Prepare_tag, sizeof(PBFT_C_Prepare_rep)
#ifndef USE_PKEY
	  + ((dst) ? MAC_size : PBFT_C_node->auth_size())) {
#else
          + ((dst) ? MAC_size : PBFT_C_node->sig_size())) {
#endif
    rep().extra = (dst) ? 1 : 0;
    rep().view = v;
    rep().seqno = s;
    rep().digest = d;
    rep().id = PBFT_C_node->id();
    rep().padding = 0;
    if (!dst) {
#ifndef USE_PKEY
    	PBFT_C_node->gen_auth_out(contents(), sizeof(PBFT_C_Prepare_rep));
#else
    	PBFT_C_node->gen_signature(contents(), sizeof(PBFT_C_Prepare_rep),
			  contents()+sizeof(PBFT_C_Prepare_rep));
#endif
    } else {
      dst->gen_mac_out(contents(), sizeof(PBFT_C_Prepare_rep),
		       contents()+sizeof(PBFT_C_Prepare_rep));
    }
}


void PBFT_C_Prepare::re_authenticate(PBFT_C_Principal *p) {
  if (rep().extra == 0) {
#ifndef USE_PKEY
	  PBFT_C_node->gen_auth_out(contents(), sizeof(PBFT_C_Prepare_rep));
#endif
  } else
    p->gen_mac_out(contents(), sizeof(PBFT_C_Prepare_rep),
		   contents()+sizeof(PBFT_C_Prepare_rep));
}


bool PBFT_C_Prepare::verify() {
  // This type of message should only be sent by a replica other than me
  // and different from the primary
  if (!PBFT_C_node->is_replica(id()) || id() == PBFT_C_node->id())
    return false;

  if (rep().extra == 0) {
    // Check signature size.
#ifndef USE_PKEY
    if (PBFT_C_replica->primary(view()) == id() ||
	size()-(int)sizeof(PBFT_C_Prepare_rep) < PBFT_C_node->auth_size(id()))
      return false;

    return PBFT_C_node->verify_auth_in(id(), contents(), sizeof(PBFT_C_Prepare_rep));
#else
    if (replica->primary(view()) == id() ||
	size()-(int)sizeof(PBFT_C_Prepare_rep) < node->sig_size(id()))
      return false;

    return node->i_to_p(id())->verify_signature(contents(), sizeof(PBFT_C_Prepare_rep),
						contents()+sizeof(PBFT_C_Prepare_rep));
#endif


  } else {
    if (size()-(int)sizeof(PBFT_C_Prepare_rep) < MAC_size)
      return false;

    return PBFT_C_node->i_to_p(id())->verify_mac_in(contents(), sizeof(PBFT_C_Prepare_rep),
				      contents()+sizeof(PBFT_C_Prepare_rep));
  }

  return false;
}


bool PBFT_C_Prepare::convert(PBFT_C_Message *m1, PBFT_C_Prepare  *&m2) {
  if (!m1->has_tag(PBFT_C_Prepare_tag, sizeof(PBFT_C_Prepare_rep)))
    return false;

  m2 = (PBFT_C_Prepare*)m1;
  m2->trim();
  return true;
}



