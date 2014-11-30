#ifndef _PBFT_R_PBFT_LIBBYZ_H
#define _PBFT_R_PBFT_LIBBYZ_H

#ifdef __cplusplus
extern "C"
{
#endif

#ifndef __cplusplus
	typedef unsigned long bool;
#endif

	/* Because of FILE parameter */
#include <stdio.h>

#include "types.h"
	//#include "PBFT_R_Modify.h"
	//#include "Digest.h"
#include "PBFT_R_State_defs.h"
#include "libbyz.h"

	/*
	 * PBFT_R_Client
	 */

	int PBFT_R_pbft_init_client(char *conf, char *conf_priv, char *host_name,
			short port);
	/* Effects: Initializes a libbyz client process using the information in the file
	 named by "conf" and the private key in the file named by "conf_priv".
	 If port is 0 the library will select the first line matching this
	 host in "conf". Otherwise, it selects the line with port value "port". */

	int PBFT_R_pbft_alloc_request(Byz_req *req);
	/* Requires: "req" points to a Byz_req structure
	 Effects: If successful returns 0 and initializes "req" by allocating internal
	 memory for the request, making "req->contents" point to that memory, and "req->size"
	 contain the number of bytes that can be used starting from "req->contents". If it fails
	 it returns -1. */

	void PBFT_R_pbft_free_request(Byz_req *req);
	/* Requires: "req" points to a Byz_req structure whose "req->contents" value
	 was obtained by calling Byz_alloc_req.
	 Effects: Frees the internal memory associated with "req". */

	void PBFT_R_pbft_free_reply(Byz_rep *rep);
	/* Requires: "rep" points to a Byz_rep structure whose "req->contents" value
	 was obtained by calling Byz_recv_reply.
	 Effects: Frees the internal memory associated with "rep". */

	int PBFT_R_pbft_send_request(Byz_req *req, bool ro);
	/* Requires: "req" points to a Byz_req structure whose "req->contents"
	 value was obtained by calling Byz_alloc_req and whose "req->size"
	 value is the actual number of bytes in the request.
	 "read_only" is true iff the request
	 does not modify the service state. All previous request have been
	 followed by an invocation of Byz_recv_reply.

	 Effects: Invokes the request. If successful, returns 0.
	 Otherwise returns -1. */

	int PBFT_R_pbft_recv_reply(Byz_rep *rep);
	/* Requires: "rep" points to an uninitialized Byz_rep structure.
	 There was a previous request for which there was not an invocation
	 of Byz_recv_reply.

	 If successful, initializes "rep" to
	 point to the reply and returns 0. ("rep" must be deallocated by the
	 caller using Byz_free_reply.) Otherwise, does not initialize "rep"
	 and returns -1. */

	int PBFT_R_pbft_invoke(Byz_req *req, Byz_rep *rep, bool ro);
	/* Requires: "req" points to a Byz_req structure whose "req->contents"
	 value was obtained by calling Byz_alloc_req and whose "req->size"
	 value is the actual number of bytes in the request.
	 "read_only" is true iff the request
	 does not modify the service state. All previous request have been
	 followed by an invocation of Byz_recv_reply.
	 "rep" points to an uninitialized Byz_rep structure.

	 Effects: Invokes the request. If successful, initializes "rep" to
	 point to the reply and returns 0. ("rep" must be deallocated by the
	 caller using Byz_free_reply.) Otherwise, does not initialize "rep"
	 and returns -1. */

	/*
	 * PBFT_R_Replica
	 */

#ifndef NO_STATE_TRANSLATION

	int PBFT_R_pbft_init_replica(char *conf, char *conf_priv, unsigned int num_objs,
			int (*exec)(Byz_req*, Byz_rep*, Byz_buffer*, int, bool),
			void (*comp_ndet)(Seqno, Byz_buffer *), int ndet_max_len,
			bool (*check_ndet)(Byz_buffer *),
			int (*get_obj)(int, char **),
			void (*put_objs)(int, int *, int *, char **),
			void (*shutdown_proc)(FILE *o),
			void (*restart_proc)(FILE *i),
			short port);

	/* Effects: Initializes a libbyz PBFT_R_replica process using the information
	 in the file named by "conf" and the private key in the file named
	 by "conf_priv".  The state managed by the PBFT_R_replica consists of a
	 total number of "num_objs" objects, and the PBFT_R_replica will call
	 the "exec" upcall to execute requests and the "comp_ndet" upcall to
	 compute non-deterministic choices for each request. Those choices
	 can be validated at the remaining PBFT_R_replicas using "check_ndet".
	 "ndet_max_len" must be the maximum number of bytes comp_ndet places
	 in its argument buffer.  In order to manage its abstract state, the
	 library will call the abstraction function "get_obj" and its
	 inverse "put_objs".  Before a recovery, the PBFT_R_replica will call
	 the "shutdown_proc" upcall which allows arbitrary info to be saved
	 to disk and when restarting, that information can ve retreived when
	 the "restart_proc" is called. If not successful, the function
	 returns -1 and a different value otherwise.

	 The specs for the upcalls are:
	 int exec(Byz_req *req, Byz_rep *rep, Byz_buffer *ndet,
	 int client, bool read_only);

	 Effects:
	 - "req->contents" is a character array with a request with
	 "req->size" bytes

	 - "rep->contents" is a character array where exec should place the
	 reply to the request. This reply cannot excede the value of
	 "rep->size" on entry to the exec. On exit from exec, "rep->size"
	 must contain the actual number of bytes in the reply.

	 - "ndet->contents" is a character array with non-deterministic
	 choices associated with the request and is "ndet->size" bytes long

	 - "client" is the identifier of the client that executed the
	 request (index of client's public key in configuration file)

	 - "read_only" is true iff the request should execute only if it does
	 not modify the PBFT_R_replica's state.

	 If "read_only" is true "exec" should not execute the request in
	 "req" unless it is in fact read only. If the request is not read
	 only it should return -1 without modifying the service
	 state. Except for this case exec should execute the request in req
	 using the non-deterministic choices and place the replies in
	 rep. The execution of the request will typically require access
	 control checks using the client identifier. If the request executes
	 successfully exec should return 0.


	 void comp_ndet(Seqno seqno, Byz_buffer *ndet);
	 Effects: "ndet->contents" is a character array where comp_ndet
	 should place the non-deterministic choices (e.g., time) associated
	 with the request with sequence number seqno. These choices cannot
	 excede the value of "ndet->size" on entry to the comp_ndet. On exit
	 from comp_ndet, "ndet->size" must contain the actual number of
	 bytes in the choices.

	 bool check_ndet(Byz_buffer *ndet);
	 Effects: returns TRUE iff PBFT_R_replica decides to accept the nondet choice
	 contained in ndet.

	 int get_obj(int i, char **obj);
	 Effects: Allocates a buffer and places a pointer to it in "*obj",
	 obtains the value of the abstract object with index "i",
	 and places that value in the buffer.  Returns the size of
	 the object.

	 void put_objs(int totaln, int *sizes, int *indices, char **objs);
	 Effects: receives a vector of "totaln" objects in "objs" with the
	 corresponding indices and sizes. This upcall causes the
	 application to update its concrete state using the new
	 values for the abstract objects passed as arguments. The
	 library guarantees that the upcall is invoked with an
	 argument that brings the abstract state of the PBFT_R_replica to
	 a consistent value (i.e., the value of a valid
	 checkpoint).


	 void shutdown_proc(FILE *o);
	 Effects: saves persistent recovery info to "o"

	 void restart_proc(FILE *i);
	 Effects: read persistent recovery info from "i"

	 */

	/*
	 * The service code should call the following function before
	 * it modifies the state managed by the PBFT_R_replica.
	 *
	 */

	void PBFT_R_pbft_modify(int npages, int* pages);
	/* Requires: "pages" is an array of "npages" integers. All the elements of the
	 array are valid page numbers for the PBFT_R_replica's state (i.e. they
	 are between 0 and the num_objs-1, where num_objs is the
	 total number of objects in the application state, as defined in
	 the PBFT_R_replica's initialization
	 Effects:  Informs library that the pages in pages[0..npages-1]
	 are about to be modified. */

#else // ifndef NO_STATE_TRANSLATION
	int PBFT_R_pbft_init_replica(char *conf, char *conf_priv, char *host_name,
			char *mem, unsigned int size, int (*exec)(Byz_req *, Byz_rep *, Byz_buffer *, int, bool), void (*comp_ndet)(Seqno, Byz_buffer *), int ndet_max_len);
	/* Requires: "mem" is vm page aligned and "size" is a multiple of the vm page size.

	 Effects: Initializes a libbyz PBFT_R_replica process using the information in the file
	 named by "conf" and the private key in the file named by "conf_priv".
	 The state managed by the PBFT_R_replica is set to the "size" contiguous bytes starting
	 at "mem", and the PBFT_R_replica will call the "exec" upcall to execute requests and
	 the "comp_ndet" upcall to compute non-deterministic choices for each request.
	 "ndet_max_len" must be the maximum number of bytes comp_ndet places in its argument
	 buffer. The PBFT_R_replication code uses the begining of "mem" to store protocol data.
	 If successful, the function returns the number of bytes used which is guaranteed
	 to be a multiple of the vm page size. Otherwise, the function returns -1.

	 The specs for the upcalls are:
	 int exec(Byz_req *req, Byz_rep *rep, Byz_buffer *ndet, int client, bool read_only);

	 Effects:
	 - "req->contents" is a character array with a request with
	 "req->size" bytes

	 - "rep->contents" is a character array where exec should place the
	 reply to the request. This reply cannot excede the value of
	 "rep->size" on entry to the exec. On exit from exec, "rep->size"
	 must contain the actual number of bytes in the reply.

	 - "ndet->contents" is a character array with non-deterministic
	 choices associated with the request and is "ndet->size" bytes long

	 - "client" is the identifier of the client that executed the
	 request (index of client's public key in configuration file)

	 - "read_only" is true iff the request should execute only if it does
	 not modify the PBFT_R_replica's state.

	 If "read_only" is true "exec" should not execute the request in
	 "req" unless it is in fact read only. If the request is not read
	 only it should return -1 without modifying the service
	 state. Except for this case exec should execute the request in req
	 using the non-deterministic choices and place the replies in
	 rep. The execution of the request will typically require access
	 control checks using the client identifier. If the request executes
	 successfully exec should return 0.


	 void comp_ndet(Seqno seqno, Byz_buffer *ndet);
	 Effects: "ndet->contents" is a character array where comp_ndet
	 should place the non-deterministic choices (e.g., time) associated
	 with the request with sequence number seqno. These choices cannot
	 excede the value of "ndet->size" on entry to the comp_ndet. On exit
	 from comp_ndet, "ndet->size" must contain the actual number of
	 bytes in the choices.

	 */

	/*
	 * The service code should call one of the following functions before
	 * it modifies the state managed by the PBFT_R_replica.
	 *
	 */

	void PBFT_R_pbft_modify(char *mem, int size);
	/* Requires: "mem" and "mem+size-1" are within the PBFT_R_replica's state.
	 Effects: Informs library that the bytes between "mem" and
	 "mem+size" are about to be modified. */

#endif

	void PBFT_R_pbft_replica_run();
	/* Effects: Loops executing requests. */

	void PBFT_R_pbft_reset_stats();
	/* Effects: Resets library's statistics counters */

	void PBFT_R_pbft_print_stats();
	/* Effects: Print library statistics to stdout */

	void PBFT_R_pbft_reset_client();
/* Reverts client to its initial state to ensure independence of experimental
 points */

#ifndef NO_STATE_TRANSLATION
char* Byz_get_cached_object(int i);
#endif

#ifdef __cplusplus
}
#endif

#endif /* _PBFT_R_PBFT_LIBBYZ_H */
