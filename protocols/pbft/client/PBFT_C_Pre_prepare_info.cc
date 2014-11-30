#include "PBFT_C_Pre_prepare.h"
#include "PBFT_C_Pre_prepare_info.h"
#include "PBFT_C_Replica.h"

PBFT_C_Pre_prepare_info::~PBFT_C_Pre_prepare_info() {
  delete pp;
}


void PBFT_C_Pre_prepare_info::add(PBFT_C_Pre_prepare* p) {
  th_assert(pp == 0, "Invalid state");
  pp = p;
  mreqs = p->num_big_reqs();
  mrmap = 0;
  PBFT_C_Big_req_table* brt = PBFT_C_replica->big_reqs();

  for (int i=0; i < p->num_big_reqs(); i++) {
    if (brt->add_pre_prepare(p->big_req_digest(i), i, p->seqno(), p->view())) {
      mreqs--;
      Bits_set((char*)&mrmap, i);
    }
  }
}


void PBFT_C_Pre_prepare_info::add(Digest& rd, int i) {
  if (pp && pp->big_req_digest(i) == rd) {
    mreqs--;
    Bits_set((char*)&mrmap, i);
  }
}


bool PBFT_C_Pre_prepare_info::encode(FILE* o) {
  bool hpp = pp != 0;
  size_t sz = fwrite(&hpp, sizeof(bool), 1, o);
  bool ret = true;
  if (hpp)
    ret = pp->encode(o);
  return ret & (sz == 1);
}


bool PBFT_C_Pre_prepare_info::decode(FILE* i) {
  bool hpp;
  size_t sz = fread(&hpp, sizeof(bool), 1, i);
  bool ret = true;
  if (hpp) {
    pp = (PBFT_C_Pre_prepare*) new PBFT_C_Message;
    ret &= pp->decode(i);
  }
  return ret & (sz == 1);
}


PBFT_C_Pre_prepare_info::BRS_iter::BRS_iter(PBFT_C_Pre_prepare_info const *p, BR_map m) {
  ppi = p;
  mrmap = m;
  next = 0;
}

bool PBFT_C_Pre_prepare_info::BRS_iter::get(PBFT_C_Request*& r) {
  PBFT_C_Pre_prepare* pp = ppi->pp;
  while (pp && next < pp->num_big_reqs()) {
    if (!Bits_test((char*)&mrmap, next) & Bits_test((char*)&(ppi->mrmap), next)) {
      r = PBFT_C_replica->big_reqs()->lookup(pp->big_req_digest(next));
      th_assert(r != 0, "Invalid state");
      next++;
      return true;
    }
    next++;
  }
  return false;
}
