#include "PBFT_C_Node.h"
#include "PBFT_C_Prepared_cert.h"

#include "PBFT_C_Certificate.t"
template class PBFT_C_Certificate<PBFT_C_Prepare>;

PBFT_C_Prepared_cert::PBFT_C_Prepared_cert() : pc(PBFT_C_node->f()*2), primary(false) {}


PBFT_C_Prepared_cert::~PBFT_C_Prepared_cert() { pi.clear(); }


bool PBFT_C_Prepared_cert::is_pp_correct() {
  if (pi.pre_prepare()) {
    PBFT_C_Certificate<PBFT_C_Prepare>::Val_iter viter(&pc);
    int vc;
    PBFT_C_Prepare* val;
    while (viter.get(val, vc)) {
      if (vc >= PBFT_C_node->f() && pi.pre_prepare()->match(val)) {
	return true;
      }
    }
  }
  return false;
}


bool PBFT_C_Prepared_cert::add(PBFT_C_Pre_prepare *m) {
  if (pi.pre_prepare() == 0) {
    PBFT_C_Prepare* p = pc.mine();

    if (p == 0) {
      if (m->verify()) {
	pi.add(m);
	return true;
      }

      if (m->verify(PBFT_C_Pre_prepare::NRC)) {
	// Check if there is some value that matches pp and has f
	// senders.
	PBFT_C_Certificate<PBFT_C_Prepare>::Val_iter viter(&pc);
	int vc;
	PBFT_C_Prepare* val;
	while (viter.get(val, vc)) {
	  if (vc >= PBFT_C_node->f() && m->match(val)) {
	    pi.add(m);
	    return true;
	  }
	}
      }
    } else {
      // If we sent a prepare, we only accept a matching pre-prepare.
      if (m->match(p) && m->verify(PBFT_C_Pre_prepare::NRC)) {
	pi.add(m);
	return true;
      }
    }
  }
  delete m;
  return false;
}


bool PBFT_C_Prepared_cert::encode(FILE* o) {
  bool ret = pc.encode(o);
  ret &= pi.encode(o);
  int sz = fwrite(&primary, sizeof(bool), 1, o);
  return ret & (sz == 1);
}


bool PBFT_C_Prepared_cert::decode(FILE* i) {
  th_assert(pi.pre_prepare() == 0, "Invalid state");

  bool ret = pc.decode(i);
  ret &= pi.decode(i);
  int sz = fread(&primary, sizeof(bool), 1, i);
  t_sent = zeroPBFT_C_Time();

  return ret & (sz == 1);
}


bool PBFT_C_Prepared_cert::is_empty() const {
  return pi.pre_prepare() == 0 && pc.is_empty();
}
