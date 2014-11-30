#include <stdlib.h>
#include <strings.h>
#include "th_assert.h"
#include "M_Message_tags.h"
#include "M_Request.h"
#include "M_Node.h"
#include "M_Principal.h"

// extra & 1 = read only

M_Request::M_Request(M_Request_rep *r) :
    M_Message(r) {}

M_Request::M_Request(int t, unsigned sz) :
    M_Message(t, sz) {}

M_Request::M_Request(Request_id r) :
	M_Message(M_Request_tag, M_Max_message_size) {
	rep().cid = M_node->id();
	rep().sender_id = M_node->id();
	rep().rid = r;
	rep().command_size = 0;
	set_size(sizeof(M_Request_rep));
}

M_Request::~M_Request() {
}

char *M_Request::store_command(int &max_len) {
	max_len = msize() - sizeof(M_Request_rep) - M_node->auth_size();
	return contents() + sizeof(M_Request_rep);
}

bool M_Request::convert(M_Message *m1, M_Request *&m2) {
	if (!m1->has_tag(M_Request_tag, sizeof(M_Request_rep))) {
		return false;
	}

	m2 = new M_Request(true);
	memcpy(m2->contents(), m1->contents(), m1->size());
	delete m1;
	m2->trim();
	return true;
}
/*
bool M_Accept::convert(M_Message *m1, M_Accept *&m2) {
	if (!m1->has_tag(M_Accept_tag, sizeof(M_Request_rep))) {
		return false;
	}
	m2 = new M_Accept();
	memcpy(m2->contents(), m1->contents(), m1->size());
	delete m1;
	m2->trim();
	return true;
}

bool M_Learn::convert(M_Message *m1, M_Learn *&m2) {
	if (!m1->has_tag(M_Learn_tag, sizeof(M_Request_rep))) {
		return false;
	}
	m2 = new M_Learn();
	memcpy(m2->contents(), m1->contents(), m1->size());
	delete m1;
	m2->trim();
	return true;
}

bool M_PrepareResponse::convert(M_Message *m1, M_PrepareResponse *&m2) {
	if (!m1->has_tag(M_PrepareResponse_tag, sizeof(M_Request_rep))) {
		return false;
	}
	m2 = new M_PrepareResponse();
	memcpy(m2->contents(), m1->contents(), m1->size());
	delete m1;
	m2->trim();
	return true;
}
*/

inline M_Request_rep& M_Request::rep() const {
	th_assert(ALIGNED(msg), "Improperly aligned pointer");
	return *((M_Request_rep*) msg);
}

void M_Request::display() const {
	fprintf(stderr, "Request display: (client_id = %d)\n", rep().cid);
}

//void M_Accept::display() const {
	//fprintf(stderr, "Accept display: (client_id = %d)\n", rep().cid);
//}
