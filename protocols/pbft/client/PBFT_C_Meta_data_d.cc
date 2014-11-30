#include "th_assert.h"
#include "PBFT_C_Message_tags.h"
#include "PBFT_C_Meta_data_d.h"
#include "Partition.h"
#include "PBFT_C_Node.h"
#include "PBFT_C_Replica.h"
#include "PBFT_C_Principal.h"


PBFT_C_Meta_data_d::PBFT_C_Meta_data_d(Request_id r, int l, int i, Seqno ls)
  : PBFT_C_Message(PBFT_C_Meta_data_d_tag, sizeof(PBFT_C_Meta_data_d_rep)+MAC_size) {
  th_assert(l < PLevels, "Invalid argument");
  th_assert(i < PLevelSize[l], "Invalid argument");
  rep().rid = r;
  rep().ls = ls;
  rep().l = l;
  rep().i = i;
  rep().id = PBFT_C_replica->id();

  for (int k=0; k < max_out/checkpoint_interval+1; k++)
    rep().digests[k].zero();
  rep().n_digests = 0;
}


void PBFT_C_Meta_data_d::add_digest(Seqno n, Digest& digest) {
  th_assert(n%checkpoint_interval == 0, "Invalid argument");
  th_assert((last_stable() <= n) && (n <= last_stable()+max_out), "Invalid argument");

  int index = (n-last_stable())/checkpoint_interval;
  rep().digests[index] = digest;

  if (index >= rep().n_digests) {
    rep().n_digests = index+1;
  }
}


bool PBFT_C_Meta_data_d::digest(Seqno n, Digest& d) {
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


void PBFT_C_Meta_data_d::authenticate(PBFT_C_Principal *p) {
  set_size(sizeof(PBFT_C_Meta_data_d_rep)+MAC_size);
  p->gen_mac_out(contents(), sizeof(PBFT_C_Meta_data_d_rep), contents()+sizeof(PBFT_C_Meta_data_d_rep));
}


bool PBFT_C_Meta_data_d::verify() {
  // Meta-data must be sent by replicas.
  if (!PBFT_C_node->is_replica(id()) || PBFT_C_node->id() == id() || last_stable() < 0)
    return false;

  if (level() < 0 || level() >= PLevels)
    return false;

  if (index() < 0 || index() >=  PLevelSize[level()])
    return false;

  if (rep().n_digests < 1 || rep().n_digests >= max_out/checkpoint_interval+1)
    return false;

  // Check sizes
  if (size() < (int)ALIGNED_SIZE(sizeof(PBFT_C_Meta_data_d_rep)+MAC_size)) {
    return false;
  }

  // Check MAC
  PBFT_C_Principal *p = PBFT_C_node->i_to_p(id());
  if (p) {
    return p->verify_mac_in(contents(), sizeof(PBFT_C_Meta_data_d_rep),
			    contents()+sizeof(PBFT_C_Meta_data_d_rep));
  }

  return false;
}


bool PBFT_C_Meta_data_d::convert(PBFT_C_Message *m1, PBFT_C_Meta_data_d  *&m2) {
  if (!m1->has_tag(PBFT_C_Meta_data_d_tag, sizeof(PBFT_C_Meta_data_d_rep)))
    return false;
  m1->trim();
  m2 = (PBFT_C_Meta_data_d*)m1;
  return true;
}
