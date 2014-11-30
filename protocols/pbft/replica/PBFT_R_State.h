#ifndef _PBFT_R_State_h
#define _PBFT_R_State_h 1

#include "th_assert.h"
#include "types.h"
#include "Digest.h"
#include "DSum.h"
#include "PBFT_R_Bitmap.h"
#include "PBFT_R_Partition.h"
#include "Log.h"
#include "PBFT_R_Time.h"
#include "PBFT_R_State_defs.h"

//
// Auxiliary classes:
//
class Block;
class Part;
class FPartQueue;
class CPartQueue;
class PBFT_R_Checkpoint_rec;
#ifndef NO_STATE_TRANSLATION
class Page_mapping;
class BlockCopy;
#endif
class PBFT_R_Data;
class PBFT_R_Meta_data;
class PBFT_R_Meta_data_d;
class PBFT_R_Fetch;
class PBFT_R_Replica;
class PBFT_R_Meta_data_cert;
class PageCache;


class PBFT_R_State {
public:

#ifndef NO_STATE_TRANSLATION
  PBFT_R_State(PBFT_R_Replica *PBFT_R_replica, int num_objs,
	int (*gets)(int, char **),
	void (*puts)(int, int *, int *, char **),
	void (*shutdown_p)(FILE *o),
	void (*restart_p)(FILE *i)
	);

  // Effects: creates an object that handles state digesting and
  // checkpointing. The state will consist of "num_objs" objects,
  // the abstraction function is "gets" and "puts" is one of its
  // inverses.  The procedures invoked before and after recovery to save
  // and restore extra state information are "shutdown_p" and "restart_p".

#else   // ifndef NO_STATE_TRANSLATION
  PBFT_R_State(PBFT_R_Replica *PBFT_R_replica, char *memory, int num_bytes);
  // Requires: mem is Block aligned and contains an integral number of
  // Blocks.
  // Effects: Creates an object that handles state digesting and
  // checkpointing for the region starting at "mem" with size
  // "num_bytes".
#endif

  ~PBFT_R_State();
  // Effects: Deallocates all storage associated with state.

  //
  // Maintaining checkpoints
  //
  void cow_single(int bindex);
  // Effects: Copies block with bindex and marks it as copied.

  void cow(char *mem, int size);
  // Effects: Performs copies for the blocks in the region
  // starting at "mem" and of size "size"
  // if they have not been copied since last checkpoint.
  // It also marks them as copied.
 
  void checkpoint(Seqno seqno);
  // Effects: Saves a checkpoint of the current state (associated with
  // seqno) and computes the digest of all partition.

  Seqno rollback();
  // Requires: !in_fetch_state && there is a checkpoint in this
  // Effects: Rolls back to the last checkpoint and returns its
  // sequence number.

  void discard_checkpoint(Seqno seqno, Seqno le);
  // Effects: Discards the checkpoints with sequence number less than
  // or equal to "seqno" and saves information about the current state 
  // whose sequence number is "le"

  void compute_full_digest();
  // Effects: Computes a state digest from scratch and a digest for
  // each partition.
  
  bool digest(Seqno n, Digest& d);
  // Effects: If there is a checkpoint for sequence number "n" in
  // this, returns true and sets "d" to its digest. Otherwise, returns
  // false.

  // 
  // PBFT_R_Fetching missing state
  //
  bool in_fetch_state() const;
  // Effects: Returns true iff the PBFT_R_replica is fetching missing state.

  void start_fetch(Seqno last_exec, Seqno c=-1, Digest *cd=0, bool stable=false);
  // Effects: Sends fetch message for missing state. If "c != -1" then
  // "cd" points to the digest of checkpoint sequence number "c". "stable" should
  // be true iff the specific checkpoint being fetched is stable.

  void send_fetch(bool change_replier=false);
  // Effects: Sends fetch message requesting missing state. If
  // change_replier is true changes the selected replier.

  bool in_check_state() const;
  // Effects: Returns true iff the PBFT_R_replica is checking state

  void start_check(Seqno last_exec);
  // Effects: Starts checking state that reflects execution up to "last_exec" 

  void check_state();
  // Effects: checks if state is correct.

  bool shutdown(FILE* o, Seqno ls);
  // Effects: Shuts down state writing value to "o"

  bool restart(FILE* i, PBFT_R_Replica *rep, Seqno ls, Seqno le, bool corrupt); 
  // Effects: Restarts the state object from value in "i"

  bool enforce_bound(Seqno b, Seqno ks, bool corrupt);
  // Effects: Enforces that there is no information above bound
  // "b". "ks" is the maximum sequence number that I know is stable.

  // PBFT_R_Message handlers
  void handle(PBFT_R_Meta_data *m);
  void handle(PBFT_R_Meta_data_d *m);
  void handle(PBFT_R_Data *m);

  bool handle(PBFT_R_Fetch *m, Seqno last_stable);
  // Effects: Returns true if it was able to verify the message and
  // false otherwise.

  void mark_stale();
  // Effects: Discards incomplete certificate.

  void simulate_reboot();
  // Effects: Simulates a reboot by invalidating state

  bool retrans_fetch(PBFT_R_Time cur) const;
  // Effects: Returns true iff fetch should be retransmitted
 
#ifndef NO_STATE_TRANSLATION
  char* get_cached_obj(int i);
#endif
  
private:
  // Parent PBFT_R_replica object.
  PBFT_R_Replica* PBFT_R_replica;

  // Actual memory holding current state and the number of Blocks
  // in that memory.

#ifdef NO_STATE_TRANSLATION
  Block* mem;
#else
  Block* rep_mem;
#endif
  int nb;

  // PBFT_R_Bitmap with a bit for each block in the memory region indicating
  // whether the block should be copied the next time it is written;
  // blocks should be copied iff their bit is 0.
  PBFT_R_Bitmap cowb;
  
  Part* ptree[PLevels];        // PBFT_R_Partition tree.
  DSum* stree[PLevels-1];      // Tree of sums of digests of subpartitions.

  Log<PBFT_R_Checkpoint_rec> clog;   // PBFT_R_Checkpoint log
  Seqno lc;                   // Sequence number of the last checkpoint


  //
  // Information used while fetching state.
  //
  bool fetching;      // true iff PBFT_R_replica is fetching missing state
  bool keep_ckpts;    // whether to keep last checkpoints
  int flevel;         // level of state partition info being fetched  
  FPartQueue* stalep[PLevels]; // queue of out-of-date partitions for each level

#ifndef NO_STATE_TRANSLATION
  Page_mapping *fetched_pages;  // set of fetched pages in a fetch operation

  PageCache *pages_lc;        // copies of pages at last checkpoint
#endif
    
  PBFT_R_Meta_data_cert* cert;  // certificate for partition we are working on
  int lreplier;          // id of last PBFT_R_replica we chose as replier
  PBFT_R_Time last_fetch_t;     // PBFT_R_Time when last fetch was sent. 


  //
  // Information used while checking state during recovery
  //
  bool checking;       // true iff PBFT_R_replica is checking state
  Seqno check_start;   // last checkpoint sequence number when checking started
  bool corrupt;        // true iff PBFT_R_replica's state is known to be corrupt
  int poll_cnt;        // check for messages after digesting this many blocks

  // queue of partitions whose digests need to be checked. It can have
  // partitions from different levels.
  CPartQueue *to_check;   
  int lchecked;      // index of last block checked in to_check.high().
  int refetch_level; // level of ancestor of current partition whose subpartitions 
                     // have already been added to to_check.
#ifndef NO_STATE_TRANSLATION
  int total_size;    // Total size of object being fetched
  int next_chunk;    // Next fragment number to be fetched
  char *reassemb;    // Buffer for reassembling the fragments
#endif

#ifndef NO_STATE_TRANSLATION
  int (*get_segment)(int, char **);
  void (*put_segments)(int, int *, int *, char **);
  void (*shutdown_proc)(FILE *o);
  void (*restart_proc)(FILE *i);
#endif

  int digest(Digest &d, int l, int i);
  // Effects: Sets "d" to the current digest of partition  "(l,i)"
  // Returns: size of object in partition (l,i)

  void digest(Digest& d, int i, Seqno lm, char *data, int size);
  // Effects: Sets "d" to MD5(i#lm#(data,size))

  bool check_digest(Digest& d, PBFT_R_Meta_data* m);
  // Effects: Checks if the digest of the partion in "m" is "d"

  void done_with_level();
  // Requires: flevel has an empty out-of-date queue.
  // Effects: It decrements flevel and, if parent is consistent,
  // removes parent. If the queue of parent becomes empty it calls
  // itself recursively. Unless there is no parent, in which case it
  // in_fetch_state_to false and updates state accordingly.

  void update_ptree(Seqno n);
  // Effects: Updates the digests of the blocks whose cow bits were reset
  // since the last checkpoint and computes a new state digest using the
  // state digest computed during the last checkpoint.

#ifndef NO_STATE_TRANSLATION
  char* get_data(Seqno c, int i, int &objsz);
#else
  char* get_data(Seqno c, int i);
#endif
  // Requires: There is a checkpoint with sequence number "c" in this
  // Effects: Returns a pointer to the data for block index "i" at
  // checkpoint "c". [objsz gets the size of the object]

  Part& get_meta_data(Seqno c, int l, int i);
  // Requires: There is a checkpoint with sequence number "c" in this
  // Effects: Returns a pointer to the information for partition "(l,i)" at
  // checkpoint "c".

  bool check_data(int i);
  // Effects: Checks whether the actual digest of block "i" and its
  // digest in the ptree match.

};

inline bool PBFT_R_State::in_fetch_state() const { return fetching; }

inline bool PBFT_R_State::in_check_state() const { return checking; }

inline bool PBFT_R_State::retrans_fetch(PBFT_R_Time cur) const { 
  return fetching && diffPBFT_R_Time(cur, last_fetch_t) > 100000;
}

#endif // _PBFT_R_State_h



