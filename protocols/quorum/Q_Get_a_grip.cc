#include <strings.h>
#include "th_assert.h"
#include "Q_Message_tags.h"
#include "Q_Replica.h"
#include "Q_Get_a_grip.h"
#include "Q_Request.h"
#include "Q_Principal.h"

#define MAX_NON_DET_SIZE 8

Q_Get_a_grip::Q_Get_a_grip(Q_Get_a_grip_rep *cont)
{
   th_assert(ALIGNED(cont), "Improperly aligned pointer");
   msg = cont;
   max_size = -1; // To prevent contents from being deallocated or trimmed
}

Q_Get_a_grip::Q_Get_a_grip(int cid, Request_id req_id, int replica, Seqno s, Q_Request *r) :
   Q_Message(Q_Get_a_grip_tag, Q_Max_message_size)
{
   rep().cid = cid;
   rep().req_id = req_id;
   rep().replica = replica;
   rep().seqno = s;

   int size = sizeof(Q_Get_a_grip_rep) + r->size();

   set_size(size);
   memcpy(stored_request(), r->contents(), r->size());

   //   fprintf(stderr, "Q_Get_a_grip: setting size to %d (sizeof(Q_Get_a_grip) = %d)\n", size,
   //   sizeof(Q_Get_a_grip_rep));
   //   fprintf(stderr, "The stored reply has size %d\n", ((Q_Message_rep *)reply())->size);
}

bool Q_Get_a_grip::convert(Q_Message *m1, Q_Get_a_grip *&m2)
{
   if (!m1->has_tag(Q_Get_a_grip_tag, sizeof(Q_Get_a_grip_rep)))
      return false;

   //  m1->trim(); We trim the OR message after authenticating the message
   m2 = (Q_Get_a_grip*)m1;
   return true;
}

