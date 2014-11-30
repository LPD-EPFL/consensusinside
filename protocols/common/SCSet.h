#ifndef _SCSet_h
#define _SCSet_h 1

#include <stdlib.h>
#include <malloc.h>

#include "types.h"

template <class T> class SCSet {
  //
  // SCSet of T*. Type T must have an "operator==" method which is used
  // as the set's key.
  //
public:
  SCSet(int sz);
  // Effects: Creates a set that can hold up to "sz" elements.

  ~SCSet();
  // Effects: Delete set and all associated storage.

  bool has(T *e);
  bool store(T *e);
  // Effects: Adds "e" to the set (if it has a valid id not in the
  // set) and returns true only if element wasn't in the set. Otherwise,
  // returns false and does nothing. Use full() to check whether you element
  // wasn't added because of the lack of space.

  T* fetch(int id);
  unsigned int fetch_count(int id);
  // Effects: returns a pointer to the element in the set with 
  // identifier "id" or 0 if there is no such element.

  T* remove(int id);
  // Effects: Removes the element with identifier "id" from the set 
  // and returns a pointer to it.

  void clear();
  // Effects: Removes all elements from the set and deletes them.

  void non_destructive_clear();
  // Effects: Removes all elements from the set without deleting them.

  int size() const;
  // Effects: Returns the number of elements in the set.

  bool full() const;
  // Effects: Returns true if set is full


private:
  int max_size;
  T **elems;
  unsigned int *counter;
  int cur_size;
};

template <class T>
inline int SCSet<T>::size() const { return cur_size; }

template <class T>
inline bool SCSet<T>::full() const { return cur_size == max_size; }

template <class T> 
SCSet<T>::SCSet(int sz) : max_size(sz), cur_size(0) {
  elems = (T**) malloc(sizeof(T*)*sz);
  counter = new unsigned int[sz];
  for (int  i=0; i < max_size; i++) {
    elems[i] = 0;
    counter[i] = 0;
  }
}

template <class T> 
SCSet<T>::~SCSet() {
  for (int i=0; i < max_size; i++)
    if (elems[i]) delete elems[i];
  free(elems);
  delete [] counter;
}

template <class T> 
bool SCSet<T>::store(T *e) {
  int i = 0;
  while (i < cur_size) {
      if (e->operator==(*elems[i])) {
	  counter[i]++;
	  return false;
      }
      i++;
  }
  if (cur_size == max_size)
      return false;
  elems[cur_size] = e;
  counter[cur_size] = 1;
  ++cur_size;
  return true;
}

template <class T> 
bool SCSet<T>::has(T *e) {
  int i = 0;
  while (i < cur_size) {
      if (e->operator==(*elems[i])) {
	  return true;
      }
      i++;
  }
  return false;
}

template <class T>
T *SCSet<T>::fetch(int id) {
  if (id >= max_size || id > cur_size || id < 0 || elems[id] == 0)
      return 0;
  T *ret = elems[id];
  return ret;
}

template <class T>
unsigned int SCSet<T>::fetch_count(int id) {
  if (id >= max_size || id > cur_size || id < 0 || elems[id] == 0)
      return 0;
  return counter[id];
}

template <class T>
T *SCSet<T>::remove(int id) {
  if (id >= max_size || id > cur_size || id < 0 || elems[id] == 0)
    return 0;
  T *ret = elems[id];
  for (int i = id; i < cur_size-1; i++) {
      elems[i] = elems[i+1];
      counter[i] = counter[i+1];
  }
  elems[cur_size] = 0;
  counter[cur_size] = 0;
  cur_size--;
  return ret;
}

template <class T>
void SCSet<T>::clear() {
  for (int i=0; i < max_size; i++) {
    delete elems[i];
    elems[i] = NULL;
    counter[i] = 0;
  }
  cur_size = 0;
}

template <class T>
void SCSet<T>::non_destructive_clear() {
  for (int i=0; i < max_size; i++) {
    elems[i] = 0;
    counter[i] = 0;
  }
  cur_size = 0;
}

#endif // _SCSet_h
