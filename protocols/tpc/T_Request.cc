#include <stdlib.h>
#include <strings.h>
#include "th_assert.h"
#include "T_Message_tags.h"
#include "T_Request.h"
#include "T_Node.h"
#include "T_Principal.h"

// extra & 1 = read only

T_Request::T_Request(T_Request_rep *r) :
    T_Message(r) {}

T_Request::T_Request(Request_id r) :
	T_Message(T_Request_tag, T_Max_message_size) {
	rep().cid = T_node->id();
	rep().sender_id = T_node->id();
	rep().rid = r;
	rep().command_size = 0;
	set_size(sizeof(T_Request_rep));
}

T_Request::~T_Request() {
}

char *T_Request::store_command(int &max_len) {
	max_len = msize() - sizeof(T_Request_rep) - T_node->auth_size();
	return contents() + sizeof(T_Request_rep);
}

bool T_Request::convert(T_Message *m1, T_Request *&m2) {
	if (!m1->has_tag(T_Request_tag, sizeof(T_Request_rep))) {
		return false;
	}

	m2 = new T_Request(true);
	memcpy(m2->contents(), m1->contents(), m1->size());
	delete m1;
	m2->trim();
	return true;
}

inline T_Request_rep& T_Request::rep() const {
	th_assert(ALIGNED(msg), "Improperly aligned pointer");
	return *((T_Request_rep*) msg);
}

void T_Request::display() const {
	fprintf(stderr, "Request display: (client_id = %d)\n", rep().cid);
}
