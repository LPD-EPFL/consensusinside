#include "th_assert.h"
#include "PBFT_R_Message_tags.h"
#include "PBFT_R_View_change_ack.h"
#include "PBFT_R_Node.h"
#include "PBFT_R_Principal.h"

PBFT_R_View_change_ack::PBFT_R_View_change_ack(View v, int id, int vcid, Digest const &vcd) :
  PBFT_R_Message(PBFT_R_View_change_ack_tag, sizeof(PBFT_R_View_change_ack_rep) + MAC_size) {
    rep().v = v;
    rep().id = PBFT_R_node->id();
    rep().vcid = vcid;
    rep().vcd = vcd;
    
    int old_size = sizeof(PBFT_R_View_change_ack_rep);
    set_size(old_size+MAC_size);
    PBFT_R_Principal *p = PBFT_R_node->i_to_p(PBFT_R_node->primary(v));
    p->gen_mac_out(contents(), old_size, contents()+old_size);
}

void PBFT_R_View_change_ack::re_authenticate(PBFT_R_Principal *p) {
  p->gen_mac_out(contents(), sizeof(PBFT_R_View_change_ack_rep), contents()+sizeof(PBFT_R_View_change_ack_rep));
}

bool PBFT_R_View_change_ack::verify() {
  // These messages must be sent by PBFT_R_replicas other than me, the PBFT_R_replica that sent the 
  // corresponding view-change, or the primary.
  if (!PBFT_R_node->is_PBFT_R_replica(id()) || id() == PBFT_R_node->id() 
      || id() == vc_id() || PBFT_R_node->primary(view()) == id())
    return false;

  if (view() <= 0 || !PBFT_R_node->is_PBFT_R_replica(vc_id()))
    return false;

  // Check sizes
  if (size()-(int)sizeof(PBFT_R_View_change_ack) < MAC_size) 
    return false;

  // Check MAC.
  PBFT_R_Principal *p = PBFT_R_node->i_to_p(id());
  int old_size = sizeof(PBFT_R_View_change_ack_rep);
  if (!p->verify_mac_in(contents(), old_size, contents()+old_size))
    return false;

  return true;
}


bool PBFT_R_View_change_ack::convert(PBFT_R_Message *m1, PBFT_R_View_change_ack  *&m2) {
  if (!m1->has_tag(PBFT_R_View_change_ack_tag, sizeof(PBFT_R_View_change_ack_rep)))
    return false;

  m2 = (PBFT_R_View_change_ack*)m1;
  m2->trim();
  return true;
}
