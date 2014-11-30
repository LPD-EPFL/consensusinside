#include "th_assert.h"
#include "PBFT_R_Message_tags.h"
#include "PBFT_R_Meta_data.h"
#include "PBFT_R_Partition.h"
#include "PBFT_R_Node.h"
#include "PBFT_R_Replica.h"
#include "PBFT_R_Principal.h"
#include "PBFT_R_Bitmap.h"

PBFT_R_Meta_data::PBFT_R_Meta_data(Request_id r, int l, int i, Seqno lu, Seqno lm,  Digest& d)
  : PBFT_R_Message(PBFT_R_Meta_data_tag, Max_message_size) {
  th_assert(l < PLevels, "Invalid argument");
  th_assert(i < PLevelSize[l], "Invalid argument");
  rep().rid = r;
  rep().lu = lu;
  rep().lm = lm;
  rep().l = l;
  rep().i = i;
  rep().id = PBFT_R_replica->id();
  rep().d = d;
  rep().np = 0;
  set_size(sizeof(PBFT_R_Meta_data_rep));
}


void PBFT_R_Meta_data::add_sub_part(int i, Digest &d) {
  Part_info &p = parts()[rep().np];
  p.i = i;
  p.d = d;
  rep().np++;
  set_size(sizeof(PBFT_R_Meta_data_rep)+rep().np*sizeof(Part_info));
}


PBFT_R_Meta_data::Sub_parts_iter::Sub_parts_iter(PBFT_R_Meta_data *m) {
  msg = m;
  cuPBFT_R_mod = 0;
  max_mod = m->rep().np;
  if (m->level() != PLevels-1) {
    index = m->index()*PSize[m->level()+1];
    max_index = index+PSize[m->level()+1];
  } else {
    index = m->index();
    max_index = index+1;
  }
}


bool PBFT_R_Meta_data::Sub_parts_iter::get(int& i, Digest& d) {
  if (index < max_index) {
    if (cuPBFT_R_mod < max_mod) {
      Part_info *cur = msg->parts() + cuPBFT_R_mod;
      if (index < cur->i) {
	d.zero();
	i = index;
	index++;
	return true;
      }

      th_assert(index ==  cur->i, "Invalid state");
      d = cur->d;
      i = index;
      index++;
      cuPBFT_R_mod++;
      return true;
    }

    d.zero();
    i = index;
    index++;
    return true;
  }
  return false;
}


bool PBFT_R_Meta_data::verify() {
  // Meta-data must be sent by PBFT_R_replicas.
  if (!PBFT_R_node->is_PBFT_R_replica(id()) || PBFT_R_node->id() == id())
    return false;

  // PBFT_R_Meta_data messages are not sent for leaf partitions
  if (level() < 0 || level() >= PLevels-1)
    return false;

  if (index() < 0 || index() >=  PLevelSize[level()])
    return false;

  if (rep().np > 1 && rep().np >  PSize[level()+1])
    return false;

  if (last_mod() > last_uptodate())
    return false;

  // Check sizes
  int old_size = sizeof(PBFT_R_Meta_data_rep) + rep().np*sizeof(Part_info);
  if (size() < ALIGNED_SIZE(old_size)) {
    return false;
  }

  Part_info *ps = parts();
  int min = index()*PSize[level()+1];
  int max = min+PSize[level()+1];
  PBFT_R_Bitmap sparts(max-min);
  for (int i=0; i < rep().np; i++) {
    int si = ps[i].i;
    if (si < min || si >= max || sparts.test(si-min)) {
      return false;
    }
    sparts.set(si-min);
  }

  return true;
}


bool PBFT_R_Meta_data::convert(PBFT_R_Message *m1, PBFT_R_Meta_data  *&m2) {
  if (!m1->has_tag(PBFT_R_Meta_data_tag, sizeof(PBFT_R_Meta_data_rep)))
    return false;
  m1->trim();
  m2 = (PBFT_R_Meta_data*)m1;
  return true;
}
