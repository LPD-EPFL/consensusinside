#ifndef _PBFT_R_Prepared_cert_h
#define _PBFT_R_Prepared_cert_h 1

#include <sys/time.h>
#include "types.h"
#include "parameters.h"
#include "PBFT_R_Certificate.h"
#include "PBFT_R_Prepare.h"
#include "PBFT_R_Node.h"
#include "PBFT_R_Pre_prepare.h"
#include "PBFT_R_Pre_prepare_info.h"

class PBFT_R_Prepared_cert {
public:
  PBFT_R_Prepared_cert();
  // Effects: Creates an empty prepared certificate. 

  ~PBFT_R_Prepared_cert();
  // Effects: Deletes certificate and all the messages it contains.
 
  bool add(PBFT_R_Prepare *m);
  // Effects: Adds "m" to the certificate and returns true provided
  // "m" satisfies:
  // 1. there is no message from "m.id()" in the certificate
  // 2. "m->verify() == true"
  // 3. if "pc.cvalue() != 0", "pc.cvalue()->match(m)";
  // otherwise, it has no effect on this and returns false.  This
  // becomes the owner of "m" (i.e., no other code should delete "m"
  // or retain pointers to "m".)

  bool add(PBFT_R_Pre_prepare *m);
  // Effects: Adds "m" to the certificate and returns true provided
  // "m" satisfies:
  // 1. there is no prepare from the calling principal in the certificate
  // 2. "m->verify() == true"

  bool add_mine(PBFT_R_Prepare *m);
  // Requires: The identifier of the calling principal is "m->id()",
  // it is not the primary for "m->view(), and "mine()==0".
  // Effects: If "cvalue() != 0" and "!cvalue()->match(m)", it has no
  // effect and returns false. Otherwise, adds "m" to the certificate
  // and returns. This becomes the owner of "m"
  
  bool add_mine(PBFT_R_Pre_prepare *m);
  // Requires: The identifier of the calling principal is "m->id()",
  // it is the primary for "m->view()", and it has no message in
  // certificate.
  // Effects: Adds "m" to certificate and returns true.

  void add_old(PBFT_R_Pre_prepare *m);
  // Requires: There is no pre-prepare in this
  // Effects: Adds a pre-prepare message macthing this from an old
  // view.

  void add(Digest& d, int i);
  // Effects: If there is a pre-prepare message in this whose i-th
  // reference to a big request is d, records that d is cached and may
  // make the certificate complete.
 
  PBFT_R_Prepare *my_prepare(PBFT_R_Time **t=0);
  PBFT_R_Pre_prepare *my_pre_prepare(PBFT_R_Time **t=0);
  // Effects: If the calling PBFT_R_replica has a prepare/pre_prepare message
  // in the certificate, returns a pointer to that message and sets
  // "*t" (if not null) to point to the time at which the message was
  // last sent.  Otherwise, it has no effect and returns 0.

  int num_correct();
  // Effects: Returns number of prepares in certificate that are known
  // to be correct.

  bool is_complete();
  // Effects: Returns true iff the certificate is complete.

  bool is_pp_complete();
  // Effects: Returns true iff the pre-prepare-info is complete.

  bool is_pp_correct();
  // Effects: Returns true iff there are f prepares with same digest
  // as pre-prepare.

  PBFT_R_Pre_prepare* pre_prepare() const;
  // Effects: Returns the pre-prepare in the certificate (or null if
  // the certificate contains no such message.)

  BR_map missing_reqs() const;
  // Effects: Returns a bit map with a bit reset for each request that is missing
  // in pre-prepare.

  PBFT_R_Pre_prepare* rem_pre_prepare();
  // Effects: Returns the pre-prepare in the certificate and removes it 
  
  PBFT_R_Prepare* prepare() const;
  // Effects: If there is a correct prepare value returns it;
  // otherwise returns 0. 

  PBFT_R_Pre_prepare_info const * prep_info() const;
  // Effects: Returns a pointer to the pre-prepare info in this.

  void mark_stale();
  // Effects: Discards all messages in certificate except mine. 

  void clear();
  // Effects: Discards all messages in certificate

  bool is_empty() const;
  // Effects: Returns true iff the certificate is empty

  bool encode(FILE* o);
  bool decode(FILE* i);
  // Effects: Encodes and decodes object state from stream. Return
  // true if successful and false otherwise.
  
private:
  PBFT_R_Certificate<PBFT_R_Prepare> pc;
  PBFT_R_Pre_prepare_info pi;
  PBFT_R_Time t_sent; // time at which pp was sent (if I am primary)
  bool primary; // true iff pp was added with add_mine
};


inline bool PBFT_R_Prepared_cert::add(PBFT_R_Prepare *m) { return pc.add(m); }


inline bool PBFT_R_Prepared_cert::add_mine(PBFT_R_Prepare *m) { 
  th_assert(PBFT_R_node->id() != PBFT_R_node->primary(m->view()), "Invalid Argument");
  return pc.add_mine(m); 
}

  
inline bool PBFT_R_Prepared_cert::add_mine(PBFT_R_Pre_prepare *m) { 
  th_assert(PBFT_R_node->id() == PBFT_R_node->primary(m->view()), "Invalid Argument");
  th_assert(!pi.pre_prepare(), "Invalid state");
  pi.add_complete(m);
  primary = true;
  t_sent = currentPBFT_R_Time();
  return true;
}


inline void PBFT_R_Prepared_cert::add_old(PBFT_R_Pre_prepare *m) {
  th_assert(pi.pre_prepare() == 0, "Invalid state");
  pi.add(m);
}
 

inline void PBFT_R_Prepared_cert::add(Digest& d, int i) {
  pi.add(d, i);
}

 
inline PBFT_R_Prepare *PBFT_R_Prepared_cert::my_prepare(PBFT_R_Time **t) { return pc.mine(t); }


inline PBFT_R_Pre_prepare *PBFT_R_Prepared_cert::my_pre_prepare(PBFT_R_Time **t) {
  if (primary) {
    if (t && pi.pre_prepare()) *t = &t_sent;
    return pi.pre_prepare();
  }
  return 0;
}

 
inline const PBFT_R_Pre_prepare_info* PBFT_R_Prepared_cert::prep_info() const { return &pi; }
 

inline int PBFT_R_Prepared_cert::num_correct() { return pc.num_correct(); }


inline bool PBFT_R_Prepared_cert::is_complete() { 
  return pi.is_complete() && pc.is_complete() && pi.pre_prepare()->match(pc.cvalue());
}  


inline bool PBFT_R_Prepared_cert::is_pp_complete() {
  return pi.is_complete();
}


inline PBFT_R_Pre_prepare *PBFT_R_Prepared_cert::pre_prepare()  const { return pi.pre_prepare(); }


inline BR_map PBFT_R_Prepared_cert::missing_reqs() const { return pi.missing_reqs(); }


inline PBFT_R_Pre_prepare *PBFT_R_Prepared_cert::rem_pre_prepare() { 
  PBFT_R_Pre_prepare *ret = pi.pre_prepare(); 
  pi.zero();
  return ret; 
}

  
inline PBFT_R_Prepare *PBFT_R_Prepared_cert::prepare()  const { return pc.cvalue(); }


inline void PBFT_R_Prepared_cert::mark_stale() {
  if (!is_complete()) {
    if (PBFT_R_node->primary() != PBFT_R_node->id()) {
      pi.clear();
    }
    pc.mark_stale();
  }
}


inline void PBFT_R_Prepared_cert::clear() { 
  pi.clear();
  t_sent = 0;
  pc.clear(); 
  primary = false;
}


#endif // PBFT_R_Prepared_cert_h
 
  










