#include <strings.h>
#include "th_assert.h"
#include "zl_Message_tags.h"
#include "zl_Reply.h"
#include "zl_Node.h"
#include "zl_Principal.h"

zl_Reply::zl_Reply() :
	zl_Message(zl_Reply_tag, zl_Max_message_size)
{
}

zl_Reply::zl_Reply(View view, Request_id req, int replica, Digest &d,
		zl_Principal *p, int cid) :
	zl_Message(zl_Reply_tag, sizeof(zl_Reply_rep)+zl_MAC_size)
{
	rep().v = view;
	rep().rid = req;
	rep().cid = cid;
	rep().replica = replica;
	rep().reply_size = -1;
	rep().digest = d;
	rep().rh_digest.zero();
	rep().should_switch = false;
	rep().instance_id = zl_node->instance_id();

	zl_my_principal->gen_mac(contents(), sizeof(zl_Reply_rep), contents()
			+sizeof(zl_Reply_rep), p->pid());
}

zl_Reply::zl_Reply(zl_Reply_rep *r) :
	zl_Message(r)
{
}

zl_Reply::~zl_Reply()
{
}

char* zl_Reply::reply(int &len)
{
	len = rep().reply_size;
	return contents()+sizeof(zl_Reply_rep);
}

char *zl_Reply::store_reply(int &max_len)
{
	max_len = msize()-sizeof(zl_Reply_rep)-zl_MAC_size;

	return contents()+sizeof(zl_Reply_rep);
}

void zl_Reply::authenticate(zl_Principal *p, int act_len)
{
	th_assert((unsigned)act_len <= msize()-sizeof(zl_Reply_rep)-zl_MAC_size,
			"Invalid reply size");

	rep().reply_size = act_len;
	rep().digest = Digest(contents()+sizeof(zl_Reply_rep), act_len);
	int old_size = sizeof(zl_Reply_rep)+act_len;
	set_size(old_size+zl_MAC_size);

	zl_my_principal->gen_mac(contents(), sizeof(zl_Reply_rep), contents()
			+old_size, p->pid());

	trim();
}

bool zl_Reply::verify()
{
	// Replies must be sent by replicas.
	if (!zl_node->is_replica(id()))
	{
		fprintf(stderr, "Does not come from a replica\n");
		return false;
	}

	// Check sizes
	int rep_size = (full()) ? rep().reply_size : 0;
	if (size()-(int)sizeof(zl_Reply_rep)-rep_size < zl_MAC_size)
	{
		fprintf(stderr, "Its bigger than MAC\n");
		return false;
	}

	// Check reply
	if (full())
	{
		Digest d(contents()+sizeof(zl_Reply_rep), rep_size);
		if (d != rep().digest)
		{
			fprintf(stderr, "Digest is not equal...\n");
			return false;
		}
	}

	// Check MAC
	zl_Principal *sender = zl_node->i_to_p(id());
	int size_wo_MAC = sizeof(zl_Reply_rep)+rep_size;

	return sender->verify_mac(contents(), sizeof(zl_Reply_rep), contents()
			+size_wo_MAC);
}

bool zl_Reply::convert(zl_Message *m1, zl_Reply *&m2)
{
	if (!m1->has_tag(zl_Reply_tag, sizeof(zl_Reply_rep)))
	{
		return false;
	}
	m2= new zl_Reply();
	memcpy(m2->contents(), m1->contents(), m1->size());
	delete m1;
	m2->trim();
	return true;
}
