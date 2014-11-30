#include "R_Message.h"
#include "R_Mes_queue.h"

R_Mes_queue::R_Mes_queue() :
   head(0), tail(0), nelems(0), nbytes(0)
{
}

bool R_Mes_queue::append(R_Message *m)
{
   PR_Node *cn = new PR_Node(m);

   nbytes += m->size();
   nelems++;

   if (head == 0)
   {
      head = tail = cn;
      cn->prev = cn->next = 0;
   }
   else
   {
      tail->next = cn;
      cn->prev = tail;
      cn->next = 0;
      tail = cn;
   }
   return true;
}

R_Message *R_Mes_queue::remove()
{
   if (head == 0)
      return 0;

   R_Message *ret = head->m;
   th_assert(ret != 0, "Invalid state");

   PR_Node* old_head = head;
   head = head->next;
   delete old_head;

   if (head != 0)
      head->prev = 0;
   else
      tail = 0;

   nelems--;
   nbytes -= ret->size();

   return ret;
}
