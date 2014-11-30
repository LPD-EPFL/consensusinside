#include <stdlib.h>
#include <strings.h>
#include "th_assert.h"
#include "T_Message_tags.h"
#include "T_Ack.h"
#include "T_Node.h"
#include "T_Principal.h"

T_Ack::T_Ack(T_Ack_rep *r) :
    T_Message(r) {}

T_Ack::T_Ack() :
	T_Message(T_Ack_tag, sizeof(T_Ack_rep)) {
	set_size(sizeof(T_Ack_rep));
}

T_Ack::T_Ack(Request_id r) :
	T_Message(T_Ack_tag, sizeof(T_Ack_rep)) {
	rep().sender = T_node->id();
	rep().rid = r;
	set_size(sizeof(T_Ack_rep));
}

T_Ack::~T_Ack() {
}

bool T_Ack::convert(T_Message *m1, T_Ack *&m2) {
	if (!m1->has_tag(T_Ack_tag, sizeof(T_Ack_rep))) {
		return false;
	}

	m2 = new T_Ack();
	memcpy(m2->contents(), m1->contents(), m1->size());
	delete m1;
	m2->trim();
	return true;
}

inline T_Ack_rep& T_Ack::rep() const {
	th_assert(ALIGNED(msg), "Improperly aligned pointer");
	return *((T_Ack_rep*) msg);
}

void T_Ack::display() const {
	fprintf(stderr, "Ack display: (sender_id = %d)\n", rep().sender);
}
