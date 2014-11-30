#include <strings.h>
#include "th_assert.h"
#include "T_Message_tags.h"
#include "T_Reply.h"
#include "T_Node.h"
#include "T_Principal.h"

#define T_MAC_size 0

T_Reply::T_Reply() :
	T_Message(T_Reply_tag, T_Max_message_size)
{
//rep().rid = 5;
}

T_Reply::T_Reply(View view, Request_id req, int replica, //Digest &d,
		T_Principal *p, int cid) :
	T_Message(T_Reply_tag, sizeof(T_Reply_rep)+T_MAC_size)
{
	rep().v = view;
	rep().rid = req;
	rep().cid = cid;
	rep().replica = replica;
	rep().reply_size = -1;
	//rep().rh_digest.zero();
}

T_Reply::T_Reply(T_Reply_rep *r) :
	T_Message(r)
{
}

T_Reply::~T_Reply()
{
}

char* T_Reply::reply(int &len)
{
	len = rep().reply_size;
	return contents()+sizeof(T_Reply_rep);
}

char *T_Reply::store_reply(int &max_len)
{
	max_len = msize()-sizeof(T_Reply_rep)-T_MAC_size;
	return contents()+sizeof(T_Reply_rep);
}

bool T_Reply::verify()
{
	// Replies must be sent by replicas.
	if (!T_node->is_replica(id()))
	{
		fprintf(stderr, "Does not come from a replica\n");
		return false;
	}
	// Check sizes
	int rep_size = (full()) ? rep().reply_size : 0;
	if (size()-(int)sizeof(T_Reply_rep)-rep_size < T_MAC_size)
	{
		fprintf(stderr, "Its bigger than MAC\n");
		return false;
	}
	return true;
}

bool T_Reply::convert(T_Message *m1, T_Reply *&m2)
{
	if (!m1->has_tag(T_Reply_tag, sizeof(T_Reply_rep)))
	{
		return false;
	}
	m2= new T_Reply();
	memcpy(m2->contents(), m1->contents(), m1->size());
	delete m1;
	m2->trim();
	return true;
}
