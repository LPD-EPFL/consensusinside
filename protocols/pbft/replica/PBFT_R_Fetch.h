#ifndef _PBFT_R_Fetch_h
#define _PBFT_R_Fetch_h 1

#include "types.h"
#include "Digest.h"
#include "PBFT_R_Message.h"
#include "PBFT_R_State_defs.h"

class PBFT_R_Principal;

// 
// PBFT_R_Fetch messages have the following format:
//
struct PBFT_R_Fetch_rep : public PBFT_R_Message_rep {
  Request_id rid;     // sequence number to prevent replays
  int level;          // level of partition
  int index;          // index of partition within level
  Seqno lu;           // information for partition is up-to-date till seqno lu
  Seqno rc;           // specific checkpoint requested (-1) if none
  int repid;          // id of designated replier (valid if c >= 0)
  int id;             // id of the PBFT_R_replica that generated the message.
#ifndef NO_STATE_TRANSLATION
  int chunk_no;       // number of the fragment we are requesting
  int padding;
#endif

  // Followed by an authenticator.
};

class PBFT_R_Fetch : public PBFT_R_Message {
  // 
  // PBFT_R_Fetch messages
  //
public:
  PBFT_R_Fetch(Request_id rid, Seqno lu, int level, int index,
#ifndef NO_STATE_TRANSLATION
	int chunkn = 0,
#endif
	Seqno rc=-1, int repid=-1);
  // Effects: Creates a new authenticated PBFT_R_Fetch message.

  void re_authenticate(PBFT_R_Principal *p=0);
  // Effects: Recomputes the authenticator in the message using the
  // most recent keys. If "p" is not null, may only update "p"'s
  // entry.

  Request_id request_id() const;
  // Effects: PBFT_R_Fetches the request identifier from the message.

  Seqno last_uptodate() const;
  // Effects: PBFT_R_Fetches the last up-to-date sequence number from the message.

  int level() const;
  // Effects: Returns the level of the partition  

  int index() const;
  // Effects: Returns the index of the partition within its level

  int id() const;
  // Effects: PBFT_R_Fetches the identifier of the PBFT_R_replica from the message.

#ifndef NO_STATE_TRANSLATION
  int chunk_number() const;
  // Effects: Returns the number of the fragment that is being requested
#endif


  Seqno checkpoint() const;
  // Effects: Returns the specific checkpoint requested or -1

  int replier() const;
  // Effects: If checkpoint() > 0, returns the designated replier. Otherwise,
  // returns -1;

  bool verify();
  // Effects: Verifies if the message is correctly authenticated by
  // the PBFT_R_replica id().

  static bool convert(PBFT_R_Message *m1, PBFT_R_Fetch *&m2);
  // Effects: If "m1" has the right size and tag, casts "m1" to a
  // "PBFT_R_Fetch" pointer, returns the pointer in "m2" and returns
  // true. Otherwise, it returns false. 
 
private:
  PBFT_R_Fetch_rep &rep() const;
  // Effects: Casts contents to a PBFT_R_Fetch_rep&

};


inline PBFT_R_Fetch_rep &PBFT_R_Fetch::rep() const { 
  th_assert(ALIGNED(msg), "Improperly aligned pointer");
  return *((PBFT_R_Fetch_rep*)msg); 
}

inline Request_id PBFT_R_Fetch::request_id() const { return rep().rid; }

inline  Seqno  PBFT_R_Fetch::last_uptodate() const { return rep().lu; }

inline int PBFT_R_Fetch::level() const { return rep().level; }

inline int PBFT_R_Fetch::index() const { return rep().index; }

inline int PBFT_R_Fetch::id() const { return rep().id; }

#ifndef NO_STATE_TRANSLATION
inline int PBFT_R_Fetch::chunk_number() const { return rep().chunk_no; }
#endif

inline Seqno PBFT_R_Fetch::checkpoint() const { return rep().rc; }

inline int PBFT_R_Fetch::replier() const { return rep().repid; }



#endif // _PBFT_R_Fetch_h
