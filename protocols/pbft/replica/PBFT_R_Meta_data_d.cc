#include "th_assert.h"
#include "PBFT_R_Message_tags.h"
#include "PBFT_R_Meta_data_d.h"
#include "PBFT_R_Partition.h"
#include "PBFT_R_Node.h"
#include "PBFT_R_Replica.h"
#include "PBFT_R_Principal.h"


PBFT_R_Meta_data_d::PBFT_R_Meta_data_d(Request_id r, int l, int i, Seqno ls)
  : PBFT_R_Message(PBFT_R_Meta_data_d_tag, sizeof(PBFT_R_Meta_data_d_rep)+MAC_size) {
  th_assert(l < PLevels, "Invalid argument");
  th_assert(i < PLevelSize[l], "Invalid argument");
  rep().rid = r;
  rep().ls = ls;
  rep().l = l;
  rep().i = i;
  rep().id = PBFT_R_replica->id();

  for (int k=0; k < max_out/checkpoint_interval+1; k++)
    rep().digests[k].zero();
  rep().n_digests = 0;
}


void PBFT_R_Meta_data_d::add_digest(Seqno n, Digest& digest) {
  th_assert(n%checkpoint_interval == 0, "Invalid argument");
  th_assert((last_stable() <= n) && (n <= last_stable()+max_out), "Invalid argument");

  int index = (n-last_stable())/checkpoint_interval;
  rep().digests[index] = digest;

  if (index >= rep().n_digests) {
    rep().n_digests = index+1;
  }
}


bool PBFT_R_Meta_data_d::digest(Seqno n, Digest& d) {
  if (n%checkpoint_interval != 0 || last_stable() > n) {
    return false;
  }

  int index = (n-last_stable())/checkpoint_interval;
  if (index >= rep().n_digests || rep().digests[index].is_zero()) {
    return false;
  }

  d = rep().digests[index];
  return true;
}


void PBFT_R_Meta_data_d::authenticate(PBFT_R_Principal *p) {
  set_size(sizeof(PBFT_R_Meta_data_d_rep)+MAC_size);
  p->gen_mac_out(contents(), sizeof(PBFT_R_Meta_data_d_rep), contents()+sizeof(PBFT_R_Meta_data_d_rep));
}


bool PBFT_R_Meta_data_d::verify() {
  // Meta-data must be sent by PBFT_R_replicas.
  if (!PBFT_R_node->is_PBFT_R_replica(id()) || PBFT_R_node->id() == id() || last_stable() < 0)
    return false;

  if (level() < 0 || level() >= PLevels)
    return false;

  if (index() < 0 || index() >=  PLevelSize[level()])
    return false;

  if (rep().n_digests < 1 || rep().n_digests >= max_out/checkpoint_interval+1)
    return false;

  // Check sizes
  if (size() < (int)ALIGNED_SIZE(sizeof(PBFT_R_Meta_data_d_rep)+MAC_size)) {
    return false;
  }

  // Check MAC
  PBFT_R_Principal *p = PBFT_R_node->i_to_p(id());
  if (p) {
    return p->verify_mac_in(contents(), sizeof(PBFT_R_Meta_data_d_rep),
			    contents()+sizeof(PBFT_R_Meta_data_d_rep));
  }

  return false;
}


bool PBFT_R_Meta_data_d::convert(PBFT_R_Message *m1, PBFT_R_Meta_data_d  *&m2) {
  if (!m1->has_tag(PBFT_R_Meta_data_d_tag, sizeof(PBFT_R_Meta_data_d_rep)))
    return false;
  m1->trim();
  m2 = (PBFT_R_Meta_data_d*)m1;
  return true;
}
