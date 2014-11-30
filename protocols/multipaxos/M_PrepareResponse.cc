#include <stdlib.h>
#include <strings.h>
#include "th_assert.h"
#include "M_Message_tags.h"
#include "M_PrepareResponse.h"
#include "M_Node.h"
#include "M_Principal.h"

// extra & 1 = read only

char *M_PrepareResponse::store_command(int &max_len) {
	max_len = msize() - sizeof(M_PrepareResponse_rep) - M_node->auth_size();
	return contents() + sizeof(M_PrepareResponse_rep);
}

bool M_PrepareResponse::convert(M_Message *m1, M_PrepareResponse *&m2) {
	if (!m1->has_tag(M_PrepareResponse_tag, sizeof(M_PrepareResponse_rep))) {
		return false;
	}

	m2 = new M_PrepareResponse();
	memcpy(m2->contents(), m1->contents(), m1->size());
	delete m1;
	m2->trim();
	return true;
}

inline M_PrepareResponse_rep& M_PrepareResponse::rep() const {
	th_assert(ALIGNED(msg), "Improperly aligned pointer");
	return *((M_PrepareResponse_rep*) msg);
}

void M_PrepareResponse::display() const {
	fprintf(stderr, "Request display: (client_id = %d)\n", rep().cid);
}

