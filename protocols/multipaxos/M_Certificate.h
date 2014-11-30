#ifndef _M_HighestCounter_h
#define _M_HighestCounter_h 1

#include <sys/time.h>
#include "types.h"
#include "parameters.h"
#include "Bitmap.h"
#include "M_Node.h"

template<class T> class M_Certificate
{
   //
   // A certificate is a set of "matching" messages from different
   // replicas.
   //
   // T must have the following methods:
   // bool match(T*);
   // // Effects: Returns true iff the messages match
   //
   // int id();
   // // Effects: Returns the identifier of the principal that
   // // sent the message.
   //
   // bool verify();
   // // Effects: Returns true iff the message is properly authenticated
   // // and statically correct.
   //
   // bool full();
   // // Effects: Returns true iff the message is full

public:
   M_Certificate(int complete = 0);
   // Requires: "complete" >= f+1 or 0
   // Effects: Creates an empty certificate. The certificate is
   // complete when it contains at least "complete" matching messages
   // from different replicas. If the complete argument is omitted (or
   // 0) it is taken to be 3f+1.

   ~M_Certificate();
   // Effects: Deletes certificate and all the messages it contains.

	//Added by Maysam Yabandeh
   bool add(T *m, M_Node *n);
   // Effects: Adds "m" to the certificate and returns true provided
   // "m" satisfies:
   // 1. there is no message from "m.id()" in the certificate
   // 2. "m->verify() == true"
   // 3. if "cvalue() != 0", "cvalue()->match(m)";
   // otherwise, it has no effect on this and returns false.  This
   // becomes the owner of "m" (i.e., no other code should delete "m"
   // or retain pointers to "m".)

   bool add_mine(T *m, M_Node *n);
   // Effects: Adds self message "m" to the certificate and returns true provided
   // "m" satisfies:
   // 1. there is no message from "m.id()" in the certificate
   // 2. "m->verify() == true"
   // 3. if "cvalue() != 0", "cvalue()->match(m)";
   // otherwise, it has no effect on this and returns false.  This
   // becomes the owner of "m" (i.e., no other code should delete "m"
   // or retain pointers to "m".)

   T *cvalue() const;
   // Effects: Returns the correct message value for this certificate
   // or 0 if this value is not known. Note that the certificate
   // retains ownership over the returned value (e.g., if clear or
   // mark_stale are called the value may be deleted.)

   bool is_complete() const;

   void clear();
   // Effects: Discards all messages in certificate

   void set_complete(int c);
   // Effects: Sets the value of complete
private:
   int complete;
   int count;
   Bitmap bmap; // bitmap with replicas whose message is in this.

   T *value; // correct certificate value or 0 if unknown.
};

template<class T> inline T *M_Certificate<T>::cvalue() const
{
   return value;
}

template<class T> inline bool M_Certificate<T>::is_complete() const
{
   //fprintf(stderr, "gags.count = %d\n", count);
   return count >= complete;
}

template<class T> inline void M_Certificate<T>::clear()
{
   count = 0;
   bmap.clear();
   value = 0;
}

template<class T> inline void M_Certificate<T>::set_complete(int c)
{
  complete = c;
}


template<class T> class M_HighestCounter
{
   //
   // A certificate is a set of "matching" messages from different
   // replicas.
   //
   // T must have the following methods:
   // bool match(T*);
   // // Effects: Returns true iff the messages match
   //
   // int id();
   // // Effects: Returns the identifier of the principal that
   // // sent the message.
   //
   // bool verify();
   // // Effects: Returns true iff the message is properly authenticated
   // // and statically correct.
   //
   // bool full();
   // // Effects: Returns true iff the message is full

public:
   M_HighestCounter(int complete = 0);
   // Requires: "complete" >= f+1 or 0
   // Effects: Creates an empty certificate. The certificate is
   // complete when it contains at least "complete" matching messages
   // from different replicas. If the complete argument is omitted (or
   // 0) it is taken to be 3f+1.

   ~M_HighestCounter();
   // Effects: Deletes certificate and all the messages it contains.

	//Added by Maysam Yabandeh
   bool add(T *m);
   // Effects: Adds "m" to the certificate and returns true provided
   // "m" satisfies:
   // 1. there is no message from "m.id()" in the certificate
   // 2. "m->verify() == true"
   // 3. if "cvalue() != 0", "cvalue()->match(m)";
   // otherwise, it has no effect on this and returns false.  This
   // becomes the owner of "m" (i.e., no other code should delete "m"
   // or retain pointers to "m".)

   bool add_mine(T *m, M_Node *n);
   // Effects: Adds self message "m" to the certificate and returns true provided
   // "m" satisfies:
   // 1. there is no message from "m.id()" in the certificate
   // 2. "m->verify() == true"
   // 3. if "cvalue() != 0", "cvalue()->match(m)";
   // otherwise, it has no effect on this and returns false.  This
   // becomes the owner of "m" (i.e., no other code should delete "m"
   // or retain pointers to "m".)

   T *cvalue() const;
   // Effects: Returns the correct message value for this certificate
   // or 0 if this value is not known. Note that the certificate
   // retains ownership over the returned value (e.g., if clear or
   // mark_stale are called the value may be deleted.)

   bool is_complete() const;

   void clear();
   // Effects: Discards all messages in certificate

   void set_complete(int c);
   // Effects: Sets the value of complete
private:
   int complete;
   int count;
	bool bmap[5];
   //Bitmap bmap; // bitmap with replicas whose message is in this.

   T *value; // correct certificate value or 0 if unknown.
};

template<class T> class M_Counter
{

public:
   //M_Counter(int complete = 0);
   M_Counter();

	//Added by Maysam Yabandeh
   bool add(T *m);
   bool is_complete() const;
   void clear();
   // Effects: Discards all messages in certificate
   void set_complete(int c);
   // Effects: Sets the value of complete
private:
   int complete;
   int count;
	bool bmap[5];
	//Added by Maysam Yabandeh
	//Bitmap has a problem with std::map
   //Bitmap bmap; // bitmap with replicas whose message is in this.
};

template<class T> inline T *M_HighestCounter<T>::cvalue() const
{
   return value;
}

template<class T> inline bool M_HighestCounter<T>::is_complete() const
{
   //fprintf(stderr, "gags.count = %d >= %d\n", count, complete);
   return count >= complete;
}

template<class T> inline void M_HighestCounter<T>::clear()
{
   count = 0;
   //bmap.clear();
   value = 0;
	for (int i = 0; i < 5; i++)
		bmap[i] = false;
}

template<class T> inline void M_HighestCounter<T>::set_complete(int c)
{
  complete = c;
}

template<class T> inline bool M_Counter<T>::is_complete() const
{
   //fprintf(stderr, "counter.count = %d >= %d\n", count, complete);
   return count >= complete;
}

template<class T> inline void M_Counter<T>::clear()
{
   count = 0;
	for (int i = 0; i < 5; i++)
		bmap[i] = false;
   //bmap.clear();
}

template<class T> inline void M_Counter<T>::set_complete(int c)
{
  complete = c;
}

#endif // M_HighestCounter_h
