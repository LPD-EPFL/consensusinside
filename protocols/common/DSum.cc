#include<string.h>
#include "Digest.h"
#include "DSum.h"

void DSum::add(Digest &d) {
  mp_limb_t ret = mpn_add((mp_ptr) sum, (mp_srcptr) sum, nlimbs,
                          (mp_srcptr)d.udigest(), sizeof(d)/sizeof(mp_limb_t));
  th_assert(ret == 0, "sum and d should be such that there is no carry");

  if (mpn_cmp(sum, M->sum, nlimbs) >= 0) {
    mpn_sub((mp_ptr) sum, (mp_srcptr) sum, nlimbs, (mp_srcptr)M->sum, nlimbs);
  }
}


void  DSum::sub(Digest &d) {
  int dlimbs = sizeof(d)/sizeof(mp_limb_t);
  bool gt = false;
  for (int i = nlimbs-1; i >= dlimbs; i--) {
    if (sum[i] != 0) {
      gt = true;
      break;
    }
  }

  mp_limb_t ret;
  if (!gt && mpn_cmp(&sum[dlimbs-1], (mp_limb_t*)d.udigest(), dlimbs) < 0) {
    ret = mpn_add((mp_ptr) sum, (mp_srcptr) sum, nlimbs, (mp_srcptr)M->sum, nlimbs);
    th_assert(ret == 0, "There should be no carry");
  }

  ret = mpn_sub((mp_ptr) sum, (mp_srcptr) sum, nlimbs,
                (mp_srcptr)d.udigest(), sizeof(d)/sizeof(mp_limb_t));
  th_assert(ret == 0, "sum and d should be such that there is no borrow");

}


