#include <strings.h>
#include "th_assert.h"
#include "R_Message_tags.h"
#include "R_Reply.h"
#include "R_Node.h"
#include "R_Principal.h"

R_Reply::R_Reply() :
	R_Message(R_Reply_tag, R_Max_message_size)
{
}

R_Reply::R_Reply(R_Reply_rep *r) :
	R_Message(r)
{
}

R_Reply::~R_Reply()
{
}

char *R_Reply::store_reply(int &max_len)
{
	max_len = msize()-sizeof(R_Reply_rep)- (R_node->f() + 1) * (R_UNonce_size
			+ R_UMAC_size);

	return contents()+sizeof(R_Reply_rep);
}

bool R_Reply::verify()
{
	// Check sizes
	int rep_size = rep().reply_size;
	if (size()-(int)sizeof(R_Reply_rep)-rep_size < (R_node->f() + 1)
			* (R_UNonce_size + R_UMAC_size))
	{
		return false;
	}

	// Check reply
	Digest d(contents()+sizeof(R_Reply_rep), rep_size);
	if (d != rep().digest)
	{
		fprintf(stderr, "R_Reply::verify: Digest does not match\n");
		return false;
	}

	// Check signature.
	int size_wo_MAC = sizeof(R_Reply_rep) + rep_size;

	int unused;
	bool ret = R_node->verify_auth(R_node->n(), rep().replica, R_node->id(),
		    (char*)&(rep().digest), sizeof(Digest), contents()+size_wo_MAC, &unused, NULL, 3);

	return ret;
}

bool R_Reply::convert(R_Message *m1, R_Reply *&m2)
{
	if (!m1->has_tag(R_Reply_tag, sizeof(R_Reply_rep)))
	{
		return false;
	}
	//  m1->trim();
	// m2 = (R_Reply*)m1;
	// m2 = new R_Reply((R_Reply_rep *) m1->contents());
	m2=new R_Reply();
	memcpy(m2->contents(), m1->contents(), m1->size());
	//free(m1->contents());
	delete m1;
	m2->trim();
	return true;
}
