#include <strings.h>
#include "th_assert.h"
#include "Q_Message_tags.h"
#include "Q_Reply.h"
#include "Q_Node.h"
#include "Q_Principal.h"

#define Q_MAC_size 0

Q_Reply::Q_Reply() :
	Q_Message(Q_Reply_tag, Q_Max_message_size)
{
}

Q_Reply::Q_Reply(View view, Request_id req, int replica, Digest &d,
		Q_Principal *p, int cid) :
	Q_Message(Q_Reply_tag, sizeof(Q_Reply_rep)+Q_MAC_size)
{
	rep().v = view;
	rep().rid = req;
	rep().cid = cid;
	rep().replica = replica;
	rep().reply_size = -1;
	rep().digest = d;
	rep().rh_digest.zero();
	rep().should_switch = false;
	rep().instance_id = Q_node->instance_id();

	//Q_my_principal->gen_mac(contents(), sizeof(Q_Reply_rep), contents()
			//+sizeof(Q_Reply_rep), p->pid());
}

Q_Reply::Q_Reply(Q_Reply_rep *r) :
	Q_Message(r)
{
}

Q_Reply::~Q_Reply()
{
}

char* Q_Reply::reply(int &len)
{
	len = rep().reply_size;
	return contents()+sizeof(Q_Reply_rep);
}

char *Q_Reply::store_reply(int &max_len)
{
	max_len = msize()-sizeof(Q_Reply_rep)-Q_MAC_size;

	return contents()+sizeof(Q_Reply_rep);
}

void Q_Reply::authenticate(Q_Principal *p, int act_len)
{
	//th_assert((unsigned)act_len <= msize()-sizeof(Q_Reply_rep)-Q_MAC_size,
			//"Invalid reply size");
//
	//rep().reply_size = act_len;
	//rep().digest = Digest(contents()+sizeof(Q_Reply_rep), act_len);
	//int old_size = sizeof(Q_Reply_rep)+act_len;
	//set_size(old_size+Q_MAC_size);
//
	//Q_my_principal->gen_mac(contents(), sizeof(Q_Reply_rep), contents()
			//+old_size, p->pid());
//
	//trim();
}

bool Q_Reply::verify()
{
	// Replies must be sent by replicas.
	if (!Q_node->is_replica(id()))
	{
		fprintf(stderr, "Does not come from a replica\n");
		return false;
	}


	// Check sizes
	int rep_size = (full()) ? rep().reply_size : 0;
	if (size()-(int)sizeof(Q_Reply_rep)-rep_size < Q_MAC_size)
	{
		fprintf(stderr, "Its bigger than MAC\n");
		return false;
	}

	return true;

	// Check reply
	//if (full())
	//{
		//Digest d(contents()+sizeof(Q_Reply_rep), rep_size);
		//if (d != rep().digest)
		//{
			//fprintf(stderr, "Digest is not equal...\n");
			//return false;
		//}
	//}

	// Check MAC
	//Q_Principal *sender = Q_node->i_to_p(id());
	//int size_wo_MAC = sizeof(Q_Reply_rep)+rep_size;
//
	//return sender->verify_mac(contents(), sizeof(Q_Reply_rep), contents()
			//+size_wo_MAC);
}

bool Q_Reply::convert(Q_Message *m1, Q_Reply *&m2)
{
	if (!m1->has_tag(Q_Reply_tag, sizeof(Q_Reply_rep)))
	{
		return false;
	}
	m2= new Q_Reply();
	memcpy(m2->contents(), m1->contents(), m1->size());
	delete m1;
	m2->trim();
	return true;
}
