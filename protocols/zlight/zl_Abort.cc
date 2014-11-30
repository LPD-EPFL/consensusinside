#include <strings.h>
#include "th_assert.h"
#include "zl_Message_tags.h"
// #include "zl_Replica.h"
#include "zl_Abort.h"
#include "zl_Request.h"
#include "zl_Principal.h"
#include "zl_Node.h"
#include "Array.h"
#include "Digest.h"

#define MAX_NON_DET_SIZE 8

zl_Abort::zl_Abort(int p_replica, Request_id p_rid, Req_history_log &req_h) :
   zl_Message(zl_Abort_tag, zl_Max_message_size)
{
   rep().rid = p_rid;
   rep().replica = p_replica;
   int req_h_size = req_h.array().size();

   //fprintf(stderr, "Constructor of zl_Abort for request (cid = %d, req_id = %llu, replica = %d)\n", p_cid, p_rid, p_replica);

   // We don't know whether [cid, req_id] is in the request history or not
   int offset = sizeof(zl_Abort_rep);
   int i = 0;

   for (i=0; i<req_h_size; i++)
   {
      if (offset + (sizeof(int)+sizeof(Request_id) + sizeof(Digest))
            > zl_Max_message_size)
      {
         fprintf(stderr, "Too many messages in the abort message\n");
         exit(1);
      }
      Rh_entry rhe = req_h.array()[i];
      int cur_cid = rhe.req->client_id();
      Request_id cur_rid = rhe.req->request_id();
      Digest cur_d = rhe.req->digest();

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

   digest() = Digest(contents() + sizeof(zl_Abort_rep), offset - sizeof(zl_Abort_rep));

   //fprintf(stderr, "end of creation\n");
}

zl_Abort::zl_Abort(zl_Abort_rep *cont)
{
   th_assert(ALIGNED(cont), "Improperly aligned pointer");
   msg = cont;
   max_size = -1; // To prevent contents from being deallocated or trimmed
}

bool zl_Abort::convert(zl_Message *m1, zl_Abort *&m2)
{
   if (!m1->has_tag(zl_Abort_tag, sizeof(zl_Abort_rep)))
      return false;

   m1->trim();
   m2 = (zl_Abort*)m1;
   return true;
}

void zl_Abort::sign()
{
   int old_size = sizeof(zl_Abort_rep) + (rep().hist_size * ((sizeof(int) + sizeof(Request_id) + sizeof(Digest))));
   if (msize() - old_size < zl_node->sig_size())
   {
      fprintf(stderr, "Not enough space to sign message\n");
      exit(1);
   }
   set_size(old_size + zl_node->sig_size());
   // We need to add a digest here of the full content...
   zl_node->gen_signature(contents(), sizeof(zl_Abort_rep), contents() +old_size);
   //fprintf(stderr, "signature is made at offset %d\n", old_size);
}

// This method must be called by a backup-BFT replica
bool zl_Abort::verify()
{
   // Replies must be sent by replicas.
   if (!zl_node->is_replica(id()))
   {
      return false;
   }

   zl_Principal *abstract_replica = zl_node->i_to_p(rep().replica);

   unsigned int size_wo_MAC = sizeof(zl_Abort_rep) + (rep().hist_size * ((sizeof(int) + sizeof(Request_id) + sizeof(Digest))));

   // Check digest
   Digest d(contents() + sizeof(zl_Abort_rep), size_wo_MAC - sizeof(zl_Abort_rep));

   if (d != digest())
   {
      fprintf(stderr, "zl_Abort::verify: digest is not equal\n");
      return false;
   }

   // Check signature (enough space)
   if (size()-size_wo_MAC < abstract_replica->sig_size())
   {
      return false;
   }

   // Check signature.
   bool ret = abstract_replica->verify_signature(contents(), sizeof(zl_Abort_rep),
         contents() + size_wo_MAC, true);

   return ret;
}

#if 0
bool zl_Abort::verify_by_abstract()
{
   // Replies must be sent by replicas.
   if (!zl_node->is_replica(id()))
   {
      fprintf(stderr, "zl_Abort::verify_by_abstract: reply is not sent by a replica\n");
      return false;
   }

   zl_Principal *replica = zl_node->i_to_p(id());

   unsigned int size_wo_MAC = sizeof(zl_Abort_rep) + (rep().hist_size * ((sizeof(int) + sizeof(Request_id) + sizeof(Digest))));

   // Check digest
   Digest d(contents() + sizeof(zl_Abort_rep), size_wo_MAC - sizeof(zl_Abort_rep));

   if (d != digest())
   {
      fprintf(stderr, "zl_Abort::verify_by_abstract: digest is not equal\n");
      return false;
   }

   // Check signature (enough space)
   if (size()-size_wo_MAC < replica->sig_size())
   {
      fprintf(stderr, "zl_Abort::verify_by_abstract: size is not big enough\n");
      return false;
   }

   // Check signature
   bool ret = replica->verify_signature(contents(), sizeof(zl_Abort_rep),
         contents() + size_wo_MAC);

   return ret;
}
#endif
