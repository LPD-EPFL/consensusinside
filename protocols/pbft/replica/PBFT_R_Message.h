#ifndef _PBFT_R_Message_h
#define _PBFT_R_Message_h 1

#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include "th_assert.h"
#include "types.h"
#include "PBFT_R_Message_tags.h"
#include "PBFT_R_Log_allocator.h"


// Maximum message size. Must verify ALIGNED_SIZE.
const int Max_message_size = 16000;

//
// All messages have the following format:
//
struct PBFT_R_Message_rep {
  short tag;
  short extra; // May be used to store extra information.
  int size;    // Must be a multiple of 8 bytes to ensure proper
               // alignment of embedded messages.

  //Followed by char payload[size-sizeof(PBFT_R_Message_rep)];
};


class PBFT_R_Message {
  //
  // Generic messages
  //
public:
  PBFT_R_Message(unsigned sz=0);
  // Effects: Creates an untagged PBFT_R_Message object that can hold up
  // to "sz" bytes and holds zero bytes. Useful to create message
  // buffers to receive messages from the network.

 ~PBFT_R_Message();
  // Effects: Deallocates all storage associated with this message.

  void trim();
  // Effects: Deallocates surplus storage.

  char* contents();
  // Effects: Return a byte string with the message contents.
  // TODO: should be protected here because of request iterator in
  // PBFT_R_Pre_prepare.cc

  int size() const;
  // Effects: PBFT_R_Fetches the message size.

  int tag() const;
  // Effects: PBFT_R_Fetches the message tag.

  bool has_tag(int t, int sz) const;
  // Effects: If message has tag "t", its size is greater than "sz",
  // its size less than or equal to "max_size", and its size is a
  // multiple of ALIGNMENT, returns true.  Otherwise, returns false.


  View view() const;
  // Effects: Returns any view associated with the message or 0.

  bool full() const;
  // Effects: PBFT_R_Messages may be full or empty. Empty messages are just
  // digests of full messages.

  // PBFT_R_Message-specific heap management operators.
  void *operator new(size_t s);
  void operator delete(void *x, size_t s);
  static void init();
  // Effects: Should be called once to initialize the memory allocator.
  // Before any message is allocated.

  const char *stag();
  // Effects: Returns a string with tag name

  bool encode(FILE* o);
  bool decode(FILE* i);
  // Effects: Encodes and decodes object state from stream. Return
  // true if successful and false otherwise.

 static void debug_alloc() {  }
  // Effects: Prints debug information for memory allocator.

protected:
  PBFT_R_Message(int t, unsigned sz);
  // Effects: Creates a message with tag "t" that can hold up to "sz"
  // bytes. Useful to create messages to send to the network.

  PBFT_R_Message(PBFT_R_Message_rep *contents);
  // Requires: "contents" contains a valid PBFT_R_Message_rep.
  // Effects: Creates a message from "contents". No copy is made of
  // "contents" and the storage associated with "contents" is not
  // deallocated if the message is later deleted. Useful to create
  // messages from reps contained in other messages.

  void set_size(int size);
  // Effects: Sets message size to the smallest multiple of 8 bytes
  // greater than equal to "size" and pads the message with zeros
  // between "size" and the new message size. Important to ensure
  // proper alignment of embedded messages.

  int msize() const;
  // Effects: PBFT_R_Fetches the maximum number of bytes that can be stored in
  // this message.

  static bool convert(char *src, unsigned len, int t, int sz, PBFT_R_Message &m);
  // Requires: convert can safely read up to "len" bytes starting at
  // "src" Effects: If "src" is a PBFT_R_Message_rep for which "has_tag(t,
  // sz)" returns true and sets m to have contents "src". Otherwise,
  // it returns false.  No copy is made of "src" and the storage
  // associated with "contents" is not deallocated if "m" is later
  // deleted.

  friend class PBFT_R_Node;
  friend class PBFT_R_Pre_prepare;

  PBFT_R_Message_rep *msg;    // Pointer to the contents of the message.
  int max_size;        // Maximum number of bytes that can be stored in "msg"
                       // or "-1" if this instance is not responsible for
                       // deallocating the storage in msg.
  // Invariant: max_size <= 0 || 0 < msg->size <= max_size

private:
};


// Methods inlined for speed

inline int PBFT_R_Message::size() const {
  return msg->size;
}

inline int PBFT_R_Message::tag() const {
  return msg->tag;
}

inline bool PBFT_R_Message::has_tag(int t, int sz) const {
  if (max_size >= 0 && msg->size > max_size)
    return false;

  if (!msg || msg->tag != t || msg->size < sz || !ALIGNED(msg->size))
    return false;
  return true;
}

inline  View PBFT_R_Message::view() const { return 0; }

inline bool PBFT_R_Message::full() const { return true; }

inline void* PBFT_R_Message::operator new(size_t s) {
  void *ret = (void*)malloc(ALIGNED_SIZE(s));
  th_assert(ret != 0, "Ran out of memory\n");
  return ret;
}

inline void PBFT_R_Message::operator delete(void *x, size_t s) {
  if (x != 0)
    free((char*)x);
}

inline int PBFT_R_Message::msize() const {
  return (max_size >= 0) ? max_size : msg->size;
}

inline char *PBFT_R_Message::contents() {
  return (char *)msg;
}

#endif //_PBFT_R_Message_h






