#include <stdlib.h>
#include <strings.h>
#include "th_assert.h"
#include "PBFT_C_Message_tags.h"
#include "PBFT_C_Request.h"
#include "PBFT_C_Node.h"
#include "PBFT_C_Principal.h"
#include "MD5.h"

#include "PBFT_C_Statistics.h"

// extra & 1 = read only
// extra & 2 = signed

PBFT_C_Request::PBFT_C_Request(Request_id r, short rr) :
  PBFT_C_Message(PBFT_C_Request_tag, Max_message_size) {
  rep().cid = PBFT_C_node->id();
  rep().rid = r;
  rep().replier = rr;
  rep().command_size = 0;
  set_size(sizeof(PBFT_C_Request_rep));
}


PBFT_C_Request* PBFT_C_Request::clone() const {
   PBFT_C_Request* ret = (PBFT_C_Request*)new PBFT_C_Message(max_size);
   memcpy(ret->msg, msg, msg->size);
   return ret;
}

char *PBFT_C_Request::store_command(int &max_len) {
  int max_auth_size = MAX(PBFT_C_node->principal()->sig_size(), PBFT_C_node->auth_size());
  max_len = msize()-sizeof(PBFT_C_Request_rep)-max_auth_size;
  return contents()+sizeof(PBFT_C_Request_rep);
}


inline void PBFT_C_Request::comp_digest(Digest& d) {
  INCR_OP(num_digests);
  START_CC(digest_cycles);

  MD5_CTX context;
  MD5Init(&context);
  MD5Update(&context, (char*)&(rep().cid), sizeof(int)+sizeof(Request_id)+rep().command_size);
  MD5Final(d.udigest(), &context);

  STOP_CC(digest_cycles);
}


void PBFT_C_Request::authenticate(int act_len, bool read_only) {
  th_assert((unsigned)act_len <=
	    msize()-sizeof(PBFT_C_Request_rep)-PBFT_C_node->auth_size(),
	    "Invalid request size");

  rep().extra = ((read_only) ? 1 : 0);
  rep().command_size = act_len;
  if (rep().replier == -1)
    rep().replier = lrand48()%PBFT_C_node->n();
  comp_digest(rep().od);


  int old_size = sizeof(PBFT_C_Request_rep)+act_len;
  set_size(old_size+PBFT_C_node->auth_size());
  PBFT_C_node->gen_auth_in(contents(), sizeof(PBFT_C_Request_rep), contents()+old_size);
}


void PBFT_C_Request::re_authenticate(bool change, PBFT_C_Principal *p) {
  if (change) {
    rep().extra &= ~1;
  }
  int new_rep = lrand48()%PBFT_C_node->n();
  rep().replier = (new_rep != rep().replier) ? new_rep : (new_rep+1)%PBFT_C_node->n();

  int old_size = sizeof(PBFT_C_Request_rep)+rep().command_size;
  if ((rep().extra & 2) == 0) {
	  PBFT_C_node->gen_auth_in(contents(), sizeof(PBFT_C_Request_rep), contents()+old_size);
  } else {
	  PBFT_C_node->gen_signature(contents(), sizeof(PBFT_C_Request_rep), contents()+old_size);
  }
}


void PBFT_C_Request::sign(int act_len) {
  th_assert((unsigned)act_len <=
	    msize()-sizeof(PBFT_C_Request_rep)-PBFT_C_node->principal()->sig_size(),
	    "Invalid request size");

  rep().extra |= 2;
  rep().command_size = act_len;
  comp_digest(rep().od);

  int old_size = sizeof(PBFT_C_Request_rep)+act_len;
  set_size(old_size+PBFT_C_node->principal()->sig_size());
  PBFT_C_node->gen_signature(contents(), sizeof(PBFT_C_Request_rep), contents()+old_size);
}


PBFT_C_Request::PBFT_C_Request(PBFT_C_Request_rep *contents) : PBFT_C_Message(contents) {}


bool PBFT_C_Request::verify() {
  const int nid = PBFT_C_node->id();
  const int cid = client_id();
  const int old_size = sizeof(PBFT_C_Request_rep)+rep().command_size;
  PBFT_C_Principal* p = PBFT_C_node->i_to_p(cid);
  Digest d;

  comp_digest(d);
  if (p != 0 && d == rep().od) {
    if ((rep().extra & 2) == 0) {
      // PBFT_C_Message has an authenticator.
      if (cid != nid && cid >= PBFT_C_node->n() && size()-old_size >= PBFT_C_node->auth_size(cid))
	return PBFT_C_node->verify_auth_out(cid, contents(), sizeof(PBFT_C_Request_rep),
				       contents()+old_size);
    } else {
      // PBFT_C_Message is signed.
      if (size() - old_size >= p->sig_size())
	return p->verify_signature(contents(), sizeof(PBFT_C_Request_rep),
				   contents()+old_size, true);
    }
  }
  return false;
}


bool PBFT_C_Request::convert(PBFT_C_Message *m1, PBFT_C_Request  *&m2) {
  if (!m1->has_tag(PBFT_C_Request_tag, sizeof(PBFT_C_Request_rep)))
    return false;

  m2 = (PBFT_C_Request*)m1;
  m2->trim();
  return true;
}


bool PBFT_C_Request::convert(char *m1, unsigned max_len, PBFT_C_Request &m2) {
  if (!PBFT_C_Message::convert(m1, max_len, PBFT_C_Request_tag, sizeof(PBFT_C_Request_rep),m2))
    return false;
  return true;
}

