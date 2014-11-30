#include <strings.h>
#include "th_assert.h"
#include "PBFT_R_Message_tags.h"
#include "PBFT_R_Reply.h"
#include "PBFT_R_Node.h"
#include "PBFT_R_Principal.h"

#include "PBFT_R_Statistics.h"

PBFT_R_Reply::PBFT_R_Reply(View view, Request_id req, int PBFT_R_replica) : 
  PBFT_R_Message(PBFT_R_Reply_tag, Max_message_size) {
    rep().v = view; 
    rep().rid = req;
    rep().PBFT_R_replica = PBFT_R_replica;
    rep().reply_size = 0;
    set_size(sizeof(PBFT_R_Reply_rep));
}


PBFT_R_Reply::PBFT_R_Reply(PBFT_R_Reply_rep *r) : PBFT_R_Message(r) {}


PBFT_R_Reply::PBFT_R_Reply(View view, Request_id req, int PBFT_R_replica, Digest &d, 
	     PBFT_R_Principal *p, bool tentative) :
  PBFT_R_Message(PBFT_R_Reply_tag, sizeof(PBFT_R_Reply_rep)+MAC_size) {

	if (tentative) rep().extra = 1;
	else rep().extra = 0;
	
    rep().v = view; 
    rep().rid = req;
    rep().PBFT_R_replica = PBFT_R_replica;
    rep().reply_size = -1;
    rep().digest = d;

    INCPBFT_R_OP(reply_auth);
    START_CC(reply_auth_cycles);
    p->gen_mac_out(contents(), sizeof(PBFT_R_Reply_rep), contents()+sizeof(PBFT_R_Reply_rep));
    STOP_CC(reply_auth_cycles);
}


PBFT_R_Reply* PBFT_R_Reply::copy(int id) const {
  PBFT_R_Reply* ret = (PBFT_R_Reply*)new PBFT_R_Message(msg->size);
  memcpy(ret->msg, msg, msg->size);
  ret->rep().PBFT_R_replica = id;
  return ret;
}


char *PBFT_R_Reply::store_reply(int &max_len) {
  max_len = msize()-sizeof(PBFT_R_Reply_rep)-MAC_size;
  return contents()+sizeof(PBFT_R_Reply_rep);
}


void PBFT_R_Reply::authenticate(PBFT_R_Principal *p, int act_len, bool tentative) {
  th_assert((unsigned)act_len <= msize()-sizeof(PBFT_R_Reply_rep)-MAC_size,
	    "Invalid reply size");

  if (tentative) rep().extra = 1;
  
  rep().reply_size = act_len;
  rep().digest = Digest(contents()+sizeof(PBFT_R_Reply_rep), act_len);
  int old_size = sizeof(PBFT_R_Reply_rep)+act_len;
  set_size(old_size+MAC_size);

  INCPBFT_R_OP(reply_auth);
  START_CC(reply_auth_cycles);
  p->gen_mac_out(contents(), sizeof(PBFT_R_Reply_rep), contents()+old_size);
  STOP_CC(reply_auth_cycles);

  trim();
}


void PBFT_R_Reply::re_authenticate(PBFT_R_Principal *p) {
  int old_size = sizeof(PBFT_R_Reply_rep)+rep().reply_size;

  INCPBFT_R_OP(reply_auth);
  START_CC(reply_auth_cycles);
  p->gen_mac_out(contents(), sizeof(PBFT_R_Reply_rep), contents()+old_size);
  STOP_CC(reply_auth_cycles);
}


void PBFT_R_Reply::commit(PBFT_R_Principal *p) {
  if (rep().extra == 0) return; // PBFT_R_Reply is already committed.

  rep().extra = 0;
  int old_size = sizeof(PBFT_R_Reply_rep)+rep().reply_size;
  p->gen_mac_out(contents(), sizeof(PBFT_R_Reply_rep), contents()+old_size);
}


bool PBFT_R_Reply::verify() {
  // Replies must be sent by PBFT_R_replicas.
  if (!PBFT_R_node->is_PBFT_R_replica(id())) 
    return false;

  // Check sizes
  int rep_size = (full()) ? rep().reply_size : 0;
  if (size()-(int)sizeof(PBFT_R_Reply_rep)-rep_size < MAC_size) 
    return false;

  // Check reply
  if (full()) {
    Digest d(contents()+sizeof(PBFT_R_Reply_rep), rep_size);
    if (d != rep().digest)
      return false;
  }

  // Check signature.
  PBFT_R_Principal *PBFT_R_replica = PBFT_R_node->i_to_p(rep().PBFT_R_replica);
  int size_wo_MAC = sizeof(PBFT_R_Reply_rep)+rep_size;
  
  INCPBFT_R_OP(reply_auth_ver);
  START_CC(reply_auth_vePBFT_R_cycles);

  bool ret = PBFT_R_replica->verify_mac_in(contents(), sizeof(PBFT_R_Reply_rep), contents()+size_wo_MAC);

  STOP_CC(reply_auth_vePBFT_R_cycles);

  return ret;
}


bool PBFT_R_Reply::convert(PBFT_R_Message *m1, PBFT_R_Reply *&m2) {
  if (!m1->has_tag(PBFT_R_Reply_tag, sizeof(PBFT_R_Reply_rep)))
    return false;
  
  m1->trim();
  m2 = (PBFT_R_Reply*)m1;
  return true;
}
