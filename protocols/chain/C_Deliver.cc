#include <stdlib.h>
#include <strings.h>
#include "th_assert.h"
#include "C_Message_tags.h"
#include "C_Deliver.h"
#include "C_Node.h"
#include "C_Principal.h"
#include "MD5.h"

C_Deliver::C_Deliver() :
	C_Message(C_Deliver_tag, C_Max_message_size)
{
}

C_Deliver::C_Deliver(int cid, Request_id& r, Digest& d) :
	C_Message(C_Deliver_tag, C_Max_message_size)
{
	rep().cid = cid;
	rep().rid = r;
	rep().od = d;
	set_size(sizeof(C_Deliver_rep)+C_node->auth_size());
}

C_Deliver::~C_Deliver()
{
}

inline void C_Deliver::set_digest(Digest& d)
{
	//  fprintf(stderr, "Computing MD5 digest (inputLen = %d)\n", sizeof(int)
	//        +sizeof(Request_id)+ rep().command_size);INCR_OP(num_digests);START_CC(digest_cycles);
	rep().od = d;
}

bool C_Deliver::convert(C_Message *m1, C_Deliver *&m2)
{

	if (!m1->has_tag(C_Deliver_tag, sizeof(C_Deliver_rep)))
	{
		return false;
	}

	m2 = new C_Deliver();
	memcpy(m2->contents(), m1->contents(), m1->size());
	delete m1;
	m2->trim();
	return true;
}

