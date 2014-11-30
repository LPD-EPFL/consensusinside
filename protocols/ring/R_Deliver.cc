#include <stdlib.h>
#include <strings.h>
#include "th_assert.h"
#include "R_Message_tags.h"
#include "R_Deliver.h"
#include "R_Node.h"
#include "R_Principal.h"
#include "MD5.h"

R_Deliver::R_Deliver() :
	R_Message(R_Deliver_tag, R_Max_message_size)
{
}

R_Deliver::R_Deliver(int cid, Seqno& seqno, Request_id& r, Digest& d) :
	R_Message(R_Deliver_tag, R_Max_message_size)
{
	rep().cid = cid;
	rep().rid = r;
	rep().seqno = seqno;
	rep().od = d;
	set_size(sizeof(R_Deliver_rep)+R_node->authenticators_size());
}

R_Deliver::~R_Deliver()
{
}

inline void R_Deliver::set_digest(Digest& d)
{
	//  fprintf(stderr, "Computing MD5 digest (inputLen = %d)\n", sizeof(int)
	//        +sizeof(Request_id)+ rep().command_size);INCR_OP(num_digests);START_CC(digest_cycles);
	rep().od = d;
}

bool R_Deliver::convert(R_Message *m1, R_Deliver *&m2)
{

	if (!m1->has_tag(R_Deliver_tag, sizeof(R_Deliver_rep)))
	{
		return false;
	}

	m2 = new R_Deliver();
	memcpy(m2->contents(), m1->contents(), m1->size());
	delete m1;
	m2->trim();
	return true;
}

