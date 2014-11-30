#include <stdlib.h>
#include "th_assert.h"
#include "PBFT_C_Message.h"
#include "PBFT_C_Node.h"

PBFT_C_Message::PBFT_C_Message(unsigned sz) : msg(0), max_size(ALIGNED_SIZE(sz)) {
  if (sz != 0) {
    msg = (PBFT_C_Message_rep*) malloc(max_size);
    th_assert(ALIGNED(msg), "Improperly aligned pointer");
    msg->tag = -1;
    msg->size = 0;
    msg->extra = 0;
  }
}

PBFT_C_Message::PBFT_C_Message(int t, unsigned sz) {
  max_size = ALIGNED_SIZE(sz);
  msg = (PBFT_C_Message_rep*) malloc(max_size);
  th_assert(ALIGNED(msg), "Improperly aligned pointer");
  msg->tag = t;
  msg->size = max_size;
  msg->extra = 0;
}

PBFT_C_Message::PBFT_C_Message(PBFT_C_Message_rep *cont) {
  th_assert(ALIGNED(cont), "Improperly aligned pointer");
  msg = cont;
  max_size = -1; // To prevent contents from being deallocated or trimmed
}

PBFT_C_Message::~PBFT_C_Message() {
  if (max_size > 0) free(msg);
}

void PBFT_C_Message::trim() {
  if (max_size > 0 && realloc((char*)msg, msg->size)) {
    max_size = msg->size;
  }
}


void PBFT_C_Message::set_size(int size) {
  th_assert(msg && ALIGNED(msg), "Invalid state");
  th_assert(max_size < 0 || ALIGNED_SIZE(size) <= max_size, "Invalid state");
  int aligned = ALIGNED_SIZE(size);
  for (int i=size; i < aligned; i++) ((char*)msg)[i] = 0;
  msg->size = aligned;
}


bool PBFT_C_Message::convert(char *src, unsigned len, int t,
			     int sz, PBFT_C_Message &m) {
  // First check if src is large enough to hold a PBFT_C_Message_rep
  if (len < sizeof(PBFT_C_Message_rep)) return false;

  // Check alignment.
  if (!ALIGNED(src)) return false;

  // Next check tag and message size
  PBFT_C_Message ret((PBFT_C_Message_rep*)src);
  if (!ret.has_tag(t, sz)) return false;

  m = ret;
  return true;
}


bool PBFT_C_Message::encode(FILE* o) {
  int csize = size();

  size_t sz = fwrite(&max_size, sizeof(int), 1, o);
  sz += fwrite(&csize, sizeof(int), 1, o);
  sz += fwrite(msg, 1, csize, o);

  return sz == 2U+csize;
}


bool PBFT_C_Message::decode(FILE* i) {
  delete msg;

  size_t sz = fread(&max_size, sizeof(int), 1, i);
  msg = (PBFT_C_Message_rep*) malloc(max_size);

  int csize;
  sz += fread(&csize, sizeof(int), 1, i);

  if (msg == 0 || csize < 0 || csize > max_size)
    return false;

  sz += fread(msg, 1, csize, i);
  return sz == 2U+csize;
}


void PBFT_C_Message::init() {
}


const char *PBFT_C_Message::stag() {
  static const char *string_tags[] = {"Free_message",
				  "PBFT_C_Request",
				  "PBFT_C_Reply",
				  "PBFT_C_Pre_prepare",
				  "PBFT_C_Prepare",
				  "PBFT_C_Commit",
				  "PBFT_C_Checkpoint",
				  "PBFT_C_Status",
				  "PBFT_C_View_change",
				  "PBFT_C_New_view",
				  "PBFT_C_View_change_ack",
				  "PBFT_C_New_key",
				  "PBFT_C_Meta_data",
                                  "PBFT_C_Meta_data_d",
				  "PBFT_C_Data_tag",
				  "PBFT_C_Fetch",
                                  "PBFT_C_Query_stable",
                                  "PBFT_C_Reply_stable"};
  return string_tags[tag()];
}
