#include <stdlib.h>
#include <strings.h>
#include "th_assert.h"
#include "M_Message_tags.h"
#include "M_Accept.h"
#include "M_Node.h"
#include "M_Principal.h"

// extra & 1 = read only

char *M_Accept::store_command(int &max_len) {
	max_len = msize() - sizeof(M_Accept_rep) - M_node->auth_size();
	return contents() + sizeof(M_Accept_rep);
}

bool M_Accept::convert(M_Message *m1, M_Accept *&m2) {
	if (!m1->has_tag(M_Accept_tag, sizeof(M_Accept_rep))) {
		return false;
	}

	m2 = new M_Accept();
	memcpy(m2->contents(), m1->contents(), m1->size());
	delete m1;
	m2->trim();
	return true;
}

inline M_Accept_rep& M_Accept::rep() const {
	th_assert(ALIGNED(msg), "Improperly aligned pointer");
	return *((M_Accept_rep*) msg);
}

void M_Accept::display() const {
	fprintf(stderr, "Request display: (client_id = %d)\n", rep().cid);
}

