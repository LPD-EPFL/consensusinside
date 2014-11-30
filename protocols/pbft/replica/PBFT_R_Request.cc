#include <stdlib.h>
#include <strings.h>
#include "th_assert.h"
#include "PBFT_R_Message_tags.h"
#include "PBFT_R_Request.h"
#include "PBFT_R_Node.h"
#include "PBFT_R_Principal.h"
#include "MD5.h"

#include "PBFT_R_Statistics.h"

// extra & 1 = read only
// extra & 2 = signed

PBFT_R_Request::PBFT_R_Request(Request_id r, short rr) :
	PBFT_R_Message(PBFT_R_Request_tag, Max_message_size)
{
	rep().cid = PBFT_R_node->id();
	rep().rid = r;
	rep().replier = rr;
	rep().command_size = 0;
	set_size(sizeof(PBFT_R_Request_rep));
}

PBFT_R_Request* PBFT_R_Request::clone() const
{
	PBFT_R_Request* ret = (PBFT_R_Request*)new PBFT_R_Message(max_size);
	memcpy(ret->msg, msg, msg->size);
	return ret;
}

char *PBFT_R_Request::store_command(int &max_len)
{
	int max_auth_size= MAX(PBFT_R_node->principal()->sig_size(), PBFT_R_node->auth_size());
	max_len = msize()-sizeof(PBFT_R_Request_rep)-max_auth_size;
	return contents()+sizeof(PBFT_R_Request_rep);
}

inline void PBFT_R_Request::comp_digest(Digest& d)
{
	INCPBFT_R_OP(num_digests);START_CC(digest_cycles);

	MD5_CTX context;
	MD5Init(&context);
	MD5Update(&context, (char*)&(rep().cid), sizeof(int)+sizeof(Request_id)+rep().command_size);
	MD5Final(d.udigest(), &context);

	STOP_CC(digest_cycles);
}

void PBFT_R_Request::authenticate(int act_len, bool read_only)
{
	th_assert((unsigned)act_len <= msize()-sizeof(PBFT_R_Request_rep)
			-PBFT_R_node->auth_size(), "Invalid request size");

	rep().extra = ((read_only) ? 1 : 0);
	rep().command_size = act_len;
	if (rep().replier == -1)
		rep().replier = lrand48()%PBFT_R_node->n();
	comp_digest(rep().od);

	int old_size = sizeof(PBFT_R_Request_rep)+act_len;
	set_size(old_size+PBFT_R_node->auth_size());
	PBFT_R_node->gen_auth_in(contents(), sizeof(PBFT_R_Request_rep), contents()
			+old_size);
}

void PBFT_R_Request::re_authenticate(bool change, PBFT_R_Principal *p)
{
	if (change)
	{
		rep().extra &= ~1;
	}
	int new_rep = lrand48()%PBFT_R_node->n();
	rep().replier = (new_rep != rep().replier) ? new_rep : (new_rep+1)%PBFT_R_node->n();

	int old_size = sizeof(PBFT_R_Request_rep)+rep().command_size;
	if ((rep().extra & 2) == 0)
	{
		PBFT_R_node->gen_auth_in(contents(), sizeof(PBFT_R_Request_rep),
				contents()+old_size);
	} else
	{
		PBFT_R_node->gen_signature(contents(), sizeof(PBFT_R_Request_rep),
				contents()+old_size);
	}
}

void PBFT_R_Request::sign(int act_len)
{
	th_assert((unsigned)act_len <= msize()-sizeof(PBFT_R_Request_rep)
			-PBFT_R_node->principal()->sig_size(), "Invalid request size");

	rep().extra |= 2;
	rep().command_size = act_len;
	comp_digest(rep().od);

	int old_size = sizeof(PBFT_R_Request_rep)+act_len;
	set_size(old_size+PBFT_R_node->principal()->sig_size());
	PBFT_R_node->gen_signature(contents(), sizeof(PBFT_R_Request_rep),
			contents()+old_size);
}

PBFT_R_Request::PBFT_R_Request(PBFT_R_Request_rep *contents) :
	PBFT_R_Message(contents)
{
}

bool PBFT_R_Request::verify()
{
	const int nid = PBFT_R_node->id();
	const int cid = client_id();
	const int old_size = sizeof(PBFT_R_Request_rep)+rep().command_size;
	PBFT_R_Principal* p = PBFT_R_node->i_to_p(cid);
	Digest d;

//	fprintf(stderr, "PBFT_R_Request: Verifying the request...\n");

	comp_digest(d);
	Digest zeroD = Digest();
	zeroD.zero();

	if ((getenv("PBFT_J2EE") != NULL) && rep().od == zeroD)
	{
		// PBFT_R_Message has an authenticator.
		bool ret = PBFT_R_node->verify_auth_out(cid, contents(),
				sizeof(PBFT_R_Request_rep), contents()+old_size);
/*		if (ret)
			fprintf(stderr, "PBFT_R_Request: Verifying the request returns true\n");
		else
			fprintf(stderr, "PBFT_R_Request: Verifying the request returns false\n");
*/		return true;

	}
	if (p != 0 && d == rep().od)
	{
		if ((rep().extra & 2) == 0)
		{
			// PBFT_R_Message has an authenticator.
			if (cid != nid && cid >= PBFT_R_node->n() && size()-old_size
					>= PBFT_R_node->auth_size(cid))
			{
				bool ret = PBFT_R_node->verify_auth_out(cid, contents(),
						sizeof(PBFT_R_Request_rep), contents()+old_size);
				/*				if (ret)
				 fprintf(stderr, "PBFT_R_Request: Verifying the request returns true\n");
				 else
				 fprintf(stderr, "PBFT_R_Request: Verifying the request returns false\n");
				 */
				return ret;
			}
		} else
		{
			// PBFT_R_Message is signed.
			if (size() - old_size >= p->sig_size())
				return p->verify_signature(contents(),
						sizeof(PBFT_R_Request_rep), contents()+old_size, true);
		}
	}
	return false;
}

bool PBFT_R_Request::convert(PBFT_R_Message *m1, PBFT_R_Request *&m2)
{
	if (!m1->has_tag(PBFT_R_Request_tag, sizeof(PBFT_R_Request_rep)))
		return false;

	m2 = (PBFT_R_Request*)m1;
	m2->trim();
	return true;
}

bool PBFT_R_Request::convert(char *m1, unsigned max_len, PBFT_R_Request &m2)
{
	if (!PBFT_R_Message::convert(m1, max_len, PBFT_R_Request_tag,
			sizeof(PBFT_R_Request_rep), m2))
		return false;
	return true;
}

