#ifndef _LIBBYZ_H
#define _LIBBYZ_H

#ifdef __cplusplus
extern "C"
{
#endif

#ifndef __cplusplus
typedef unsigned long bool;
enum bool_values
{	false = 0, true = 1};
#endif

/* Because of FILE parameter */
#include <stdio.h>

#include "types.h"
#include "Traces.h"

/* Should be a power of 2 less than or equal to the vm page size */
static const int Block_size = 4096;

enum protocols_e
{	quorum = 1, chain = 2, pbft = 4, zlight = 8, ring = 16 , tpc = 32, oneacceptor = 64, multipaxos = 128};

extern enum protocols_e current_protocol;
extern enum protocols_e next_protocol_instance;

struct _Byz_buffer
{
	int size;
	char *contents;
	void *opaque;
};

typedef struct _Byz_buffer Byz_buffer;
typedef struct _Byz_buffer Byz_req;
typedef struct _Byz_buffer Byz_rep;

#ifdef __cplusplus
}
#endif

struct OutstandingRequests {
    int cid;
    Request_id rid;
};

#endif /* _LIBBYZ_H */
