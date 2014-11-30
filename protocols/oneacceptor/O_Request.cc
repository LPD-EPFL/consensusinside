#include <stdlib.h>
#include <strings.h>
#include "th_assert.h"
#include "O_Message_tags.h"
#include "O_Request.h"
#include "O_Node.h"
#include "O_Principal.h"

// extra & 1 = read only

O_Request::O_Request(O_Request_rep *r) :
    O_Message(r) {}

O_Request::O_Request(int t, unsigned sz) :
    O_Message(t, sz) {}

O_Request::O_Request(Request_id r) :
	O_Message(O_Request_tag, O_Max_message_size) {
	rep().cid = O_node->id();
	rep().sender_id = O_node->id();
	rep().rid = r;
	rep().command_size = 0;
	set_size(sizeof(O_Request_rep));
}

O_Request::~O_Request() {
}

char *O_Request::store_command(int &max_len) {
	max_len = msize() - sizeof(O_Request_rep) - O_node->auth_size();
	return contents() + sizeof(O_Request_rep);
}

bool O_Request::convert(O_Message *m1, O_Request *&m2) {
	if (!m1->has_tag(O_Request_tag, sizeof(O_Request_rep))) {
		return false;
	}

	m2 = new O_Request(true);
	memcpy(m2->contents(), m1->contents(), m1->size());
	delete m1;
	m2->trim();
	return true;
}

void O_Accept::convert() {
	msg->tag = O_Accept_tag; 
	sender_id() = O_node->id();
}

void O_Learn::convert() {
	msg->tag = O_Learn_tag; 
	sender_id() = O_node->id();
}

bool O_Accept::convert(O_Message *m1, O_Accept *&m2) {
	if (!m1->has_tag(O_Accept_tag, sizeof(O_Request_rep))) {
		return false;
	}
	m2 = new O_Accept();
	memcpy(m2->contents(), m1->contents(), m1->size());
	delete m1;
	m2->trim();
	return true;
}

bool O_Learn::convert(O_Message *m1, O_Learn *&m2) {
	if (!m1->has_tag(O_Learn_tag, sizeof(O_Request_rep))) {
		return false;
	}
	m2 = new O_Learn();
	memcpy(m2->contents(), m1->contents(), m1->size());
	delete m1;
	m2->trim();
	return true;
}

inline O_Request_rep& O_Request::rep() const {
	th_assert(ALIGNED(msg), "Improperly aligned pointer");
	return *((O_Request_rep*) msg);
}

void O_Request::display() const {
	fprintf(stderr, "Request display: (client_id = %d)\n", rep().cid);
}

//void O_Accept::display() const {
	//fprintf(stderr, "Accept display: (client_id = %d)\n", rep().cid);
//}
