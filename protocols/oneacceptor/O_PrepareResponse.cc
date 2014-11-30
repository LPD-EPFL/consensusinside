#include <stdlib.h>
#include <strings.h>
#include "th_assert.h"
#include "O_Message_tags.h"
#include "O_PrepareResponse.h"
#include "O_Node.h"
#include "O_Principal.h"

// extra & 1 = read only

O_PrepareResponse::O_PrepareResponse(O_PrepareResponse_rep *r) :
    O_Message(r) {}

O_PrepareResponse::O_PrepareResponse() :
	O_Message(O_PrepareResponse_tag, sizeof(O_PrepareResponse_rep)) {
	set_size(sizeof(O_PrepareResponse_rep));
}

O_PrepareResponse::O_PrepareResponse(uint64_t index, seq_t n, AcceptedProposals& acc) :
	O_Message(O_PrepareResponse_tag, O_Max_message_size) {
	rep().sender_id = O_node->id();
	rep().command_size = 0;
	rep().index = index;
	rep().n = n;
	rep().acceptedProposals = acc;
	set_size(sizeof(O_PrepareResponse_rep));
}

O_PrepareResponse::~O_PrepareResponse() {
}

char *O_PrepareResponse::store_command(int &max_len) {
	max_len = msize() - sizeof(O_PrepareResponse_rep) - O_node->auth_size();
	return contents() + sizeof(O_PrepareResponse_rep);
}

bool O_PrepareResponse::convert(O_Message *m1, O_PrepareResponse *&m2) {
	if (!m1->has_tag(O_PrepareResponse_tag, sizeof(O_PrepareResponse_rep))) {
		return false;
	}

	m2 = new O_PrepareResponse();
	memcpy(m2->contents(), m1->contents(), m1->size());
	delete m1;
	m2->trim();
	return true;
}

inline O_PrepareResponse_rep& O_PrepareResponse::rep() const {
	th_assert(ALIGNED(msg), "Improperly aligned pointer");
	return *((O_PrepareResponse_rep*) msg);
}

void O_PrepareResponse::display() const {
	fprintf(stderr, "PrepareResponse display: (sender_id = %d)\n", rep().sender_id);
}
