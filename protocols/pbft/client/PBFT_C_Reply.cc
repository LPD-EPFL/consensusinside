#include <strings.h>
#include "th_assert.h"
#include "PBFT_C_Message_tags.h"
#include "PBFT_C_Reply.h"
#include "PBFT_C_Node.h"
#include "PBFT_C_Principal.h"

#include "PBFT_C_Statistics.h"

PBFT_C_Reply::PBFT_C_Reply(View view, Request_id req, int replica) :
  PBFT_C_Message(PBFT_C_Reply_tag, Max_message_size) {
    rep().v = view;
    rep().rid = req;
    rep().replica = replica;
    rep().reply_size = 0;
    set_size(sizeof(PBFT_C_Reply_rep));
}


PBFT_C_Reply::PBFT_C_Reply(PBFT_C_Reply_rep *r) : PBFT_C_Message(r) {}


PBFT_C_Reply::PBFT_C_Reply(View view, Request_id req, int replica, Digest &d,
	     PBFT_C_Principal *p, bool tentative) :
  PBFT_C_Message(PBFT_C_Reply_tag, sizeof(PBFT_C_Reply_rep)+MAC_size) {

	if (tentative) rep().extra = 1;
	else rep().extra = 0;

    rep().v = view;
    rep().rid = req;
    rep().replica = replica;
    rep().reply_size = -1;
    rep().digest = d;

    INCR_OP(reply_auth);
    START_CC(reply_auth_cycles);
    p->gen_mac_out(contents(), sizeof(PBFT_C_Reply_rep), contents()+sizeof(PBFT_C_Reply_rep));
    STOP_CC(reply_auth_cycles);
}


PBFT_C_Reply* PBFT_C_Reply::copy(int id) const {
  PBFT_C_Reply* ret = (PBFT_C_Reply*)new PBFT_C_Message(msg->size);
  memcpy(ret->msg, msg, msg->size);
  ret->rep().replica = id;
  return ret;
}


char *PBFT_C_Reply::store_reply(int &max_len) {
  max_len = msize()-sizeof(PBFT_C_Reply_rep)-MAC_size;
  return contents()+sizeof(PBFT_C_Reply_rep);
}


void PBFT_C_Reply::authenticate(PBFT_C_Principal *p, int act_len, bool tentative) {
  th_assert((unsigned)act_len <= msize()-sizeof(PBFT_C_Reply_rep)-MAC_size,
	    "Invalid reply size");

  if (tentative) rep().extra = 1;

  rep().reply_size = act_len;
  rep().digest = Digest(contents()+sizeof(PBFT_C_Reply_rep), act_len);
  int old_size = sizeof(PBFT_C_Reply_rep)+act_len;
  set_size(old_size+MAC_size);

  INCR_OP(reply_auth);
  START_CC(reply_auth_cycles);
  p->gen_mac_out(contents(), sizeof(PBFT_C_Reply_rep), contents()+old_size);
  STOP_CC(reply_auth_cycles);

  trim();
}


void PBFT_C_Reply::re_authenticate(PBFT_C_Principal *p) {
  int old_size = sizeof(PBFT_C_Reply_rep)+rep().reply_size;

  INCR_OP(reply_auth);
  START_CC(reply_auth_cycles);
  p->gen_mac_out(contents(), sizeof(PBFT_C_Reply_rep), contents()+old_size);
  STOP_CC(reply_auth_cycles);
}


void PBFT_C_Reply::commit(PBFT_C_Principal *p) {
  if (rep().extra == 0) return; // PBFT_C_Reply is already committed.

  rep().extra = 0;
  int old_size = sizeof(PBFT_C_Reply_rep)+rep().reply_size;
  p->gen_mac_out(contents(), sizeof(PBFT_C_Reply_rep), contents()+old_size);
}


bool PBFT_C_Reply::verify() {
  // Replies must be sent by replicas.
  if (!PBFT_C_node->is_replica(id()))
    return false;

  // Check sizes
  int rep_size = (full()) ? rep().reply_size : 0;
  if (size()-(int)sizeof(PBFT_C_Reply_rep)-rep_size < MAC_size)
    return false;

  // Check reply
  if (full()) {
    Digest d(contents()+sizeof(PBFT_C_Reply_rep), rep_size);
    if (d != rep().digest)
      return false;
  }

  // Check signature.
  PBFT_C_Principal *replica = PBFT_C_node->i_to_p(rep().replica);
  int size_wo_MAC = sizeof(PBFT_C_Reply_rep)+rep_size;

  INCR_OP(reply_auth_ver);
  START_CC(reply_auth_ver_cycles);

  bool ret = replica->verify_mac_in(contents(), sizeof(PBFT_C_Reply_rep), contents()+size_wo_MAC);

  STOP_CC(reply_auth_ver_cycles);

  return ret;
}


bool PBFT_C_Reply::convert(PBFT_C_Message *m1, PBFT_C_Reply *&m2) {
  if (!m1->has_tag(PBFT_C_Reply_tag, sizeof(PBFT_C_Reply_rep)))
    return false;

  m1->trim();
  m2 = (PBFT_C_Reply*)m1;
  return true;
}
