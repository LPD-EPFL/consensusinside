#include <stdlib.h>
#include <strings.h>
#include "th_assert.h"
#include "C_Message_tags.h"
#include "C_Request.h"
#include "C_Node.h"
#include "C_Principal.h"
#include "MD5.h"

// extra & 1 = read only --> NOT USED YET IN OUR IMPLEMENTATION
// extra & 4 = INIT request

C_Request::C_Request() :
	C_Message(C_Request_tag, C_Max_message_size)
{
}

C_Request::C_Request(Request_id r, short rr) :
	C_Message(C_Request_tag, C_Max_message_size)
{
	rep().cid = C_node->id();
	rep().rid = r;
	rep().replier = rr;
	rep().command_size = 0;
	// rep().is_init = 0;
	set_size(sizeof(C_Request_rep));
}

C_Request::~C_Request()
{
}

char *C_Request::store_command(int &max_len)
{
	max_len = msize()-sizeof(C_Request_rep)-C_node->auth_size();
	return contents()+sizeof(C_Request_rep);
}

inline void C_Request::comp_digest(Digest& d)
{
	//  fprintf(stderr, "Computing MD5 digest (inputLen = %d)\n", sizeof(int)
	//        +sizeof(Request_id)+ rep().command_size);INCR_OP(num_digests);START_CC(digest_cycles);
	MD5_CTX context;
	MD5Init(&context);
	MD5Update(&context, (char*)&(rep().cid), sizeof(int) + sizeof(Request_id)+ rep().command_size);
	MD5Final(d.udigest(), &context);
}

void C_Request::finalize(int act_len, bool read_only)
{
	th_assert((unsigned)act_len <= msize() - sizeof(C_Request_rep)
			- C_node->auth_size(), "Invalid request size");

	//  rep().extra = ((read_only) ? 1 : 0);
	rep().extra &= ~1;
	if (read_only)
	{
		rep().extra = rep().extra | 1;
	}
	rep().command_size = act_len;
	int listnum = rep().listnum;
	rep().listnum = 0;
	comp_digest(rep().od);

	set_size(sizeof(C_Request_rep) + act_len + C_node->auth_size());

	rep().listnum = listnum;
}

void C_Request::authenticate(int id, int authentication_offset, C_Reply_rep *rr)
{
	int listnum = rep().listnum;
	rep().listnum = 0;

	C_node->gen_auth(contents(), sizeof(C_Request_rep), contents()
			+ sizeof(C_Request_rep) + rep().command_size + authentication_offset, rr, id, client_id());

	rep().listnum = listnum;
}

int C_Request::copy_authenticators(char *dest)
{
	int size = C_node->auth_size();
	memcpy(dest, contents()+sizeof(C_Request_rep)+rep().command_size, size);
	return size;
}

void C_Request::replace_authenticators(char *src)
{
	int size = C_node->auth_size();
	memcpy(contents()+sizeof(C_Request_rep)+rep().command_size, src, size);
}

bool C_Request::verify(int *authentication_offset)
{
	int listnum = rep().listnum;
	rep().listnum = 0;

	Digest d;
	comp_digest(d);

	if (d != rep().od)
	{
		fprintf(stderr, "Digest is different\n");
		return false;
	}

	if (!C_node->verify_auth(C_node->id(), client_id(), contents(),
			sizeof(C_Request_rep), contents() + sizeof(C_Request_rep) + rep().command_size, authentication_offset))
	{
		fprintf(stderr, "Verify_auth returned false\n");
		return false;
	}

	rep().listnum = listnum;
	return true;
}

bool C_Request::convert(C_Message *m1, C_Request *&m2)
{

	if (!m1->has_tag(C_Request_tag, sizeof(C_Request_rep)))
	{
		return false;
	}

	// m2 = (C_Request*)m1;
	//  m2 = new C_Request((C_Request_rep *)m1->contents());
	//   m2->trim();
	m2 = new C_Request(true);
	memcpy(m2->contents(), m1->contents(), m1->size());
	//free(m1->contents());
	// TODO : VIVIEN WE SHOULD NOT COMMENT THIS LINE (THIS CREATES A LIBC ERROR).
	delete m1;
	m2->trim();
	return true;
}

