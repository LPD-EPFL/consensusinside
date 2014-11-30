#ifndef _PBFT_C_Fetch_h
#define _PBFT_C_Fetch_h 1

#include "types.h"
#include "Digest.h"
#include "PBFT_C_Message.h"
#include "PBFT_C_State_defs.h"

class PBFT_C_Principal;

// 
// PBFT_C_Fetch messages have the following format:
//
struct PBFT_C_Fetch_rep : public PBFT_C_Message_rep {
  Request_id rid;     // sequence number to prevent replays
  int level;          // level of partition
  int index;          // index of partition within level
  Seqno lu;           // information for partition is up-to-date till seqno lu
  Seqno rc;           // specific checkpoint requested (-1) if none
  int repid;          // id of designated replier (valid if c >= 0)
  int id;             // id of the replica that generated the message.
#ifndef NO_STATE_TRANSLATION
  int chunk_no;       // number of the fragment we are requesting
  int padding;
#endif

  // Followed by an authenticator.
};

class PBFT_C_Fetch : public PBFT_C_Message {
  // 
  // PBFT_C_Fetch messages
  //
public:
  PBFT_C_Fetch(Request_id rid, Seqno lu, int level, int index,
#ifndef NO_STATE_TRANSLATION
	int chunkn = 0,
#endif
	Seqno rc=-1, int repid=-1);
  // Effects: Creates a new authenticated PBFT_C_Fetch message.

  void re_authenticate(PBFT_C_Principal *p=0);
  // Effects: Recomputes the authenticator in the message using the
  // most recent keys. If "p" is not null, may only update "p"'s
  // entry.

  Request_id request_id() const;
  // Effects: PBFT_C_Fetches the request identifier from the message.

  Seqno last_uptodate() const;
  // Effects: PBFT_C_Fetches the last up-to-date sequence number from the message.

  int level() const;
  // Effects: Returns the level of the partition  

  int index() const;
  // Effects: Returns the index of the partition within its level

  int id() const;
  // Effects: PBFT_C_Fetches the identifier of the replica from the message.

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
  // the replica id().

  static bool convert(PBFT_C_Message *m1, PBFT_C_Fetch *&m2);
  // Effects: If "m1" has the right size and tag, casts "m1" to a
  // "PBFT_C_Fetch" pointer, returns the pointer in "m2" and returns
  // true. Otherwise, it returns false. 
 
private:
  PBFT_C_Fetch_rep &rep() const;
  // Effects: Casts contents to a PBFT_C_Fetch_rep&

};


inline PBFT_C_Fetch_rep &PBFT_C_Fetch::rep() const { 
  th_assert(ALIGNED(msg), "Improperly aligned pointer");
  return *((PBFT_C_Fetch_rep*)msg); 
}

inline Request_id PBFT_C_Fetch::request_id() const { return rep().rid; }

inline  Seqno  PBFT_C_Fetch::last_uptodate() const { return rep().lu; }

inline int PBFT_C_Fetch::level() const { return rep().level; }

inline int PBFT_C_Fetch::index() const { return rep().index; }

inline int PBFT_C_Fetch::id() const { return rep().id; }

#ifndef NO_STATE_TRANSLATION
inline int PBFT_C_Fetch::chunk_number() const { return rep().chunk_no; }
#endif

inline Seqno PBFT_C_Fetch::checkpoint() const { return rep().rc; }

inline int PBFT_C_Fetch::replier() const { return rep().repid; }



#endif // _PBFT_C_Fetch_h
