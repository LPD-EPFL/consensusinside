#include <strings.h>
#include "th_assert.h"
#include "zl_Message_tags.h"
#include "zl_Replica.h"
#include "zl_Order_request.h"
#include "zl_Request.h"
#include "zl_Principal.h"

#define MAX_NON_DET_SIZE 8

zl_Order_request::zl_Order_request(zl_Order_request_rep *cont)
{
   th_assert(ALIGNED(cont), "Improperly aligned pointer");
   msg = cont;
   max_size = -1; // To prevent contents from being deallocated or trimmed
}

zl_Order_request::zl_Order_request(View view, Seqno s, zl_Request *req) :
   zl_Message(zl_Order_request_tag, zl_Max_message_size)
{
   rep().v = view;
   rep().seq_num = s;
   rep().cid = req->client_id();
   rep().req_id = req->request_id();

   // piggyback the request on this message, and as a quick hack, just send all macs, instead only one for the receiver.
   // let the receiver decide whether its mac is correct.
   memcpy(stored_request(), req->contents(), req->size());

   set_size(sizeof(zl_Order_request_rep) + req->size() + zl_node->auth_size());
   zl_node->gen_auth(contents(), sizeof(zl_Order_request_rep), contents()
	+sizeof(zl_Order_request_rep));
}

bool zl_Order_request::convert(zl_Message *m1, zl_Order_request *&m2)
{
   if (!m1->has_tag(zl_Order_request_tag, sizeof(zl_Order_request_rep)))
      return false;

   //  m1->trim(); We trim the OR message after authenticating the message
   m2 = (zl_Order_request*)m1;
   return true;
}

bool zl_Order_request::verify()
{
   // TODO primary = 0 is hardcoded
   return zl_node->verify_auth(0, contents(), sizeof(zl_Order_request_rep),
         contents() + sizeof(zl_Order_request_rep));
}

