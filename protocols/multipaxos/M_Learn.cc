#include <stdlib.h>
#include <strings.h>
#include "th_assert.h"
#include "M_Message_tags.h"
#include "M_Learn.h"
#include "M_Node.h"
#include "M_Principal.h"

// extra & 1 = read only

char *M_Learn::store_command(int &max_len) {
	max_len = msize() - sizeof(M_Learn_rep) - M_node->auth_size();
	return contents() + sizeof(M_Learn_rep);
}

bool M_Learn::convert(M_Message *m1, M_Learn *&m2) {
	if (!m1->has_tag(M_Learn_tag, sizeof(M_Learn_rep))) {
		return false;
	}

	m2 = new M_Learn();
	memcpy(m2->contents(), m1->contents(), m1->size());
	delete m1;
	m2->trim();
	return true;
}

inline M_Learn_rep& M_Learn::rep() const {
	th_assert(ALIGNED(msg), "Improperly aligned pointer");
	return *((M_Learn_rep*) msg);
}

void M_Learn::display() const {
	fprintf(stderr, "Request display: (client_id = %d)\n", rep().cid);
}

