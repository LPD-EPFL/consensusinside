#include <stdlib.h>
#include "th_assert.h"
#include "O_Message.h"

O_Message::O_Message(unsigned sz) :
   msg(0), max_size(ALIGNED_SIZE(sz))
{
   if (sz != 0)
   {
      msg = (O_Message_rep*) malloc(max_size);
      th_assert(msg != 0, "Not enough memory\n");
      th_assert(ALIGNED(msg), "Improperly aligned pointer");
      msg->tag = -1;
      msg->size = 0;
      msg->extra = 0;
   }
}

O_Message::O_Message(int t, unsigned sz)
{
   max_size = ALIGNED_SIZE(sz);
   msg = (O_Message_rep*) malloc(max_size);
   th_assert(msg != 0, "Not enough memory\n");
   th_assert(ALIGNED(msg), "Improperly aligned pointer");
   msg->tag = t;
   msg->size = max_size;
   msg->extra = 0;
}

O_Message::O_Message(O_Message_rep *cont)
{
   th_assert(ALIGNED(cont), "Improperly aligned pointer");
   msg = cont;
   //max_size = msg->size;
   //if(msg->size == 0){
//	max_size = O_Max_message_size;
 //   }
    max_size = -1; // To prevent contents from being deallocated or trimmed
    //max_size = O_Max_message_size;
}

O_Message::~O_Message()
{
   if (max_size > 0) free(msg);
     // free((char*)msg);

}

void O_Message::trim()
{
   if (max_size > 0&& realloc((char*)msg, msg->size))
   {
      max_size = msg->size;
   }
}

void O_Message::set_size(int size)
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
