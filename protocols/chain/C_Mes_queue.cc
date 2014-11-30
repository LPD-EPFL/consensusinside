#include "C_Message.h"
#include "C_Mes_queue.h"

C_Mes_queue::C_Mes_queue() :
   head(0), tail(0), nelems(0), nbytes(0)
{
}

bool C_Mes_queue::append(C_Message *m)
{
   PC_Node *cn = new PC_Node(m);

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

C_Message *C_Mes_queue::remove()
{
   if (head == 0)
      return 0;

   C_Message *ret = head->m;
   th_assert(ret != 0, "Invalid state");

   PC_Node* old_head = head;
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
