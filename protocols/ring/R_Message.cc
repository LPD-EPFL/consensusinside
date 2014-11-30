#include <stdlib.h>
#include "th_assert.h"
#include "R_Message.h"

R_Message::R_Message(unsigned sz) :
   msg(0), max_size(ALIGNED_SIZE(sz))
{
   if (sz != 0)
   {
      //      msg = (R_Message_rep*) a->malloc(max_size);
      msg = (R_Message_rep*) malloc(max_size);
      th_assert(ALIGNED(msg), "Improperly aligned pointer");
      msg->tag = -1;
      msg->size = 0;
      msg->extra = 0;
   }
}

R_Message::R_Message(int t, unsigned sz)
{
   max_size = ALIGNED_SIZE(sz);
   msg = (R_Message_rep*) malloc(max_size);
   th_assert(ALIGNED(msg), "Improperly aligned pointer");
   msg->tag = t;
   msg->size = max_size;
   msg->extra = 0;
}

R_Message::R_Message(R_Message_rep *cont)
{
   th_assert(ALIGNED(cont), "Improperly aligned pointer");
   msg = cont;
   //max_size = msg->size;
   //if(msg->size == 0){
//	max_size = R_Max_message_size;
 //   }
    max_size = -1; // To prevent contents from being deallocated or trimmed
    //max_size = R_Max_message_size;
}

R_Message::~R_Message()
{
   if (max_size > 0) free(msg);
     // free((char*)msg);

}

void R_Message::trim()
{
   int nsz = ALIGNED_SIZE(msg->size);
   void *nptr;
   if (max_size > 0&& (nptr=realloc((char*)msg, nsz)) != NULL)
   {
      max_size = nsz;
      msg = (R_Message_rep*)nptr;
   }
}

void R_Message::set_size(int size)
{
   th_assert(msg && ALIGNED(msg), "Invalid state");
   th_assert(max_size < 0|| ALIGNED_SIZE(size) <= max_size, "Invalid state");
   int aligned= ALIGNED_SIZE(size);
   for (int i=size; i < aligned; i++)
   {
      ((char*)msg)[i] = 0;
   }
   msg->size = aligned;
}
