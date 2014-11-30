#include <strings.h>
#include "th_assert.h"
#include "Q_Message_tags.h"
// #include "Q_Replica.h"
#include "Q_Abort.h"
#include "Q_Request.h"
#include "Q_Principal.h"
#include "Q_Node.h"
#include "Array.h"
#include "Digest.h"

#define MAX_NON_DET_SIZE 8

Q_Abort::Q_Abort(int p_replica, Request_id p_rid, Req_history_log &req_h) :
   Q_Message(Q_Abort_tag, Q_Max_message_size)
{
   rep().rid = p_rid;
   rep().replica = p_replica;
   int req_h_size = req_h.array().size();

   //fprintf(stderr, "Constructor of Q_Abort for request (cid = %d, req_id = %llu, replica = %d)\n", p_cid, p_rid, p_replica);

   // We don't know whether [cid, req_id] is in the request history or not
   int offset = sizeof(Q_Abort_rep);
   int i = 0;

   for (i=0; i<req_h_size; i++)
   {
      Rh_entry rhe = req_h.array()[i];
      if (rhe.req == NULL) {
	  continue;
      }
      if (offset + (sizeof(int)+sizeof(Request_id) + sizeof(Digest))
            > Q_Max_message_size - Q_node->sig_size())
      {
         fprintf(stderr, "Too many messages in the abort message (%d)\n", req_h_size);
         exit(1);
      }

      int cur_cid = rhe.req->client_id();
      Request_id cur_rid = rhe.req->request_id();
      Digest cur_d = rhe.req->digest();

      /*
      if (cur_cid == p_cid && cur_rid == p_rid)
      {
         rep().aborted_request_digest = cur_d;
         break;
      }
      */

      memcpy(contents()+offset, (char *)&cur_cid, sizeof(int));
      offset+=sizeof(int);
      memcpy(contents()+offset, (char *)&cur_rid, sizeof(Request_id));
      offset+=sizeof(Request_id);
      memcpy(contents()+offset, cur_d.digest(), sizeof(Digest));
      offset+=sizeof(Digest);
   }

   //fprintf(stderr, "history size = %d\n", i);
   //fprintf(stderr, "offset = %d\n", offset);
   rep().hist_size = i;

   set_size(offset);

   digest() = Digest(contents() + sizeof(Q_Abort_rep), offset - sizeof(Q_Abort_rep));
   //fprintf(stderr, "end of creation\n");
}

Q_Abort::Q_Abort(Q_Abort_rep *cont)
{
   th_assert(ALIGNED(cont), "Improperly aligned pointer");
   msg = cont;
   max_size = -1; // To prevent contents from being deallocated or trimmed
}

bool Q_Abort::convert(Q_Message *m1, Q_Abort *&m2)
{
   if (!m1->has_tag(Q_Abort_tag, sizeof(Q_Abort_rep)))
      return false;

   m1->trim();
   m2 = (Q_Abort*)m1;
   return true;
}

void Q_Abort::sign()
{
   int old_size = sizeof(Q_Abort_rep) + (rep().hist_size * ((sizeof(int) + sizeof(Request_id) + sizeof(Digest))));
   if (msize() - old_size < Q_node->sig_size())
   {
      fprintf(stderr, "Q_Abort: Not enough space to sign message\n");
      exit(1);
   }
   set_size(old_size + Q_node->sig_size());
   // We need to add a digest here of the full content...
   //Q_node->gen_signature(contents(), sizeof(Q_Abort_rep), contents() +old_size);
   //fprintf(stderr, "signature is made at offset %d\n", old_size);
}

// This method must be called by a backup-BFT replica
bool Q_Abort::verify()
{
   // Replies must be sent by replicas.
   if (!Q_node->is_replica(id()))
   {
      return false;
   }

return true;
   //Q_Principal *abstract_replica = Q_node->i_to_p(rep().replica);
//
   //unsigned int size_wo_MAC = sizeof(Q_Abort_rep) + (rep().hist_size * ((sizeof(int) + sizeof(Request_id) + sizeof(Digest))));
//
   //// Check digest
   //Digest d(contents() + sizeof(Q_Abort_rep), size_wo_MAC - sizeof(Q_Abort_rep));
//
   //if (d != digest())
   //{
      //fprintf(stderr, "Q_Abort::verify: digest is not equal\n");
      //return false;
   //}
//
   //// Check signature (enough space)
   //if (size()-size_wo_MAC < abstract_replica->sig_size())
   //{
      //return false;
   //}
//
   //// Check signature.
   //bool ret = abstract_replica->verify_signature(contents(), sizeof(Q_Abort_rep),
         //contents() + size_wo_MAC, true);

   //return ret;
}

#if 0
bool Q_Abort::verify_by_abstract()
{
   // Replies must be sent by replicas.
   if (!Q_node->is_replica(id()))
   {
      fprintf(stderr, "Q_Abort::verify_by_abstract: reply is not sent by a replica\n");
      return false;
   }

   Q_Principal *replica = Q_node->i_to_p(id());

   unsigned int size_wo_MAC = sizeof(Q_Abort_rep) + (rep().hist_size * ((sizeof(int) + sizeof(Request_id) + sizeof(Digest))));

   // Check digest
   Digest d(contents() + sizeof(Q_Abort_rep), size_wo_MAC - sizeof(Q_Abort_rep));

   if (d != digest())
   {
      fprintf(stderr, "Q_Abort::verify_by_abstract: digest is not equal\n");
      return false;
   }

   // Check signature (enough space)
   if (size()-size_wo_MAC < replica->sig_size())
   {
      fprintf(stderr, "Q_Abort::verify_by_abstract: size is not big enough\n");
      return false;
   }

   // Check signature
   bool ret = replica->verify_signature(contents(), sizeof(Q_Abort_rep),
         contents() + size_wo_MAC);

   return ret;
}
#endif
