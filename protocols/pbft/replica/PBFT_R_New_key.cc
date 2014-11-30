#include "th_assert.h"
#include "PBFT_R_Message_tags.h"
#include "PBFT_R_New_key.h"
#include "PBFT_R_Node.h"
#include "PBFT_R_Principal.h"
 
PBFT_R_New_key::PBFT_R_New_key() : PBFT_R_Message(PBFT_R_New_key_tag, Max_message_size) {
  unsigned k[Nonce_size_u];

  rep().rid = PBFT_R_node->new_rid();
  rep().padding = 0;
  PBFT_R_node->principal()->set_out_key(k, rep().rid);
  rep().id = PBFT_R_node->id();

  // Get new keys and encrypt them
  PBFT_R_Principal *p;
  char *dst = contents()+sizeof(PBFT_R_New_key_rep);
  int dst_len = Max_message_size-sizeof(PBFT_R_New_key_rep);
  for (int i=0; i < PBFT_R_node->n(); i++) {
    // Skip myself.
    if (i == PBFT_R_node->id()) continue;

    PBFT_PBFT_R_random_nonce(k);
    p = PBFT_R_node->i_to_p(i);
    p->set_in_key(k);
    unsigned ssize = p->encrypt((char *)k, Nonce_size, dst, dst_len);
    th_assert(ssize != 0, "PBFT_R_Message is too small");
    dst += ssize;
    dst_len -= ssize;
  }
  // set my size to reflect the amount of space in use
  set_size(Max_message_size-dst_len);

  // Compute signature and update size.
  p = PBFT_R_node->principal();
  int old_size = size();
  th_assert(dst_len >= p->sig_size(), "PBFT_R_Message is too small");
  set_size(size()+p->sig_size());
  PBFT_R_node->gen_signature(contents(), old_size, contents()+old_size);
}

bool PBFT_R_New_key::verify() {
  // If bad principal or old message discard.
  PBFT_R_Principal *p = PBFT_R_node->i_to_p(id());
  if (p == 0 || p->last_tstamp() >= rep().rid) {
    return false;
  }

  char *dst = contents()+sizeof(PBFT_R_New_key_rep);
  int dst_len = size()-sizeof(PBFT_R_New_key_rep);
  unsigned k[Nonce_size_u];

  for (int i=0; i < PBFT_R_node->n(); i++) {
    // Skip principal that sent message
    if (i == id()) continue;

    int ssize = cyphePBFT_R_size(dst, dst_len); 
    if (ssize == 0)
      return false;

    if (i == PBFT_R_node->id()) {
      // found my key
      int ksize = PBFT_R_node->decrypt(dst, dst_len, (char *)k, Nonce_size);
      if (ksize != Nonce_size) 
	return false;
    } 

    dst += ssize;
    dst_len -= ssize;
  }

  // Check signature    
  int aligned = ALIGNED_SIZE(dst-contents());
  if (dst_len < p->sig_size() || 
      !p->verify_signature(contents(), aligned, contents()+aligned))
    return false;

  p->set_out_key(k, rep().rid);

  return true;
}

bool PBFT_R_New_key::convert(PBFT_R_Message *m1, PBFT_R_New_key  *&m2) {
  if (!m1->has_tag(PBFT_R_New_key_tag, sizeof(PBFT_R_New_key_rep)))
    return false;

  m1->trim();
  m2 = (PBFT_R_New_key*)m1;
  return true;
}
