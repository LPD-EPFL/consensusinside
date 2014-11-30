#include "PBFT_R_Meta_data_cert.h"
#include "PBFT_R_Meta_data_d.h"
#include "PBFT_R_Node.h"
#include "K_max.h"

PBFT_R_Meta_data_cert::PBFT_R_Meta_data_cert() {
  last_mdds = new PBFT_R_Meta_data_d*[PBFT_R_node->n()];
  last_stables = new Seqno[PBFT_R_node->n()];
  for (int i=0; i < PBFT_R_node->n(); i++) {
    last_mdds[i] = 0;
    last_stables[i] = 0;
  }
  ls = 0;

  max_size = PBFT_R_node->n()*(max_out/checkpoint_interval+1);
  vals = new Part_val[max_size];
  cuPBFT_R_size = 0;
  correct = PBFT_R_node->f()+1;
  c = -1;
  has_my_message = false;
}


PBFT_R_Meta_data_cert::~PBFT_R_Meta_data_cert() {
  clear();
  delete [] last_mdds;
  delete [] vals;
  delete [] last_stables;
}


void PBFT_R_Meta_data_cert::clear() { 
  for (int i=0; i < PBFT_R_node->n(); i++) {
    delete last_mdds[i];
    last_mdds[i] = 0;
    last_stables[i] = 0;
  }
  ls = 0;

  for (int i=0; i < cuPBFT_R_size; i++) 
    vals[i].clear(); 

  c = -1;
  cuPBFT_R_size = 0;
  has_my_message = false;
}


bool PBFT_R_Meta_data_cert::add(PBFT_R_Meta_data_d* m, bool mine) {
  if (mine) has_my_message = true;

  if (mine || m->verify()) {
    th_assert(mine || m->id() != PBFT_R_node->id(), 
	      "verify should return false for messages from self");

    // Check if node already had a message in the certificate.
    const int id = m->id();
    PBFT_R_Meta_data_d* om = last_mdds[id];
    if (om && om->last_checkpoint() >=  m->last_checkpoint()) {
      // the new message is stale.
      delete m;
      return false;
    }
	  
    // new message is more recent
    last_mdds[id] = m;

    if (m->last_stable() > ls)
      ls = K_max<Seqno>(PBFT_R_node->f()+1, last_stables, PBFT_R_node->n(), Seqno_max);
    else if (m->last_stable() < ls) {
      delete om;
      delete m;
      last_mdds[id] = 0;
      return false;
    }

    // Update vals to adjust for digests in m and om, and delete vals
    // with seqno less than ls.
    bool matched[max_out/checkpoint_interval+1];
    for (int i=0; i < max_out/checkpoint_interval+1; i++)
      matched[i] = 0;

    for (int i=0; i < cuPBFT_R_size; i++) {
      Part_val& val = vals[i];

      Digest md;
      if (om && om->digest(val.c, md) && md == val.d) {
	val.count--;
      }

      if (m->digest(val.c, md) && md == val.d) {
	val.count++;
	matched[(val.c-m->last_stable())/checkpoint_interval] = true;
      }

      if (val.count == 0 || val.c < ls) {
	// value is empty or obsolete
	do {
	  if (i < cuPBFT_R_size-1) {
	    val = vals[cuPBFT_R_size-1];
	    th_assert(val.c >= 0, "Invalid state");
	  }
	  vals[cuPBFT_R_size-1].clear();
	  cuPBFT_R_size--;
	} while (val.c < ls && cuPBFT_R_size > 0);
	continue;
      }

      if (val.count >= correct && val.c > c) {
	c = val.c;
	d = val.d;
      }
    }
    delete om;

    for (Seqno n=m->last_stable(); n <= m->last_checkpoint();n+=checkpoint_interval) {
      Digest d1;
      if (!matched[(n-m->last_stable())/checkpoint_interval] && m->digest(n, d1)) {
	th_assert(cuPBFT_R_size < max_size, "Invalid state");
	vals[cuPBFT_R_size].c = n;
	vals[cuPBFT_R_size].d = d1;
	vals[cuPBFT_R_size].count = 1; 
	cuPBFT_R_size++;
      }
    }      
    return true;
  }
  delete m;
  return false;
}









