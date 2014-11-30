#include <stdlib.h>
#include "th_assert.h"
#include "PBFT_R_Message.h"
#include "PBFT_R_Node.h"

PBFT_R_Message::PBFT_R_Message(unsigned sz) :
	msg(0), max_size(ALIGNED_SIZE(sz))
{
	if (sz != 0)
	{
		msg = (PBFT_R_Message_rep*) malloc(max_size);
		th_assert(ALIGNED(msg), "Improperly aligned pointer");
		msg->tag = -1;
		msg->size = 0;
		msg->extra = 0;
	}
}

PBFT_R_Message::PBFT_R_Message(int t, unsigned sz)
{
	max_size = ALIGNED_SIZE(sz);
	msg = (PBFT_R_Message_rep*) malloc(max_size);
	th_assert(ALIGNED(msg), "Improperly aligned pointer");
	msg->tag = t;
	msg->size = max_size;
	msg->extra = 0;
}

PBFT_R_Message::PBFT_R_Message(PBFT_R_Message_rep *cont)
{
	th_assert(ALIGNED(cont), "Improperly aligned pointer");
	msg = cont;
	max_size = -1; // To prevent contents from being deallocated or trimmed
}

PBFT_R_Message::~PBFT_R_Message()
{
	if (max_size > 0)
		free(msg);
}

void PBFT_R_Message::trim()
{
	if (max_size > 0 && realloc((char*)msg, msg->size))
	{
		max_size = msg->size;
	}
}

void PBFT_R_Message::set_size(int size)
{
	th_assert(msg && ALIGNED(msg), "Invalid state");
	th_assert(max_size < 0 || ALIGNED_SIZE(size) <= max_size, "Invalid state");
	int aligned= ALIGNED_SIZE(size);
	for (int i=size; i < aligned; i++)
		((char*)msg)[i] = 0;
	msg->size = aligned;
}

bool PBFT_R_Message::convert(char *src, unsigned len, int t, int sz,
		PBFT_R_Message &m)
{
	// First check if src is large enough to hold a PBFT_R_Message_rep
	if (len < sizeof(PBFT_R_Message_rep))
		return false;

	// Check alignment.
	if (!ALIGNED(src))
		return false;

	// Next check tag and message size
	PBFT_R_Message ret((PBFT_R_Message_rep*)src);
	if (!ret.has_tag(t, sz))
		return false;

	m = ret;
	return true;
}

bool PBFT_R_Message::encode(FILE* o)
{
	int csize = size();

	size_t sz = fwrite(&max_size, sizeof(int), 1, o);
	sz += fwrite(&csize, sizeof(int), 1, o);
	sz += fwrite(msg, 1, csize, o);

	return sz == 2U+csize;
}

bool PBFT_R_Message::decode(FILE* i)
{
	delete msg;

	size_t sz = fread(&max_size, sizeof(int), 1, i);
	msg = (PBFT_R_Message_rep*) malloc(max_size);

	int csize;
	sz += fread(&csize, sizeof(int), 1, i);

	if (msg == 0 || csize < 0 || csize > max_size)
return false;

  		sz += fread(msg, 1, csize, i);
	return sz == 2U+csize;
}

void PBFT_R_Message::init()
{
}

const char *PBFT_R_Message::stag()
{
	static const char *string_tags[] =
	{ "Free_message", "PBFT_R_Request", "PBFT_R_Reply", "PBFT_R_Pre_prepare",
			"PBFT_R_Prepare", "PBFT_R_Commit", "PBFT_R_Checkpoint",
			"PBFT_R_Status", "PBFT_R_View_change", "PBFT_R_New_view",
			"PBFT_R_View_change_ack", "PBFT_R_New_key", "PBFT_R_Meta_data",
			"PBFT_R_Meta_data_d", "PBFT_R_Data_tag", "PBFT_R_Fetch",
			"PBFT_R_Query_stable", "PBFT_R_Reply_stable" };
	return string_tags[tag()];
}
