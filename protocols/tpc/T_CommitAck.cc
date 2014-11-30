#include <stdlib.h>
#include <strings.h>
#include "th_assert.h"
#include "T_Message_tags.h"
#include "T_CommitAck.h"
#include "T_Node.h"
#include "T_Principal.h"

T_CommitAck::T_CommitAck(T_CommitAck_rep *r) :
    T_Message(r) {}

T_CommitAck::T_CommitAck(Request_id r) :
	T_Message(T_CommitAck_tag, sizeof(T_CommitAck_rep)) {
	rep().sender = T_node->id();
	rep().rid = r;
	set_size(sizeof(T_CommitAck_rep));
}

T_CommitAck::T_CommitAck() :
	T_Message(T_CommitAck_tag, sizeof(T_CommitAck_rep)) {
	set_size(sizeof(T_CommitAck_rep));
}

T_CommitAck::~T_CommitAck() {
}

bool T_CommitAck::convert(T_Message *m1, T_CommitAck *&m2) {
	if (!m1->has_tag(T_CommitAck_tag, sizeof(T_CommitAck_rep))) {
		return false;
	}

	m2 = new T_CommitAck();
	memcpy(m2->contents(), m1->contents(), m1->size());
	delete m1;
	m2->trim();
	return true;
}

inline T_CommitAck_rep& T_CommitAck::rep() const {
	th_assert(ALIGNED(msg), "Improperly aligned pointer");
	return *((T_CommitAck_rep*) msg);
}

void T_CommitAck::display() const {
	fprintf(stderr, "CommitAck display: (sender_id = %d)\n", rep().sender);
}
