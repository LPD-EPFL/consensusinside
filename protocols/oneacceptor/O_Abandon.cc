#include <stdlib.h>
#include <strings.h>
#include "th_assert.h"
#include "O_Message_tags.h"
#include "O_Abandon.h"
#include "O_Node.h"
#include "O_Principal.h"

// extra & 1 = read only

O_Abandon::O_Abandon(O_Abandon_rep *r) :
    O_Message(r) {}

O_Abandon::O_Abandon() :
	O_Message(O_Abandon_tag, sizeof(O_Abandon_rep)) {
	set_size(sizeof(O_Abandon_rep));
}

O_Abandon::O_Abandon(seq_t n, int leader) :
	O_Message(O_Abandon_tag, O_Max_message_size) {
	rep().sender_id = O_node->id();
	rep().command_size = 0;
	rep().n = n;
	rep().leader_id = leader;
	set_size(sizeof(O_Abandon_rep));
}

O_Abandon::~O_Abandon() {
}

char *O_Abandon::store_command(int &max_len) {
	max_len = msize() - sizeof(O_Abandon_rep) - O_node->auth_size();
	return contents() + sizeof(O_Abandon_rep);
}

bool O_Abandon::convert(O_Message *m1, O_Abandon *&m2) {
	if (!m1->has_tag(O_Abandon_tag, sizeof(O_Abandon_rep))) {
		return false;
	}

	m2 = new O_Abandon();
	memcpy(m2->contents(), m1->contents(), m1->size());
	delete m1;
	m2->trim();
	return true;
}

inline O_Abandon_rep& O_Abandon::rep() const {
	th_assert(ALIGNED(msg), "Improperly aligned pointer");
	return *((O_Abandon_rep*) msg);
}

void O_Abandon::display() const {
	fprintf(stderr, "Abandon display: (sender_id = %d)\n", rep().sender_id);
}
