#include <strings.h>
#include "th_assert.h"
#include "M_Message_tags.h"
#include "M_Reply.h"
#include "M_Node.h"
#include "M_Principal.h"

#define M_MAC_size 0

M_Reply::M_Reply() :
	M_Message(M_Reply_tag, M_Max_message_size)
{
//rep().rid = 5;
}

M_Reply::M_Reply(View view, Request_id req, int replica, //Digest &d,
		M_Principal *p, int cid) :
	M_Message(M_Reply_tag, sizeof(M_Reply_rep)+M_MAC_size)
{
	rep().v = view;
	rep().rid = req;
	rep().cid = cid;
	rep().replica = replica;
	rep().reply_size = -1;
	//rep().rh_digest.zero();
}

M_Reply::M_Reply(M_Reply_rep *r) :
	M_Message(r)
{
}

M_Reply::~M_Reply()
{
}

char* M_Reply::reply(int &len)
{
	len = rep().reply_size;
	return contents()+sizeof(M_Reply_rep);
}

char *M_Reply::store_reply(int &max_len)
{
	max_len = msize()-sizeof(M_Reply_rep)-M_MAC_size;
	return contents()+sizeof(M_Reply_rep);
}

bool M_Reply::verify()
{
	// Replies must be sent by replicas.
	if (!M_node->is_replica(id()))
	{
		fprintf(stderr, "Does not come from a replica\n");
		return false;
	}
	// Check sizes
	int rep_size = (full()) ? rep().reply_size : 0;
	if (size()-(int)sizeof(M_Reply_rep)-rep_size < M_MAC_size)
	{
		fprintf(stderr, "Its bigger than MAC\n");
		return false;
	}
	return true;
}

bool M_Reply::convert(M_Message *m1, M_Reply *&m2)
{
	if (!m1->has_tag(M_Reply_tag, sizeof(M_Reply_rep)))
	{
		return false;
	}
	m2= new M_Reply();
	memcpy(m2->contents(), m1->contents(), m1->size());
	delete m1;
	m2->trim();
	return true;
}
