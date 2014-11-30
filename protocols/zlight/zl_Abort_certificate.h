#ifndef _zl_Abort_certificate_h
#define _zl_Abort_certificate_h 1

#include "Array.h"
#include "types.h"
#include "zl_Abort.h"
#include "Bitmap.h"

class Ac_entry
{
public:
   inline Ac_entry()
   {
   }

   inline Ac_entry(zl_Abort *a)
   {
      abort = a;
   }

   inline ~Ac_entry()
   {
      delete abort;
   }

   zl_Abort *abort;
};

class zl_Abort_certificate
{

public:

   zl_Abort_certificate(int comp, int num_rep);

   ~zl_Abort_certificate();

   bool add(zl_Abort *abort);

   void clear();

   int size() const;

   bool is_complete() const;

   Ac_entry* operator[](int index) const;

private:
   Array<Ac_entry*> ac;
   int count;
   int complete;
};

inline int zl_Abort_certificate::size() const
{
   return ac.size();
}

inline bool zl_Abort_certificate::is_complete() const
{
   return count >= complete;
}

inline Ac_entry* zl_Abort_certificate::operator[](int index) const
{
   th_assert((index >= 0) && (index < ac.size()), "array index out of bounds");
   return ac[index];
}

#endif
