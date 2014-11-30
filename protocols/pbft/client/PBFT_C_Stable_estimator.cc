#include "PBFT_C_Replica.h"
#include "PBFT_C_Reply_stable.h"
#include "PBFT_C_Stable_estimator.h"
#include "K_max.h"

PBFT_C_Stable_estimator::PBFT_C_Stable_estimator() {
  nv = PBFT_C_node->n();
  vals = new Val[nv];
  est = -1;
}


PBFT_C_Stable_estimator::~PBFT_C_Stable_estimator() {
  delete [] vals;
}


bool PBFT_C_Stable_estimator::add(PBFT_C_Reply_stable* m, bool mine) {
  if (mine || m->verify()) {
    const int id = m->id();
    const int lc = m->last_checkpoint();
    const int lp = m->last_prepared();

    Val& val = vals[id];
    Seqno oc = Seqno_max;
    Seqno op = -1;

    if (lc < val.lc) {
      oc = val.lc;
      val.lc = lc;
      val.lec = 1;
      val.gep = 1;
    }

    if (val.lp < lp) {
      op = val.lp;
      val.lp = lp;
    }

    const int nge = PBFT_C_node->f()+1;
    const int nle = 2*PBFT_C_node->f()+1;
    for (int i=0; i < nv; i++) {
      if (i == id) continue;

      Val &v = vals[i];

      if ((oc > v.lc) && (lc <= v.lc))
	v.lec++;

      if (lc >= v.lc)
	val.lec++;

      if ((op < v.lc) && (lp >= v.lc))
	v.gep++;

      if (lc <= v.lp)
	v.gep++;

      if ((v.lec >= nle) && (v.gep >= nge)) {
	est = v.lc;
	break;
      }
    }

    if ((est < 0) && (val.lec >= nle) && (val.gep >= nge))
      est = val.lc;
  }
  delete m;
  return est >= 0;
}


void PBFT_C_Stable_estimator::mark_stale() {
  for (int i=0; i < PBFT_C_node->n(); i++) {
    vals[i].clear();
  }
}


void PBFT_C_Stable_estimator::clear() {
  mark_stale();
  est = -1;
}

Seqno PBFT_C_Stable_estimator::low_estimate() {
  Seqno lps[Max_num_replicas];

  for (int i=0; i < PBFT_C_node->n(); i++) {
    lps[i] = vals[i].lp;
  }

  Seqno mlp = K_max(PBFT_C_node->f()+1, lps, PBFT_C_node->n(), Seqno_max);
  return (mlp-max_out+checkpoint_interval-2)/checkpoint_interval * checkpoint_interval;
}
