#ifndef _PBFT_C_Req_queue_h
#define _PBFT_C_Req_queue_h

#include "Array.h"
#include "types.h"

class PBFT_C_Request;

class PBFT_C_Req_queue {
  // 
  // Implements a bounded queue of requests.
  //
public:
  PBFT_C_Req_queue();
  // Effects: Creates an empty queue that can hold one request per principal.
  
  bool append(PBFT_C_Request *r); 
  // Effects: If there is space in the queue and there is no request
  // from "r->client_id()" with timestamp greater than or equal to
  // "r"'s in the queue then it: appends "r" to the queue, removes any
  // other request from "r->client_id()" from the queue and returns
  // true. Otherwise, returns false.

  PBFT_C_Request *remove();
  // Effects: If there is any element in the queue, removes the first
  // element in the queue and returns it. Otherwise, returns 0.

  bool remove(int cid, Request_id rid);
  // Effects: If there are any requests from client "cid" with
  // timestamp less than or equal to "rid" removes those requests from
  // the queue. Otherwise, does nothing. In either case, it returns
  // true iff the first request in the queue is removed.

  PBFT_C_Request* first() const;
  // Effects: If there is any element in the queue, returns a pointer to
  // the first request in the queue. Otherwise, returns 0.

  PBFT_C_Request* first_client(int cid) const;
  // Effects: If there is an element in the queue from client "cid"
  // returns a pointer to the first request in the queue from
  // "cid". Otherwise, returns 0.

  bool in_progress(int cid, Request_id rid, View v);
  // Effects: Returns true iff a pre-prepare was sent for a request from "cid" 
  // with timestamp greater than or equal to "rid" was sent in view "v". 
  // Otherwise, returns false and marks that "rid" is in progress in view "v".

  int size() const;
  // Effects: Returns the current size (number of elements) in queue.

  int num_bytes() const;
  // Effects: Return the number of bytes used by elements in the queue.

  void clear();
  // Effects: Removes all the requests from this.
  
private:
  struct PPBFT_C_Node {
    PBFT_C_Request* r;
    PPBFT_C_Node* next;
    PPBFT_C_Node* prev;
 
    Request_id out_rid;
    View out_v;

    PPBFT_C_Node() { r=0; clear(); }

    void clear();
  };

  // reqs has an entry for each principal indexed by principal id.
  Array<PPBFT_C_Node> reqs; 

  PPBFT_C_Node* head;
  PPBFT_C_Node* tail;

  int nelems;    // Number of elements in queue
  int nbytes;    // Number of bytes in queue

};


inline int PBFT_C_Req_queue::size() const { 
  return nelems; 
}

inline int PBFT_C_Req_queue::num_bytes() const { 
  return nbytes; 
}

inline PBFT_C_Request *PBFT_C_Req_queue::first() const {
  if (head) {
    th_assert(head->r != 0, "Invalid state");
    return head->r;
  }
  return 0;
}

inline PBFT_C_Request *PBFT_C_Req_queue::first_client(int cid) const {
  PPBFT_C_Node& cn = reqs[cid];
  return cn.r;
}

inline bool PBFT_C_Req_queue::in_progress(int cid, Request_id rid, View v) {
  PPBFT_C_Node& cn = reqs[cid];
  if (rid > cn.out_rid || v > cn.out_v) {
    cn.out_rid = rid;
    cn.out_v = v;
    return false;
  }
  return true;
}

#endif // _PBFT_C_Req_queue_h


