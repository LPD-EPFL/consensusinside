#include <strings.h>
#include "th_assert.h"
#include "C_Message_tags.h"
#include "C_Reply.h"
#include "C_Node.h"
#include "C_Principal.h"

C_Reply::C_Reply() :
	C_Message(C_Reply_tag, C_Max_message_size)
{
}

C_Reply::C_Reply(C_Reply_rep *r) :
	C_Message(r)
{
}

C_Reply::~C_Reply()
{
}

char *C_Reply::store_reply(int &max_len)
{
	max_len = msize()-sizeof(C_Reply_rep)- (C_node->f() + 1) * (C_UNonce_size
			+ C_UMAC_size);

	return contents()+sizeof(C_Reply_rep);
}

bool C_Reply::verify()
{
	// Replies must be sent by replicas.
	if (!C_node->is_replica(id()))
	{
		return false;
	}

	// Check sizes
	int rep_size = rep().reply_size;
	if (size()-(int)sizeof(C_Reply_rep)-rep_size < (C_node->f() + 1)
			* (C_UNonce_size + C_UMAC_size))
	{
		return false;
	}

	// Check reply
	Digest d(contents()+sizeof(C_Reply_rep), rep_size);
	if (d != rep().digest)
	{
		fprintf(stderr, "C_Reply::verify: Digest does not match\n");
		return false;
	}

	// Check signature.
	int size_wo_MAC = sizeof(C_Reply_rep) + rep_size;

	int unused;
	bool ret = C_node->verify_auth(C_node->n(), unused, contents(),
			sizeof(C_Reply_rep), contents() +size_wo_MAC, &unused);

	//TODO these if condition must be removed
	if (rep().cid !=-1)
	{
		return true;
	}
	return ret;
}

bool C_Reply::convert(C_Message *m1, C_Reply *&m2)
{
	if (!m1->has_tag(C_Reply_tag, sizeof(C_Reply_rep)))
	{
		return false;
	}
	//  m1->trim();
	// m2 = (C_Reply*)m1;
	// m2 = new C_Reply((C_Reply_rep *) m1->contents());
	m2=new C_Reply();
	memcpy(m2->contents(), m1->contents(), m1->size());
	//free(m1->contents());
	delete m1;
	m2->trim();
	return true;
}
