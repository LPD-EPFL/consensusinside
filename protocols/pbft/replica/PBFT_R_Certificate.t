#include "PBFT_R_Certificate.h"
#include "PBFT_R_Node.h"

template <class T> 
PBFT_R_Certificate<T>::PBFT_R_Certificate(int comp) : bmap(Max_num_replicas) {
  max_size = PBFT_R_node->f()+1;
  vals = new PBFT_R_Message_val[max_size];
  cuPBFT_R_size = 0;
  correct = PBFT_R_node->f()+1;
  complete = (comp == 0) ? PBFT_R_node->f()*2+1 : comp;
  c = 0;
  mym = 0; 
}

template <class T> 
PBFT_R_Certificate<T>::~PBFT_R_Certificate() {
  delete [] vals;
}

template <class T> 
bool PBFT_R_Certificate<T>::add(T *m) {
  const int id = m->id();
  if (PBFT_R_node->is_PBFT_R_replica(id) && !bmap.test(id)) {
    // "m" was sent by a PBFT_R_replica that does not have a message in
    // the certificate
    if ((c == 0 || (c->count < complete && c->m->match(m))) && m->verify()) {
      // add "m" to the certificate	
      th_assert(id != PBFT_R_node->id(), "verify should return false for messages from self");
      
      bmap.set(id);	
      if (c) {
	c->count++;
	if (!c->m->full() && m->full()) {
	  // if c->m is not full and m is, replace c->m
	  delete c->m;
	  c->m = m;
	} else {
	  delete m;
	}
	return true;
      } 
      
      // Check if there is a value that matches "m"
      int i;
      for (i=0; i < cuPBFT_R_size; i++) {
	PBFT_R_Message_val &val = vals[i];
	if (val.m->match(m)) {
	  val.count++;
	  if (val.count >= correct) 
	    c = vals+i;
	  if (!val.m->full() && m->full()) {
	    // if val.m is not full and m is, replace val.m
	    delete val.m;
	    val.m = m;
	  } else {
	    delete m;
	  }
	  return true;
	}
      }
      
      // "m" has a new value.
      if (cuPBFT_R_size < max_size) {
	vals[cuPBFT_R_size].m = m;
	vals[cuPBFT_R_size++].count = 1;
	return true;
      } else {
	// Should only happen for replies to read-only requests.
	fprintf(stderr, "More than f+1 distinct values in certificate");
	clear();
      } 

    } else {
      if (m->verify()) bmap.set(id);
    }
  }
  delete m;
  return false;
}
    

template <class T> 
bool PBFT_R_Certificate<T>::add_mine(T *m) {
  th_assert(m->id() == PBFT_R_node->id(), "Invalid argument");
  th_assert(m->full(), "Invalid argument");

  if (c != 0 && !c->m->match(m)) {
    delete m;
    fprintf(stderr,"PBFT_R_Node is faulty, more than f faulty PBFT_R_replicas or faulty primary%s \n", m->stag());
    return false;
  }

  if (c == 0) {
    // Set m to be the correct value.
    int i;
    for (i=0; i < cuPBFT_R_size; i++) {
      if (vals[i].m->match(m)) {
	c = vals + i;
	break;
      }
    }
    
    if (c == 0) {
      c = vals;
      vals->count = 0;
    }
  }
  
  if (c->m == 0) {
    th_assert(cuPBFT_R_size == 0, "Invalid state");
    cuPBFT_R_size = 1;
  }

  delete c->m;
  c->m = m;
  c->count++;
  mym = m;
  t_sent = currentPBFT_R_Time();
  return true;
}


template <class T> 
void PBFT_R_Certificate<T>::mark_stale() {
  if (!is_complete()) {
    int i = 0;
    int old_cuPBFT_R_size = cuPBFT_R_size;
    if (mym) {
      th_assert(mym == c->m, "Broken invariant");
      c->m = 0;
      c->count = 0;
      c = vals;
      c->m = mym;
      c->count = 1;
      i = 1;
    } else {
      c = 0;
    }
    cuPBFT_R_size = i;    

    for (; i < old_cuPBFT_R_size; i++) vals[i].clear();
    bmap.clear();
  }
}
  
template <class T> 
T * PBFT_R_Certificate<T>::cvalue_clear() {
  if (c == 0) {
    return 0;
  }

  T *ret = c->m;
  c->m = 0;
  for (int i=0; i < cuPBFT_R_size; i++) {
    if (vals[i].m == ret)
      vals[i].m = 0;
  }
  clear();

  return ret;
}


template <class T> 
bool PBFT_R_Certificate<T>::encode(FILE* o) {
  bool ret = bmap.encode(o);

  size_t sz = fwrite(&max_size, sizeof(int), 1, o);
  sz += fwrite(&cuPBFT_R_size, sizeof(int), 1, o);
  for (int i=0; i < cuPBFT_R_size; i++) {
    int vcount = vals[i].count;
    sz += fwrite(&vcount, sizeof(int), 1, o);
    if (vcount) {
      ret &= vals[i].m->encode(o);
    }
  }

  sz += fwrite(&complete, sizeof(int), 1, o);

  int cindex = (c != 0) ? c-vals : -1;
  sz += fwrite(&cindex, sizeof(int), 1, o);

  bool hmym = mym != 0;
  sz += fwrite(&hmym, sizeof(bool), 1, o);
  
  return ret & (sz == 5U+cuPBFT_R_size);
}


template <class T> 
bool PBFT_R_Certificate<T>::decode(FILE* in) {
  bool ret = bmap.decode(in);

  size_t sz = fread(&max_size, sizeof(int), 1, in);
  delete [] vals;

  vals = new PBFT_R_Message_val[max_size];
  
  sz += fread(&cuPBFT_R_size, sizeof(int), 1, in);
  if (cuPBFT_R_size < 0 || cuPBFT_R_size >= max_size)
    return false;

  for (int i=0; i < cuPBFT_R_size; i++) {
    sz += fread(&vals[i].count, sizeof(int), 1, in);
    if (vals[i].count < 0 || vals[i].count > PBFT_R_node->n())
      return false;

    if (vals[i].count) {
      vals[i].m = (T*)new PBFT_R_Message;
      ret &= vals[i].m->decode(in);
    }
  }

  sz += fread(&complete, sizeof(int), 1, in);
  correct = PBFT_R_node->f()+1;

  int cindex;
  sz += fread(&cindex, sizeof(int), 1, in);

  bool hmym;
  sz += fread(&hmym, sizeof(bool), 1, in);
  
  if (cindex == -1) {
    c = 0;
    mym = 0;
  } else {
    if (cindex < 0 || cindex > cuPBFT_R_size) 
      return false;
    c = vals+cindex;

    if (hmym)
      mym = c->m;
  }
  
  t_sent = zeroPBFT_R_Time();

  return ret & (sz == 5U+cuPBFT_R_size);
}
