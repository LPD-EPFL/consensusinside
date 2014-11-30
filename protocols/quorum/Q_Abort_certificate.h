#ifndef _Q_Abort_certificate_h
#define _Q_Abort_certificate_h 1

#include "Array.h"
#include "types.h"
#include "Q_Abort.h"
#include "Bitmap.h"

class Ac_entry
{
public:
   inline Ac_entry()
   {
   }

   inline Ac_entry(Q_Abort *a)
   {
      abort = a;
   }

   inline ~Ac_entry()
   {
      delete abort;
   }

   Q_Abort *abort;
};

class Q_Abort_certificate
{

public:

   Q_Abort_certificate(int comp, int num_rep);

   ~Q_Abort_certificate();

   bool add(Q_Abort *abort);

   void clear();

   int size() const;

   bool is_complete() const;

   Ac_entry* operator[](int index) const;

private:
   Array<Ac_entry*> ac;
   int count;
   int complete;
};

inline int Q_Abort_certificate::size() const
{
   return ac.size();
}

inline bool Q_Abort_certificate::is_complete() const
{
   return count >= complete;
}

inline Ac_entry* Q_Abort_certificate::operator[](int index) const
{
   th_assert((index >= 0) && (index < ac.size()), "array index out of bounds");
   return ac[index];
}

#endif
