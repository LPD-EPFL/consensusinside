#include <stdlib.h>
#include <strings.h>
#include "th_assert.h"
#include "R_Message_tags.h"
#include "R_ACK.h"
#include "R_Node.h"
#include "R_Principal.h"
#include "MD5.h"

// extra & 1 = read only --> NOT USED YET IN OUR IMPLEMENTATION
// extra & 4 = INIT request

R_ACK::R_ACK() :
	R_Message(R_ACK_tag, R_Max_message_size)
{
}

R_ACK::R_ACK(int cid, Seqno& seqno, Request_id& r, Digest& d) :
	R_Message(R_ACK_tag, R_Max_message_size)
{
	rep().cid = cid;
	rep().rid = r;
	rep().seqno = seqno;
	rep().od = d;
	set_size(sizeof(R_ACK_rep)+R_node->authenticators_size());
}

R_ACK::R_ACK(R_ACK_rep *rep) :
	R_Message((R_Message_rep*)rep)
{
	set_size(sizeof(R_ACK_rep)+R_node->authenticators_size());
}

R_ACK::~R_ACK()
{
}

inline void R_ACK::set_digest(Digest& d)
{
	//  fprintf(stderr, "Computing MD5 digest (inputLen = %d)\n", sizeof(int)
	//        +sizeof(Request_id)+ rep().command_size);INCR_OP(num_digests);START_CC(digest_cycles);
	rep().od = d;
}

bool R_ACK::convert(R_Message *m1, R_ACK *&m2)
{

	if (!m1->has_tag(R_ACK_tag, sizeof(R_ACK_rep)))
	{
		return false;
	}

	m2 = new R_ACK();
	memcpy(m2->contents(), m1->contents(), m1->size());
	delete m1;
	m2->trim();
	return true;
}

// Iterator is responsible for deleting the request it created.
R_ACK::Iterator& R_ACK::Iterator::operator++()
{
    //fprintf(stderr, "======================== operator++ called ===========================\n");
    //fprintf(stderr, "Params are: offset %d, index %d, oreq %p, req %p\n", data.offset, data.index, data.oreq, data.req);
    //fprintf(stderr, "\t\tsize is %d, ?= (%d + %d + %d)\n", data.oreq->size(), sizeof(R_ACK_rep),data.oreq->rep().command_size,R_node->authenticators_size());
    R_ACK_rep *rp;
    int request_size = 0;
    if (data.req && data.req != data.oreq) {
	request_size = data.req->size();
    }

    if (!(data.oreq->is_ack_batched() || data.oreq->is_batched())
	    || data.offset+request_size >= data.oreq->size()
	    || data.oreq->size() <= ALIGNED_SIZE(sizeof(R_ACK_rep)+R_node->authenticators_size())) {
	//fprintf(stderr, "RETURN end()\n");
	data.index = -1;
	data.offset = -1;
	if (data.req && data.req != data.oreq)
	    delete data.req;
	data.req = NULL;
	return *this;
    }

    if (data.index == 0) {
	char *ptr = data.oreq->contents()+ALIGNED_SIZE(sizeof(R_ACK_rep)+R_node->authenticators_size());
	rp = (R_ACK_rep*)ptr;
    } else {
	rp = (R_ACK_rep*)(data.oreq->contents()+data.offset+data.req->size());
    }
    int size = rp->size;
    //fprintf(stderr, "This request is of size %d, at %p\n", size, rp);
    R_ACK *rn = new R_ACK(rp);
    //memcpy(rn->contents(), (char*)(rp), size);

    if (data.index == 0) {
	data.offset = ALIGNED_SIZE(sizeof(R_ACK_rep)+R_node->authenticators_size());
    } else {
	data.offset += ALIGNED_SIZE(data.req->size());
    }
    if (data.req && data.req != data.oreq)
	delete data.req;
    data.req = rn;
    data.index++;
    return *this;
}
