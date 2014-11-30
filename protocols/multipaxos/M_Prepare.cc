#include <stdlib.h>
#include <strings.h>
#include "th_assert.h"
#include "M_Message_tags.h"
#include "M_Prepare.h"
#include "M_Node.h"
#include "M_Principal.h"

// extra & 1 = read only

M_Prepare::M_Prepare(M_Prepare_rep *r) :
    M_Message(r) {}

M_Prepare::M_Prepare() :
	M_Message(M_Prepare_tag, sizeof(M_Prepare_rep)) {
	set_size(sizeof(M_Prepare_rep));
}

M_Prepare::M_Prepare(uint64_t index, seq_t n) :
	M_Message(M_Prepare_tag, M_Max_message_size) {
	rep().sender_id = M_node->id();
	rep().command_size = 0;
	rep().index = index;
	rep().n = n;
	set_size(sizeof(M_Prepare_rep));
}

M_Prepare::~M_Prepare() {
}

char *M_Prepare::store_command(int &max_len) {
	max_len = msize() - sizeof(M_Prepare_rep) - M_node->auth_size();
	return contents() + sizeof(M_Prepare_rep);
}

bool M_Prepare::convert(M_Message *m1, M_Prepare *&m2) {
	if (!m1->has_tag(M_Prepare_tag, sizeof(M_Prepare_rep))) {
		return false;
	}

	m2 = new M_Prepare();
	memcpy(m2->contents(), m1->contents(), m1->size());
	delete m1;
	m2->trim();
	return true;
}

inline M_Prepare_rep& M_Prepare::rep() const {
	th_assert(ALIGNED(msg), "Improperly aligned pointer");
	return *((M_Prepare_rep*) msg);
}

void M_Prepare::display() const {
	fprintf(stderr, "Prepare display: (sender_id = %d)\n", rep().sender_id);
}
