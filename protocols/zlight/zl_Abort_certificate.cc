#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "zl_Abort_certificate.h"
#include "Array.h"
#include "parameters.h"

zl_Abort_certificate::zl_Abort_certificate(int comp, int num_rep) :
   ac()
{
   //fprintf(stderr, "zl_Abort certificate initialized for %d replicas\n", num_rep);
   ac.append(NULL, num_rep);
   count = 0;
   complete = comp;
}

zl_Abort_certificate::~zl_Abort_certificate()
{
}

bool zl_Abort_certificate::add(zl_Abort *abort)
{
   const int id = abort->id();

   if (!ac[id])
   {
      Ac_entry *ace = new Ac_entry(abort);
      ac[id] = ace;
      count++;
      return true;
   }
   return false;
}

void zl_Abort_certificate::clear()
{
   for (int i = 0; i<ac.size() ; i++)
   {
      delete ac[i];
      ac[i] = NULL;
   }

   count = 0;
}
