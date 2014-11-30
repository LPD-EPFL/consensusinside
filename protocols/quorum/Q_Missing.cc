#include <strings.h>
#include "th_assert.h"
#include "Q_Message_tags.h"
// #include "Q_Replica.h"
#include "Q_Missing.h"
#include "Q_Request.h"
#include "Q_Principal.h"
#include "Q_Node.h"
#include "Array.h"
#include "Digest.h"

Q_Missing::Q_Missing(int p_replica, AbortHistory *missing) :
   Q_Message(Q_Missing_tag, Q_Max_message_size)
{
   rep().replica = p_replica;
   int req_h_size = missing->size();

   //fprintf(stderr, "Constructor of Q_Missing for request (cid = %d, req_id = %llu, replica = %d)\n", p_cid, p_rid, p_replica);

   // We don't know whether [cid, req_id] is in the request history or not
   ssize_t offset = sizeof(Q_Missing_rep);
   int i = 0;

   for (i=0; i<req_h_size; i++)
   {
      if (offset + (sizeof(int)+sizeof(Request_id))
            > Q_Max_message_size)
      {
         fprintf(stderr, "Q_Missing: Too many messages in the abort message\n");
         exit(1);
      }
      int cur_cid = missing->slot(i)->cid;
      Request_id cur_rid = missing->slot(i)->rid;

      memcpy(contents()+offset, (char *)&cur_cid, sizeof(int));
      offset+=sizeof(int);
      memcpy(contents()+offset, (char *)&cur_rid, sizeof(Request_id));
      offset+=sizeof(Request_id);
   }

   //fprintf(stderr, "history size = %d\n", i);
   //fprintf(stderr, "offset = %d\n", offset);
   rep().hist_size = i;

   digest() = Digest(contents() + sizeof(Q_Missing_rep), offset - sizeof(Q_Missing_rep));

   set_size(offset);
   //fprintf(stderr, "end of creation\n");
}

Q_Missing::Q_Missing(Q_Missing_rep *cont)
{
   th_assert(ALIGNED(cont), "Improperly aligned pointer");
   msg = cont;
   max_size = -1; // To prevent contents from being deallocated or trimmed
}

bool Q_Missing::convert(Q_Message *m1, Q_Missing *&m2)
{
   if (!m1->has_tag(Q_Missing_tag, sizeof(Q_Missing_rep)))
      return false;

   m1->trim();
   m2 = (Q_Missing*)m1;
   return true;
}

void Q_Missing::sign()
{
   int old_size = sizeof(Q_Missing_rep) + (rep().hist_size * ((sizeof(int) + sizeof(Request_id) + sizeof(Digest))));
   if (msize() - old_size < Q_node->sig_size())
   {
      fprintf(stderr, "Q_Missing: Not enough space to sign message\n");
      exit(1);
   }
   set_size(old_size + Q_node->sig_size());
   // We need to add a digest here of the full content...
   //Q_node->gen_signature(contents(), sizeof(Q_Missing_rep), contents() +old_size);
   //fprintf(stderr, "signature is made at offset %d\n", old_size);
}

// This method must be called by a backup-BFT replica
bool Q_Missing::verify()
{
   // Replies must be sent by replicas.
   if (!Q_node->is_replica(id()))
   {
      return false;
   }
	return true;

   //Q_Principal *abstract_replica = Q_node->i_to_p(rep().replica);
//
   //unsigned int size_wo_MAC = sizeof(Q_Missing_rep) + (rep().hist_size * ((sizeof(int) + sizeof(Request_id) + sizeof(Digest))));
//
   //// Check digest
   //Digest d(contents() + sizeof(Q_Missing_rep), size_wo_MAC - sizeof(Q_Missing_rep));
//
   //if (d != digest())
   //{
      //fprintf(stderr, "Q_Missing::verify: digest is not equal\n");
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
   //bool ret = abstract_replica->verify_signature(contents(), sizeof(Q_Missing_rep),
         //contents() + size_wo_MAC, true);

   //return ret;
}
