#include <stdlib.h>
#include <strings.h>
#include "th_assert.h"
#include "M_Message_tags.h"
#include "M_Abandon.h"
#include "M_Node.h"
#include "M_Principal.h"

// extra & 1 = read only

M_Abandon::M_Abandon(M_Abandon_rep *r) :
    M_Message(r) {}

M_Abandon::M_Abandon() :
	M_Message(M_Abandon_tag, sizeof(M_Abandon_rep)) {
	set_size(sizeof(M_Abandon_rep));
}

M_Abandon::M_Abandon(seq_t n) :
	M_Message(M_Abandon_tag, M_Max_message_size) {
	rep().sender_id = M_node->id();
	rep().command_size = 0;
	rep().n = n;
	set_size(sizeof(M_Abandon_rep));
}

M_Abandon::~M_Abandon() {
}

char *M_Abandon::store_command(int &max_len) {
	max_len = msize() - sizeof(M_Abandon_rep) - M_node->auth_size();
	return contents() + sizeof(M_Abandon_rep);
}

bool M_Abandon::convert(M_Message *m1, M_Abandon *&m2) {
	if (!m1->has_tag(M_Abandon_tag, sizeof(M_Abandon_rep))) {
		return false;
	}

	m2 = new M_Abandon(true);
	memcpy(m2->contents(), m1->contents(), m1->size());
	delete m1;
	m2->trim();
	return true;
}

inline M_Abandon_rep& M_Abandon::rep() const {
	th_assert(ALIGNED(msg), "Improperly aligned pointer");
	return *((M_Abandon_rep*) msg);
}

void M_Abandon::display() const {
	fprintf(stderr, "Abandon display: (sender_id = %d)\n", rep().sender_id);
}
