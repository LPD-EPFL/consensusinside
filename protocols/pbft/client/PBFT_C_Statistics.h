#ifndef _PBFT_C_Statistics_h
#define _PBFT_C_Statistics_h

#include <sys/time.h>
#include <sys/resource.h>
#include <unistd.h>

#include "types.h"
#include "Array.h"

#include "PBFT_C_Cycle_counter.h"

struct PBFT_C_Recovery_PBFT_C_stats {
  Long shutdown_time; // Cycles spent in shutdown
  Long reboot_time;
  Long restart_time;  // Cycles spent in restart
  Long est_time;      // Cycles for estimation procedure
  Long nk_time;       // Cycles to send new key message
  Long rr_time;       // Cycles to send the recovery request

  Long check_time;    // Cycles spent checking pages
  Long num_checked;   // Number of pages checked

  Long fetch_time;    // Cycles spent fetching during recovery
  Long num_fetched;   // Number of data blocks received
  Long num_fetched_a; // Number of data blocks accepted
  Long refetched;           // Number of blocks refetched
  Long num_fetches;
  Long meta_data_fetched;   // Number of meta-data[d] messages received
  Long meta_data_fetched_a; // Number of meta-data[d] messages accepted
  Long meta_data_bytes;
  Long meta_datad_fetched;    // Number of meta-data-d messages received
  Long meta_datad_fetched_a;  // Number of meta-data-d messages accepted
  Long meta_datad_bytes;      // Number of bytes in meta-data-d blocks
  Long meta_data_refetched;   // Number of meta-data blocks refetched


  Long sys_cycles;    // Cycles spent handling syscalls during rec.

  Long rec_bin;
  Long rec_bout;

  Long rec_time;      // Cycles to complete recovery

  PBFT_C_Recovery_PBFT_C_stats();
  void print_PBFT_C_stats();
  void zero_PBFT_C_stats();
};


struct PBFT_C_Statistics {
  // Number of cycles after statistics were zeroed.
	PBFT_C_Cycle_counter cycles_after_zero;

  //
  // Authenticators:
  //
  long num_gen_auth; // Number of authenticators generated
  PBFT_C_Cycle_counter gen_auth_cycles; // and number of cycles.

  long num_ver_auth;   // Number of authenticators verified
  PBFT_C_Cycle_counter ver_auth_cycles;// and number of cycles.

  long reply_auth;   // Number of replies authenticated
  PBFT_C_Cycle_counter reply_auth_cycles; // and number of cycles.

  long reply_auth_ver;// Number of replies verified
  PBFT_C_Cycle_counter reply_auth_ver_cycles; // and number of cycles.

  //
  // Digests:
  //
  long num_digests;  // Number of digests
  PBFT_C_Cycle_counter digest_cycles; // and number of cycles.

  long pp_digest; // Number of times pre-prepare digests are computed
  PBFT_C_Cycle_counter pp_digest_cycles; // and number of cycles.


  //
  // Signatures
  //
  long num_sig_gen;  // Number of signature generations
  PBFT_C_Cycle_counter sig_gen_cycles; // and number of cycles.

  long num_sig_ver; // Number of signature verifications
  PBFT_C_Cycle_counter sig_ver_cycles; // and number of cycles.


  //
  // Recovery:
  //
  Array<PBFT_C_Recovery_PBFT_C_stats> rec_PBFT_C_stats;
  long rec_counter;  // Number of recoveries
  long rec_overlaps; // Number of recoveries that ended after executing recovery
                     // request for next replica.
  long incomplete_recs;      // Number of recoveries ended by my next recovery
  PBFT_C_Cycle_counter rec_time;    // Total cycles for recovery
  PBFT_C_Cycle_counter est_time;    // Cycles for estimation procedure
  PBFT_C_Cycle_counter nk_time;     // Cycles to send new key message
  PBFT_C_Cycle_counter rr_time;     // Cycles to send the recovery request
  long num_checked;          // Number of pages checked
  PBFT_C_Cycle_counter check_time;  // Cycles spent checking pages
  PBFT_C_Cycle_counter shutdown_time; // Cycles spent in shutdown
  PBFT_C_Cycle_counter restart_time;  // Cycles spent in restart
  PBFT_C_Cycle_counter reboot_time;

  //
  // Bandwidth:
  //
  long long bytes_in;
  long long bytes_out;

  //
  // View changes:
  //

  //
  // PBFT_C_State:
  //
  long num_fetches;              // Number of times fetch is started
  long num_fetched;              // Number of data blocks fetched
  long num_fetched_a;            // Number of data blocks accepted
  long refetched;                // Number of data refetched while checking
  long meta_data_fetched;        // Number of meta-data messages received
  long meta_data_fetched_a;      // Number of meta-data messages accepted
  long meta_data_bytes;          // Number of bytes in meta-data blocks
  long meta_datad_fetched;       // Number of meta-data-d messages received
  long meta_datad_fetched_a;     // Number of meta-data-d messages accepted
  long meta_datad_bytes;         // Number of bytes in meta-data-d blocks
  long meta_data_refetched;
  long num_ckpts;                // Number of checkpoints computed
  PBFT_C_Cycle_counter ckpt_cycles;     // and number of cycles.
  long num_rollbacks;            // Number of rollbacks
  PBFT_C_Cycle_counter rollback_cycles; // and number of cycles
  long num_cows;                 // Number of copy-on-writes
  PBFT_C_Cycle_counter cow_cycles;      // and number of cycles
  PBFT_C_Cycle_counter fetch_cycles;    // Cycles fetching state (w/o waiting)

  long cache_hits;
  long cache_misses;

  //
  // Syscalls:
  //
  long num_recvfrom;     // Number of recvfroms
  long num_recv_success; // Number of successful recvfroms
  PBFT_C_Cycle_counter recvfrom_cycles; // and number of cycles

  long num_sendto; // Number of sendtos
  PBFT_C_Cycle_counter sendto_cycles; // and number of cycles

  PBFT_C_Cycle_counter select_cycles; // Number of cycles in select
  long select_success; // Number of times select exits with fd set
  long select_PBFT_C_fail;    // Number of times select exits with fd not set.

  PBFT_C_Cycle_counter handle_timeouts_cycles;

  struct rusage ru;

  long req_retrans; // Number of request retransmissions


  PBFT_C_Statistics();
  void print_PBFT_C_stats();
  void zero_PBFT_C_stats();

  void init_rec_PBFT_C_stats();
  void end_rec_PBFT_C_stats();
};


extern PBFT_C_Statistics PBFT_C_stats;

//#define PRINT_STATS
#ifdef PRINT_STATS
#define START_CC(x) PBFT_C_stats.##x##.start()
#define STOP_CC(x)  PBFT_C_stats.##x##.stop()
#define INCR_OP(x) PBFT_C_stats.##x##++
#define INCR_CNT(x,y) (PBFT_C_stats.##x## += (y))
#define INIT_REC_STATS() PBFT_C_stats.init_rec_PBFT_C_stats()
#define END_REC_STATS() PBFT_C_stats.end_rec_PBFT_C_stats()
#else
#define START_CC(x)
#define STOP_CC(x)
#define INCR_OP(x)
#define INCR_CNT(x,y)
#define INIT_REC_STATS()
#define END_REC_STATS()
#endif

#endif // _PBFT_C_Statistics_h
