#include <strings.h>
#include "th_assert.h"
#include "Q_Message_tags.h"
#include "Q_Replica.h"
#include "Q_Panic.h"
#include "Q_Request.h"
#include "Q_Principal.h"

#define MAX_NON_DET_SIZE 8

Q_Panic::Q_Panic(Q_Panic_rep *cont)
{
   th_assert(ALIGNED(cont), "Improperly aligned pointer");
   msg = cont;
   max_size = -1; // To prevent contents from being deallocated or trimmed
}

Q_Panic::Q_Panic(Q_Request *req) :
   Q_Message(Q_Panic_tag, Q_Max_message_size)
{
   rep().cid = req->client_id();
   rep().req_id = req->request_id();

   set_size(sizeof(Q_Panic_rep));
}

bool Q_Panic::convert(Q_Message *m1, Q_Panic *&m2)
{
   if (!m1->has_tag(Q_Panic_tag, sizeof(Q_Panic_rep)))
      return false;

   //  m1->trim(); We trim the OR message after authenticating the message
   m2 = (Q_Panic*)m1;
   return true;
}

