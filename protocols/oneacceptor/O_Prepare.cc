#include <stdlib.h>
#include <strings.h>
#include "th_assert.h"
#include "O_Message_tags.h"
#include "O_Prepare.h"
#include "O_Node.h"
#include "O_Principal.h"

// extra & 1 = read only

O_Prepare::O_Prepare(O_Prepare_rep *r) :
    O_Message(r) {}

O_Prepare::O_Prepare() :
	O_Message(O_Prepare_tag, sizeof(O_Prepare_rep)) {
	set_size(sizeof(O_Prepare_rep));
}

O_Prepare::O_Prepare(uint64_t index, seq_t n) :
	O_Message(O_Prepare_tag, O_Max_message_size) {
	rep().sender_id = O_node->id();
	rep().command_size = 0;
	rep().index = index;
	rep().n = n;
	set_size(sizeof(O_Prepare_rep));
}

O_Prepare::~O_Prepare() {
}

char *O_Prepare::store_command(int &max_len) {
	max_len = msize() - sizeof(O_Prepare_rep) - O_node->auth_size();
	return contents() + sizeof(O_Prepare_rep);
}

bool O_Prepare::convert(O_Message *m1, O_Prepare *&m2) {
	if (!m1->has_tag(O_Prepare_tag, sizeof(O_Prepare_rep))) {
		return false;
	}

	m2 = new O_Prepare();
	memcpy(m2->contents(), m1->contents(), m1->size());
	delete m1;
	m2->trim();
	return true;
}

inline O_Prepare_rep& O_Prepare::rep() const {
	th_assert(ALIGNED(msg), "Improperly aligned pointer");
	return *((O_Prepare_rep*) msg);
}

void O_Prepare::display() const {
	fprintf(stderr, "Prepare display: (sender_id = %d)\n", rep().sender_id);
}
