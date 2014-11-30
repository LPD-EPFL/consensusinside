#ifndef _CHAIN_LIBBYZ_H
#define _CHAIN_LIBBYZ_H

#ifdef __cplusplus
extern "C"
{
#endif

/* Because of FILE parameter */
#include <stdio.h>

#include "types.h"
#include "Traces.h"
#include "libbyz.h"

#ifdef __cplusplus
//class Abort_data;
#endif

int chain_alloc_request(Byz_req *req);
/* Requires: "req" points to a Byz_req structure
 Effects: If successful returns 0 and initializes "req" by allocating internal
 memory for the request, making "req->contents" point to that memory, and "req->size"
 contain the number of bytes that can be used starting from "req->contents". If it fails
 it returns -1. */

void chain_free_request(Byz_req *req);
/* Requires: "req" points to a Byz_req structure whose "req->contents" value
 was obtained by calling Byz_alloc_req.
 Effects: Frees the internal memory associated with "req". */

void chain_free_reply(Byz_rep *rep);
/* Requires: "rep" points to a Byz_rep structure whose "req->contents" value
 was obtained by calling Byz_recv_reply.
 Effects: Frees the internal memory associated with "rep". */

int chain_init_client(char *host_name, char *conf, short port);
/* Effects: Initializes a libbyz client process using the information in the file
 named by "conf" and the private key in the file named by "conf_priv".
 If port is 0 the library will select the first line matching this
 host in "conf". Otherwise, it selects the line with port value "port". */

int chain_init_replica(char *host_name, char *conf, char *conf_priv, int(*exec)(Byz_req*, Byz_rep*, Byz_buffer*,
		int, bool), short port);
/* Effects: Initializes a libbyz replica process using the information
 in the file named by "conf" and the private key in the file named
 by "conf_priv".  The state managed by the replica consists of a
 total number of "num_objs" objects, and the replica will call
 the "exec" upcall to execute requests and the "comp_ndet" upcall to
 compute non-deterministic choices for each request. Those choices
 can be validated at the remaining replicas using "check_ndet".
 "ndet_max_len" must be the maximum number of bytes comp_ndet places
 in its argument buffer.  In order to manage its abstract state, the
 library will call the abstraction function "get_obj" and its
 inverse "put_objs".  Before a recovery, the replica will call
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
 not modify the replica's state.

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
 Effects: returns TRUE iff replica decides to accept the nondet choice
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
 argument that brings the abstract state of the replica to
 a consistent value (i.e., the value of a valid
 checkpoint).


 void shutdown_proc(FILE *o);
 Effects: saves persistent recovery info to "o"

 void restart_proc(FILE *i);
 Effects: read persistent recovery info from "i"

 */

/*
 * The service code should call the following function before
 * it modifies the state managed by the replica.
 *
 */

int chain_send_request(Byz_req *req, int size, bool ro);
/* Requires: "req" points to a Byz_req structure whose "req->contents"
 value was obtained by calling Byz_alloc_req and whose "req->size"
 value is the actual number of bytes in the request.
 "read_only" is true iff the request
 does not modify the service state. All previous request have been
 followed by an invocation of Byz_recv_reply.

 Effects: Invokes the request. If successful, returns 0.
 Otherwise returns -1. */

#ifdef __cplusplus
int chain_recv_reply(Byz_rep *rep);
#endif
/* Requires: "rep" points to an uninitialized Byz_rep structure.
 There was a previous request for which there was not an invocation
 of Byz_recv_reply.

 If successful, initializes "rep" to
 point to the reply and returns 0. ("rep" must be deallocated by the
 caller using Byz_free_reply.) Otherwise, does not initialize "rep"
 and returns -1. */

int chain_invoke(Byz_req *req, Byz_rep *rep, int size, bool ro);
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

#ifdef __cplusplus
}
#endif

#endif /* _CHAIN_LIBBYZ_H */
