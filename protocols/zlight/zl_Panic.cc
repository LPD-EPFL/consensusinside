#include <strings.h>
#include "th_assert.h"
#include "zl_Message_tags.h"
#include "zl_Replica.h"
#include "zl_Panic.h"
#include "zl_Request.h"
#include "zl_Principal.h"

#define MAX_NON_DET_SIZE 8

zl_Panic::zl_Panic(zl_Panic_rep *cont)
{
   th_assert(ALIGNED(cont), "Improperly aligned pointer");
   msg = cont;
   max_size = -1; // To prevent contents from being deallocated or trimmed
}

zl_Panic::zl_Panic(zl_Request *req) :
   zl_Message(zl_Panic_tag, zl_Max_message_size)
{
   rep().cid = req->client_id();
   rep().req_id = req->request_id();

   set_size(sizeof(zl_Panic_rep));
}

bool zl_Panic::convert(zl_Message *m1, zl_Panic *&m2)
{
   if (!m1->has_tag(zl_Panic_tag, sizeof(zl_Panic_rep)))
      return false;

   //  m1->trim(); We trim the OR message after authenticating the message
   m2 = (zl_Panic*)m1;
   return true;
}

