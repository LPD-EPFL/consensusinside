#include <stdlib.h>
#include <strings.h>
#include "th_assert.h"
#include "R_Message_tags.h"
#include "R_Request.h"
#include "R_Node.h"
#include "R_Principal.h"
#include "R_ACK.h"

#include "MD5.h"

// extra & 1 = read only --> NOT USED YET IN OUR IMPLEMENTATION
// extra & 4 = INIT request

R_Request::R_Request(int sz) :
	R_Message(R_Request_tag, sz)
{
	seqnos_size = sizeof(Seqno)*(R_node->f()+1);
}

R_Request::R_Request(Request_id r) :
	R_Message(R_Request_tag, R_Max_message_size)
{
	rep().cid = R_node->id();
	rep().rid = r;
	rep().command_size = 0;
	rep().seqno = 0;
	rep().type = 0;
	// rep().is_init = 0;
	set_size(sizeof(R_Request_rep));
	seqnos_size = sizeof(Seqno)*(R_node->f()+1);
}

R_Request::R_Request(R_Request_rep *rep)
    : R_Message((R_Message_rep*)rep)
{
    seqnos_size = sizeof(Seqno)*(R_node->f()+1);
}

R_Request::~R_Request()
{
}

void R_Request::set_seqno(Seqno seq) {
	rep().seqno = seq;
}

char *R_Request::store_command(int &max_len)
{
	max_len = msize()-sizeof(R_Request_rep)-R_node->auth_size()-seqnos_size;
	return contents()+sizeof(R_Request_rep);
}

// comp_digest will allways generate digest for request as if seqno=0
inline void R_Request::comp_digest(Digest& d)
{
	//  fprintf(stderr, "Computing MD5 digest (inputLen = %d)\n", sizeof(int)
	//        +sizeof(Request_id)+ rep().command_size);INCR_OP(num_digests);START_CC(digest_cycles);
	MD5_CTX context;
	MD5Init(&context);
	MD5Update(&context, (char*)(&(rep().cid)),
		sizeof(int) + sizeof(Request_id)+sizeof(int)+sizeof(short)+ rep().command_size);
	MD5Final(d.udigest(), &context);
}

void R_Request::finalize(int act_len, bool read_only)
{
	th_assert((unsigned)act_len <= msize() - sizeof(R_Request_rep)
			- R_node->authenticators_size(), "Invalid request size");

	//  rep().extra = ((read_only) ? 1 : 0);
	rep().extra &= ~1;
	if (read_only)
	{
		rep().extra = rep().extra | 1;
	}
	rep().command_size = act_len;
	comp_digest(rep().od);

	set_size(sizeof(R_Request_rep) + act_len + R_node->auth_size() + seqnos_size);
}

void R_Request::authenticate_for_client(int id, int authentication_offset, int *new_offset, R_Reply_rep *rr, int iteration)
{
       if (R_node->id() == replica_id())
       	   return;
       int ndistance = (R_node->f()+1) - R_node->distance(R_node->id(), replica_id());
       if (ndistance < 0 || ndistance > R_node->f())
	   return;

       //fprintf(stderr, "R_Replica[%d]: authenticate_for_client %d ndistance %d, replica_id %d, rr = %p\n", R_node->id(), client_id(), ndistance, replica_id(), rr);

       int distance = R_node->distance(replica_id(),id);
       if (id == -1) {
           set_stored_seqno(0,seqno());
       } else  if (distance==0 && iteration == 1)
           set_stored_seqno(1, seqno());
       else {
           roll_stored_seqno();
           set_stored_seqno(R_node->f(), seqno());
       }

       *new_offset = ndistance;
       ndistance *= (R_UMAC_size + R_UNonce_size);
       // this is only for type, Seqno and Digest of the request.
       //fprintf(stderr, "R_Replica[%d]: authenticating for client %d\n", R_node->id(), client_id());
       R_node->gen_auth((char*)&(rep().type), sizeof(uint32_t)+sizeof(Seqno)+sizeof(Digest), contents()
                   + sizeof(R_Request_rep) + rep().command_size + seqnos_size + authentication_offset + ndistance, rr, id, replica_id(), client_id(), iteration, 1); // skip one
}

void R_Request::authenticate(int id, int authentication_offset, R_Reply_rep *rr, int iteration)
{
	int distance = R_node->distance(replica_id(),id);
	if (id == -1) {
	    set_stored_seqno(0,seqno());
	} else  if (distance<R_node->f() && iteration == 1)
	    set_stored_seqno(distance+1, seqno());
	else {
	    roll_stored_seqno();
	    set_stored_seqno(R_node->f(), seqno());
	}

	if (is_batched()) {
	    char *toauth = (char*)malloc(32*sizeof(Digest)+sizeof(uint32_t)+sizeof(Seqno));
	    *((uint32_t*)toauth) = rep().type;
	    *((Seqno*)(toauth+sizeof(uint32_t))) = rep().seqno;
	    int actpos = sizeof(uint32_t)+sizeof(Seqno);

	    R_Request::Iterator rit = begin();
	    while (rit != end()) {
		    //fprintf(stderr, "Doing one req %d\n", rit.data.index);
		memcpy(toauth+actpos, (char*)&(rit.data.req->rep().od), sizeof(Digest));
		actpos += sizeof(Digest);

		++rit;
	    }

	    R_node->gen_auth(toauth, actpos, contents()
		    + sizeof(R_Request_rep) + rep().command_size + seqnos_size + authentication_offset, rr, id, replica_id(), client_id(), iteration);
	    free(toauth);
	}
	else {
	    // this is only for type, Seqno and Digest of the request.
	    R_node->gen_auth((char*)&(rep().type), sizeof(uint32_t)+sizeof(Seqno)+sizeof(Digest), contents()
		    + sizeof(R_Request_rep) + rep().command_size + seqnos_size + authentication_offset, rr, id, replica_id(), client_id(), iteration);
	}
}

int R_Request::copy_authenticators(char *dest)
{
	int size = R_node->authenticators_size();
	memcpy(dest, contents()+sizeof(R_Request_rep)+rep().command_size, size);
	return size;
}

int R_Request::patch_authenticators_for_client(char *dest, int offset)
{
    // XXX: only works for f() == 1
	int pos = R_node->f()-offset;
	pos *= (R_UMAC_size + R_UNonce_size);
	pos += (R_UNonce_size + R_UNonce_size);
	memcpy(dest+seqnos_size+pos, contents()+sizeof(R_Request_rep)+rep().command_size+seqnos_size+pos, R_UMAC_size);
	return R_UMAC_size;
}

void R_Request::replace_authenticators(char *src)
{
	int size = R_node->authenticators_size();
	memcpy(contents()+sizeof(R_Request_rep)+rep().command_size, src, size);
}

void R_Request::swap_seqno(Seqno newseqno, Seqno *oldseqno)
{
	if (oldseqno) *oldseqno = rep().seqno;
	rep().seqno = newseqno;
}

Seqno R_Request::get_stored_seqno(int index)
{
	Seqno s;
	memcpy(&s, contents()+sizeof(R_Request_rep)+rep().command_size+sizeof(Seqno)*index, sizeof(Seqno));
	return s;
}

void R_Request::set_stored_seqno(int index, Seqno s)
{
//fprintf(stderr, "setting seqno %lld at position %d\n", s , index);
	memcpy(contents()+sizeof(R_Request_rep)+rep().command_size+sizeof(Seqno)*index, &s, sizeof(Seqno));
}

void R_Request::roll_stored_seqno()
{
//fprintf(stderr, "rolling stored seqno\n");
	memcpy(contents()+sizeof(R_Request_rep)+rep().command_size,
		contents()+sizeof(R_Request_rep)+rep().command_size+sizeof(Seqno),
		seqnos_size-sizeof(Seqno));
}

// just verifies digest and possible client authenticators
bool R_Request::verify_simple(int *authentication_offset, int iteration)
{
	Digest d;
	comp_digest(d);

	if (d != rep().od)
	{
		fprintf(stderr, "Digest is different\n");
		return false;
	}

	*authentication_offset = 0;
	if (!R_node->verify_client_auth(R_node->id(), replica_id(), client_id(), (char*)&(rep().type), sizeof(uint32_t)+sizeof(Seqno)+sizeof(Digest), contents() + sizeof(R_Request_rep) + rep().command_size+seqnos_size, authentication_offset, this, iteration))
	{
		fprintf(stderr, "verify_client_auth returned false\n");
		return false;
	}

	return true;
}

bool R_Request::verify(int *authentication_offset, int iteration)
{
	Digest d;
	comp_digest(d);

	if (d != rep().od)
	{
		fprintf(stderr, "Digest is different\n");
		return false;
	}

	if (is_batched()) {
	    *authentication_offset = 0;
	    if (!verify_simple(authentication_offset, iteration)) {
		fprintf(stderr, "Verify_simple failed for carrying request\n");
		return false;
	    }

	    bool retval = true;

	    char *toauth = (char*)malloc(32*sizeof(Digest)+sizeof(uint32_t)+sizeof(Seqno));
	    *((uint32_t*)toauth) = rep().type;
	    *((Seqno*)(toauth+sizeof(uint32_t))) = rep().seqno;
	    int actpos = sizeof(uint32_t)+sizeof(Seqno);

	    R_Request::Iterator rit = begin();
	    while (rit != end()) {
		if (rit.data.req != rit.data.oreq) {
		    int ao = 0;
		    bool rv = rit.data.req->verify_simple(&ao, iteration);
		    if (!rv) {
			fprintf(stderr, "Verify_simple failed for batched request at index %d\n", rit.data.index);
			free(toauth);
			return false;
		    }
		    retval = retval && rv;
		}
		memcpy(toauth+actpos, (char*)&(rit.data.req->rep().od), sizeof(Digest));
		actpos += sizeof(Digest);

		++rit;
	    }

	    int toskip = 0;
	    if (iteration == 1 && R_node->distance(replica_id(), R_node->id()) < R_node->f()+1)
		toskip = 1;
	    if (!R_node->verify_batched_auth(R_node->id(), replica_id(), client_id(), toauth, actpos, contents() + sizeof(R_Request_rep) + rep().command_size+seqnos_size, authentication_offset, this, iteration, toskip))
	    {
		fprintf(stderr, "Verify_batched_auth returned false\n");
		free(toauth);
		return false;
	    }
	    free(toauth);
	}
	else {
	    if (!R_node->verify_auth(R_node->id(), replica_id(), client_id(), (char*)&(rep().type), sizeof(uint32_t)+sizeof(Seqno)+sizeof(Digest), contents() + sizeof(R_Request_rep) + rep().command_size+seqnos_size, authentication_offset, this, iteration))
	    {
		fprintf(stderr, "Verify_auth returned false\n");
		return false;
	    }
	}

	return true;
}

bool R_Request::convert(R_Message *m1, R_Request *&m2)
{

	if (!m1->has_tag(R_Request_tag, sizeof(R_Request_rep)))
	{
		return false;
	}

	// m2 = (R_Request*)m1;
	//  m2 = new R_Request((R_Request_rep *)m1->contents());
	//   m2->trim();
	m2 = new R_Request(m1->size());
	memcpy(m2->contents(), m1->contents(), m1->size());
	//free(m1->contents());
	// TODO : VIVIEN WE SHOULD NOT COMMENT THIS LINE (THIS CREATES A LIBC ERROR).
	delete m1;
	m2->trim();
	return true;
}

// Iterator is responsible for deleting the request it created.
R_Request::Iterator& R_Request::Iterator::operator++()
{
    //fprintf(stderr, "======================== operator++ called ===========================\n");
    //fprintf(stderr, "Params are: offset %d, index %d, oreq %p, req %p\n", data.offset, data.index, data.oreq, data.req);
    //fprintf(stderr, "\t\tsize is %d, ?= (%d + %d + %d)\n", data.oreq->size(), sizeof(R_Request_rep),data.oreq->rep().command_size,R_node->authenticators_size());
    R_Request_rep *rp;
    int request_size = 0;
    if (data.req && data.req != data.oreq) {
	request_size = data.req->size();
    }

    if (!data.oreq->is_batched()
	    || data.offset+request_size >= data.oreq->size()
	    || data.oreq->size() <= ALIGNED_SIZE(sizeof(R_Request_rep)+data.oreq->rep().command_size+R_node->authenticators_size())) {
	//fprintf(stderr, "RETURN end()\n");
	data.index = -1;
	data.offset = -1;
	if (data.req && data.req != data.oreq)
	    delete data.req;
	data.req = NULL;
	return *this;
    }

    if (data.index == 0) {
	char *ptr = data.oreq->contents()+ALIGNED_SIZE(sizeof(R_Request_rep)+data.oreq->rep().command_size+R_node->authenticators_size());
	rp = (R_Request_rep*)ptr;
    } else {
	rp = (R_Request_rep*)(data.oreq->contents()+data.offset+data.req->size());
    }
    int size = rp->size;
    // fprintf(stderr, "This request is of size %d, at %p\n", size, rp);
    th_assert(size != 0, "Size should not be 0");
    R_Request *rn = new R_Request(rp);
    //memcpy(rn->contents(), (char*)(rp), size);

    if (data.index == 0) {
	data.offset = ALIGNED_SIZE(sizeof(R_Request_rep)+data.oreq->rep().command_size+R_node->authenticators_size());
    } else {
	data.offset += ALIGNED_SIZE(data.req->size());
    }
    if (data.req && data.req != data.oreq)
	delete data.req;
    data.req = rn;
    data.index++;
    return *this;
}
