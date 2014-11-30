#include "th_assert.h"
#include "PBFT_C_Message_tags.h"
#include "PBFT_C_View_change_ack.h"
#include "PBFT_C_Node.h"
#include "PBFT_C_Principal.h"

PBFT_C_View_change_ack::PBFT_C_View_change_ack(View v, int id, int vcid, Digest const &vcd) :
  PBFT_C_Message(PBFT_C_View_change_ack_tag, sizeof(PBFT_C_View_change_ack_rep) + MAC_size) {
    rep().v = v;
    rep().id = PBFT_C_node->id();
    rep().vcid = vcid;
    rep().vcd = vcd;

    int old_size = sizeof(PBFT_C_View_change_ack_rep);
    set_size(old_size+MAC_size);
    PBFT_C_Principal *p = PBFT_C_node->i_to_p(PBFT_C_node->primary(v));
    p->gen_mac_out(contents(), old_size, contents()+old_size);
}

void PBFT_C_View_change_ack::re_authenticate(PBFT_C_Principal *p) {
  p->gen_mac_out(contents(), sizeof(PBFT_C_View_change_ack_rep), contents()+sizeof(PBFT_C_View_change_ack_rep));
}

bool PBFT_C_View_change_ack::verify() {
  // These messages must be sent by replicas other than me, the replica that sent the
  // corresponding view-change, or the primary.
  if (!PBFT_C_node->is_replica(id()) || id() == PBFT_C_node->id()
      || id() == vc_id() || PBFT_C_node->primary(view()) == id())
    return false;

  if (view() <= 0 || !PBFT_C_node->is_replica(vc_id()))
    return false;

  // Check sizes
  if (size()-(int)sizeof(PBFT_C_View_change_ack) < MAC_size)
    return false;

  // Check MAC.
  PBFT_C_Principal *p = PBFT_C_node->i_to_p(id());
  int old_size = sizeof(PBFT_C_View_change_ack_rep);
  if (!p->verify_mac_in(contents(), old_size, contents()+old_size))
    return false;

  return true;
}


bool PBFT_C_View_change_ack::convert(PBFT_C_Message *m1, PBFT_C_View_change_ack  *&m2) {
  if (!m1->has_tag(PBFT_C_View_change_ack_tag, sizeof(PBFT_C_View_change_ack_rep)))
    return false;

  m2 = (PBFT_C_View_change_ack*)m1;
  m2->trim();
  return true;
}
