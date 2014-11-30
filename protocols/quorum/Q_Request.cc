#include <stdlib.h>
#include <strings.h>
#include "th_assert.h"
#include "Q_Message_tags.h"
#include "Q_Request.h"
#include "Q_Node.h"
#include "Q_Principal.h"
#include "MD5.h"

// extra & 1 = read only

Q_Request::Q_Request(Q_Request_rep *r) :
    Q_Message(r) {}

Q_Request::Q_Request(Request_id r, short rr) :
	Q_Message(Q_Request_tag, Q_Max_message_size) {
	rep().cid = Q_node->id();
	rep().rid = r;
	rep().replier = rr;
	rep().command_size = 0;
	rep().unused = 0;
	set_size(sizeof(Q_Request_rep));
}

Q_Request::~Q_Request() {
}

char *Q_Request::store_command(int &max_len) {
	max_len = msize() - sizeof(Q_Request_rep) - Q_node->auth_size();
	return contents() + sizeof(Q_Request_rep);
}

inline void Q_Request::comp_digest(Digest& d) {
	MD5_CTX context;
	MD5Init(&context);
	MD5Update(&context, (char*) &(rep().cid), sizeof(int) + sizeof(Request_id)
			+ rep().command_size);
	MD5Final(d.udigest(), &context);
}

void Q_Request::authenticate(int act_len, bool read_only) {
	//th_assert((unsigned)act_len <= msize() - sizeof(Q_Request_rep)
			//- Q_node->auth_size(), "Invalid request size");
//
	//// rep().extra = ((read_only) ? 1 : 0);
	//rep().extra &= ~1;
	//if (read_only) {
		//rep().extra = rep().extra | 1;
	//}
//
	//rep().command_size = act_len;
	//if (rep().replier == -1) {
		//rep().replier = lrand48() % Q_node->n();
	//}
	//comp_digest(rep().od);
//
	//int old_size = sizeof(Q_Request_rep) + act_len;
	//set_size(old_size + Q_node->auth_size());
	//Q_node->gen_auth(contents(), sizeof(Q_Request_rep), contents() + old_size);
}

bool Q_Request::verify() {
return true;
	//const int nid = Q_node->id();
	//const int cid = client_id();
	//const int old_size = sizeof(Q_Request_rep) + rep().command_size;
	//Q_Principal* p = Q_node->i_to_p(cid);
	//Digest d;
//
	//comp_digest(d);
	//if (p != 0 && d == rep().od) {
		//// Verifying the authenticator.
		//if (cid != nid && cid >= Q_node->n() && size() - old_size
				//>= Q_node->auth_size(cid)) {
			////return Q_node->verify_auth(cid, contents(), sizeof(Q_Request_rep),
					////contents() + old_size);
		   //return true;
		//}
	//}
	//return false;
}

bool Q_Request::convert(Q_Message *m1, Q_Request *&m2) {
	if (!m1->has_tag(Q_Request_tag, sizeof(Q_Request_rep))) {
		return false;
	}

	m2 = new Q_Request(true);
	memcpy(m2->contents(), m1->contents(), m1->size());
	delete m1;
	m2->trim();
	return true;
}

inline Q_Request_rep& Q_Request::rep() const {
	th_assert(ALIGNED(msg), "Improperly aligned pointer");
	return *((Q_Request_rep*) msg);
}

void Q_Request::display() const {
	fprintf(stderr, "Request display: (client_id = %d)\n", rep().cid);
}
