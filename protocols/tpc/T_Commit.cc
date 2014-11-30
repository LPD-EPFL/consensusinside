#include <stdlib.h>
#include <strings.h>
#include "th_assert.h"
#include "T_Message_tags.h"
#include "T_Commit.h"
#include "T_Node.h"
#include "T_Principal.h"

T_Commit::T_Commit(T_Commit_rep *r) :
    T_Message(r) {}

T_Commit::T_Commit(Request_id r) :
	T_Message(T_Commit_tag, sizeof(T_Commit_rep)) {
	rep().sender = T_node->id();
	rep().rid = r;
	set_size(sizeof(T_Commit_rep));
}

T_Commit::T_Commit() :
	T_Message(T_Commit_tag, sizeof(T_Commit_rep)) {
	set_size(sizeof(T_Commit_rep));
}

T_Commit::~T_Commit() {
}

bool T_Commit::convert(T_Message *m1, T_Commit *&m2) {
	if (!m1->has_tag(T_Commit_tag, sizeof(T_Commit_rep))) {
		return false;
	}

	m2 = new T_Commit();
	memcpy(m2->contents(), m1->contents(), m1->size());
	delete m1;
	m2->trim();
	return true;
}

inline T_Commit_rep& T_Commit::rep() const {
	th_assert(ALIGNED(msg), "Improperly aligned pointer");
	return *((T_Commit_rep*) msg);
}

void T_Commit::display() const {
	fprintf(stderr, "Commit display: (sender_id = %d)\n", rep().sender);
}
