#include <strings.h>
#include "th_assert.h"
#include "O_Message_tags.h"
#include "O_Reply.h"
#include "O_Node.h"
#include "O_Principal.h"

#define O_MAC_size 0

O_Reply::O_Reply() :
	O_Message(O_Reply_tag, O_Max_message_size)
{
//rep().rid = 5;
}

O_Reply::O_Reply(View view, Request_id req, int replica, //Digest &d,
		O_Principal *p, int cid) :
	O_Message(O_Reply_tag, sizeof(O_Reply_rep)+O_MAC_size)
{
	rep().v = view;
	rep().rid = req;
	rep().cid = cid;
	rep().replica = replica;
	rep().reply_size = -1;
	//rep().rh_digest.zero();
}

O_Reply::O_Reply(O_Reply_rep *r) :
	O_Message(r)
{
}

O_Reply::~O_Reply()
{
}

char* O_Reply::reply(int &len)
{
	len = rep().reply_size;
	return contents()+sizeof(O_Reply_rep);
}

char *O_Reply::store_reply(int &max_len)
{
	max_len = msize()-sizeof(O_Reply_rep)-O_MAC_size;
	return contents()+sizeof(O_Reply_rep);
}

bool O_Reply::verify()
{
	// Replies must be sent by replicas.
	if (!O_node->is_replica(id()))
	{
		fprintf(stderr, "Does not come from a replica\n");
		return false;
	}
	// Check sizes
	int rep_size = (full()) ? rep().reply_size : 0;
	if (size()-(int)sizeof(O_Reply_rep)-rep_size < O_MAC_size)
	{
		fprintf(stderr, "Its bigger than MAC\n");
		return false;
	}
	return true;
}

bool O_Reply::convert(O_Message *m1, O_Reply *&m2)
{
	if (!m1->has_tag(O_Reply_tag, sizeof(O_Reply_rep)))
	{
		return false;
	}
	m2= new O_Reply();
	memcpy(m2->contents(), m1->contents(), m1->size());
	delete m1;
	m2->trim();
	return true;
}
