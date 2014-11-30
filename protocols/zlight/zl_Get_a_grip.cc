#include <strings.h>
#include "th_assert.h"
#include "zl_Message_tags.h"
#include "zl_Replica.h"
#include "zl_Get_a_grip.h"
#include "zl_Request.h"
#include "zl_Principal.h"

#define MAX_NON_DET_SIZE 8

zl_Get_a_grip::zl_Get_a_grip(zl_Get_a_grip_rep *cont)
{
   th_assert(ALIGNED(cont), "Improperly aligned pointer");
   msg = cont;
   max_size = -1; // To prevent contents from being deallocated or trimmed
}

zl_Get_a_grip::zl_Get_a_grip(int cid, Request_id req_id, int replica, Seqno s, zl_Request *r) :
   zl_Message(zl_Get_a_grip_tag, zl_Max_message_size)
{
   rep().cid = cid;
   rep().req_id = req_id;
   rep().replica = replica;
   rep().seqno = s;

   int size = sizeof(zl_Get_a_grip_rep) + r->size();

   set_size(size);
   memcpy(stored_request(), r->contents(), r->size());

   //   fprintf(stderr, "zl_Get_a_grip: setting size to %d (sizeof(zl_Get_a_grip) = %d)\n", size,
   //   sizeof(zl_Get_a_grip_rep));
   //   fprintf(stderr, "The stored reply has size %d\n", ((zl_Message_rep *)reply())->size);
}

bool zl_Get_a_grip::convert(zl_Message *m1, zl_Get_a_grip *&m2)
{
   if (!m1->has_tag(zl_Get_a_grip_tag, sizeof(zl_Get_a_grip_rep)))
      return false;

   //  m1->trim(); We trim the OR message after authenticating the message
   m2 = (zl_Get_a_grip*)m1;
   return true;
}

