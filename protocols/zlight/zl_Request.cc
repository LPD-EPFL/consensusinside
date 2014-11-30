#include <stdlib.h>
#include <strings.h>
#include "th_assert.h"
#include "zl_Message_tags.h"
#include "zl_Request.h"
#include "zl_Node.h"
#include "zl_Principal.h"
#include "MD5.h"

// extra & 1 = read only

zl_Request::zl_Request(zl_Request_rep *r) :
    zl_Message(r) {}

zl_Request::zl_Request(Request_id r, short rr) :
	zl_Message(zl_Request_tag, zl_Max_message_size) {
	rep().cid = zl_node->id();
	rep().rid = r;
	rep().replier = rr;
	rep().command_size = 0;
	set_size(sizeof(zl_Request_rep));
}

zl_Request::~zl_Request() {
}

char *zl_Request::store_command(int &max_len) {
	max_len = msize() - sizeof(zl_Request_rep) - zl_node->auth_size();
	return contents() + sizeof(zl_Request_rep);
}

inline void zl_Request::comp_digest(Digest& d) {
	MD5_CTX context;
	MD5Init(&context);
	MD5Update(&context, (char*) &(rep().cid), sizeof(int) + sizeof(Request_id)
			+ rep().command_size);
	MD5Final(d.udigest(), &context);
}

void zl_Request::authenticate(int act_len, bool read_only) {
	th_assert((unsigned)act_len <= msize() - sizeof(zl_Request_rep)
			- zl_node->auth_size(), "Invalid request size");

	// rep().extra = ((read_only) ? 1 : 0);
	rep().extra &= ~1;
	if (read_only) {
		rep().extra = rep().extra | 1;
	}

	rep().command_size = act_len;
	if (rep().replier == -1) {
		rep().replier = lrand48() % zl_node->n();
	}
	comp_digest(rep().od);

	int old_size = sizeof(zl_Request_rep) + act_len;
	set_size(old_size + zl_node->auth_size());
	zl_node->gen_auth(contents(), sizeof(zl_Request_rep), contents() + old_size);
}

bool zl_Request::verify() {
	const int nid = zl_node->id();
	const int cid = client_id();
	const int old_size = sizeof(zl_Request_rep) + rep().command_size;
	zl_Principal* p = zl_node->i_to_p(cid);
	Digest d;

	comp_digest(d);
	if (p != 0 && d == rep().od) {
		// Verifying the authenticator.
		if (cid != nid && cid >= zl_node->n() && size() - old_size
				>= zl_node->auth_size(cid)) {
			return zl_node->verify_auth(cid, contents(), sizeof(zl_Request_rep),
					contents() + old_size);
		}
	}
	return false;
}

bool zl_Request::convert(zl_Message *m1, zl_Request *&m2) {
	if (!m1->has_tag(zl_Request_tag, sizeof(zl_Request_rep))) {
		return false;
	}

	m2 = new zl_Request(true);
	memcpy(m2->contents(), m1->contents(), m1->size());
	delete m1;
	m2->trim();
	return true;
}

inline zl_Request_rep& zl_Request::rep() const {
	th_assert(ALIGNED(msg), "Improperly aligned pointer");
	return *((zl_Request_rep*) msg);
}

void zl_Request::display() const {
	fprintf(stderr, "Request display: (client_id = %d)\n", rep().cid);
}
