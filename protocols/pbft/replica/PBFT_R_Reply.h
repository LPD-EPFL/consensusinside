#ifndef _PBFT_R_Reply_h
#define _PBFT_R_Reply_h 1

#include "types.h"
#include "libbyz.h"
#include "PBFT_R_Message.h"
#include "Digest.h"

class PBFT_R_Principal;
class PBFT_R_Rep_info;

// 
// PBFT_R_Reply messages have the following format.
//
struct PBFT_R_Reply_rep : public PBFT_R_Message_rep {
  View v;                // current view
  Request_id rid;        // unique request identifier
  Digest digest;         // digest of reply.
  int PBFT_R_replica;           // id of PBFT_R_replica sending the reply
  int reply_size;        // if negative, reply is not full.
  bool should_switch; // if true, should switch to protocol with instance_id
  enum protocols_e instance_id; // id of the instance to which should switch
  // Followed by a reply that is "reply_size" bytes long and
  // a MAC authenticating the reply to the client. The MAC is computed
  // only over the PBFT_R_Reply_rep. Replies can be empty or full. Full replies
  // contain the actual reply and have reply_size >= 0. Empty replies
  // do not contain the actual reply and have reply_size < 0.
};

class PBFT_R_Reply : public PBFT_R_Message {
  // 
  // PBFT_R_Reply messages
  //
public:
  PBFT_R_Reply() : PBFT_R_Message() {}

  PBFT_R_Reply(PBFT_R_Reply_rep *r);

  PBFT_R_Reply(View view, Request_id req, int PBFT_R_replica);
  // Effects: Creates a new (full) PBFT_R_Reply message with an empty reply and no
  // authentication. The method store_reply and authenticate should
  // be used to finish message construction.

  PBFT_R_Reply* copy(int id) const;
  // Effects: Creates a new object with the same state as this but
  // with PBFT_R_replica identifier "id"

  char *store_reply(int &max_len);
  // Effects: Returns a pointer to the location within the message
  // where the reply should be stored and sets "max_len" to the number of
  // bytes available to store the reply. The caller can copy any reply
  // with length less than "max_len" into the returned buffer. 

  void authenticate(PBFT_R_Principal *p, int act_len, bool tentative);
  // Effects: Terminates the construction of a reply message by
  // setting the length of the reply to "act_len", appending a MAC,
  // and trimming any surplus storage.

  void re_authenticate(PBFT_R_Principal *p);
  // Effects: Recomputes the authenticator in the reply using the most
  // recent key.

  PBFT_R_Reply(View view, Request_id req, int PBFT_R_replica, Digest &d, 
	PBFT_R_Principal *p, bool tentative);
  // Effects: Creates a new empty PBFT_R_Reply message and appends a MAC for principal
  // "p".

  void commit(PBFT_R_Principal *p);
  // Effects: If this is tentative converts this into an identical
  // committed message authenticated for principal "p".  Otherwise, it
  // does nothing.

  View view() const;
  // Effects: PBFT_R_Fetches the view from the message

  Request_id request_id() const;
  // Effects: PBFT_R_Fetches the request identifier from the message.

  int id() const;
  // Effects: PBFT_R_Fetches the reply identifier from the message.

  Digest &digest() const;
  // Effects: PBFT_R_Fetches the digest from the message.

  char *reply(int &len);
  // Effects: Returns a pointer to the reply and sets len to the
  // reply size.
  
  bool full() const;
  // Effects: Returns true iff "this" is a full reply.

  bool should_switch() const;
  // Effects: returns true if reply was notifying about switching

  enum protocols_e next_instance_id() const;
  // Effects: if should_switch == true, instance_id of the next abstract

  void set_instance_id(enum protocols_e nextp);
  // Effects: sets should_switch = true, and instance_id

  bool verify();
  // Effects: Verifies if the message is authenticated by rep().PBFT_R_replica.

  bool match(PBFT_R_Reply *r);
  // Effects: Returns true if the replies match.
  
  bool is_tentative() const;
  // Effects: Returns true iff the reply is tentative.

  static bool convert(PBFT_R_Message *m1, PBFT_R_Reply *&m2);
  // Effects: If "m1" has the right size and tag of a "PBFT_R_Reply", casts
  // "m1" to a "PBFT_R_Reply" pointer, returns the pointer in "m2" and
  // returns true. Otherwise, it returns false. Convert also trims any
  // surplus storage from "m1" when the conversion is successfull.

private:
  PBFT_R_Reply_rep &rep() const;
  // Effects: Casts "msg" to a PBFT_R_Reply_rep&
  
  friend class PBFT_R_Rep_info;
};

inline PBFT_R_Reply_rep& PBFT_R_Reply::rep() const { 
  th_assert(ALIGNED(msg), "Improperly aligned pointer");
  return *((PBFT_R_Reply_rep*)msg); 
}
   
inline View PBFT_R_Reply::view() const { return rep().v; }

inline Request_id PBFT_R_Reply::request_id() const { return rep().rid; }

inline int PBFT_R_Reply::id() const { return rep().PBFT_R_replica; }

inline Digest& PBFT_R_Reply::digest() const { return rep().digest; }

inline char* PBFT_R_Reply::reply(int &len) { 
  len = rep().reply_size;
  return contents()+sizeof(PBFT_R_Reply_rep);
}
  
inline bool PBFT_R_Reply::full() const { return rep().reply_size >= 0; }

inline bool PBFT_R_Reply::should_switch() const
{
	return rep().should_switch;
}

inline enum protocols_e PBFT_R_Reply::next_instance_id() const
{
	return rep().instance_id;
}

inline void PBFT_R_Reply::set_instance_id(enum protocols_e nextp)
{
	rep().should_switch = true;
	rep().instance_id = nextp;
}

inline bool PBFT_R_Reply::is_tentative() const { return rep().extra; }

inline bool PBFT_R_Reply::match(PBFT_R_Reply *r) {
  return (rep().digest == r->rep().digest) & 
    (!is_tentative() | r->is_tentative() | (view() == r->view()));
}
  


#endif // _PBFT_R_Reply_h
