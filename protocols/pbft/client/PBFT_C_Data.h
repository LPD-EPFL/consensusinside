#ifndef _PBFT_C_Data_h
#define _PBFT_C_Data_h 1

#include "types.h"
#include "PBFT_C_Message.h"
#include "Partition.h"
#include "PBFT_C_pbft_libbyz.h"
#include "PBFT_C_State_defs.h"

#ifndef NO_STATE_TRANSLATION
static const int Fragment_size = 4096;
#endif


//
// PBFT_C_Data messages have the following format:
//
struct PBFT_C_Data_rep : public PBFT_C_Message_rep {
  int index;        // index of this page within level
  int padding;
#ifndef NO_STATE_TRANSLATION
  int total_size;   // size of the object
  int chunk_no;     // current fragment number
#endif
  Seqno lm;         // Seqno of last checkpoint in which data was modified
#ifndef NO_STATE_TRANSLATION
  char data[Fragment_size];
#else
  char data[Block_size];
#endif
};

class PBFT_C_Data : public PBFT_C_Message {
  //
  // PBFT_C_Data messages
  //
public:
#ifndef NO_STATE_TRANSLATION
  PBFT_C_Data(int i, Seqno lm, char *data, int totalsz, int chunkn);
#else
  PBFT_C_Data(int i, Seqno lm, char *data);
#endif
  // Effects: Creates a new PBFT_C_Data message.
  //          (if we are using BASE) "totalsz" is the size of the object,
  //          chunkn is the number of the fragment that we are sending
  //          and "data" points to the beginning of the fragment (not to
  //          the beginning of the object - XXX change this?)

  int index() const;
  // Effects: Returns index of data page

  Seqno last_mod() const;
  // Effects: Returns the seqno of last checkpoint in which data was
  // modified

#ifndef NO_STATE_TRANSLATION
  int total_size() const;
  // Effects: Returns total size of data (this msg may contain only a
  //          fragment of smaller size)

  int chunk_number() const;
  // Effects: Returns the number of the fragment that these data correspond
  //          to, in case it is a fragmented message.

  int num_chunks() const;
  // Effects: Returns the total number of fragments needed to send the PBFT_C_Data
  //          block

#endif

  char *data() const;
  // Effects: Returns a pointer to the data page.

  static bool convert(PBFT_C_Message *m1, PBFT_C_Data *&m2);
  // Effects: If "m1" has the right size and tag, casts "m1" to a
  // "PBFT_C_Data" pointer, returns the pointer in "m2" and returns
  // true. Otherwise, it returns false.

private:
  PBFT_C_Data_rep &rep() const;
  // Effects: Casts contents to a PBFT_C_Data_rep&
};

inline PBFT_C_Data_rep &PBFT_C_Data::rep() const {
  th_assert(ALIGNED(msg), "Improperly aligned pointer");
  return *((PBFT_C_Data_rep*)msg);
}

inline int PBFT_C_Data::index() const { return rep().index; }

inline Seqno PBFT_C_Data::last_mod() const { return rep().lm; }

#ifndef NO_STATE_TRANSLATION
inline int PBFT_C_Data::total_size() const { return rep().total_size; }

inline int PBFT_C_Data::chunk_number() const { return rep().chunk_no; }

inline int PBFT_C_Data::num_chunks() const { return rep().total_size / Fragment_size +
				 (rep().total_size % Fragment_size ? 1 : 0); }
#endif

inline char *PBFT_C_Data::data() const { return rep().data; }


#endif // _PBFT_C_Data_h
