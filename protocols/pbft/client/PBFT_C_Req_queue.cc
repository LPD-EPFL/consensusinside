#include "PBFT_C_Request.h"
#include "PBFT_C_Req_queue.h"
#include "PBFT_C_Pre_prepare.h"
#include "PBFT_C_Node.h"

#include "Array.h"

PBFT_C_Req_queue::PBFT_C_Req_queue() : reqs(PPBFT_C_Node(), PBFT_C_node->np()), head(0),
  tail(0), nelems(0), nbytes(0)  {}


bool PBFT_C_Req_queue::append(PBFT_C_Request *r) {
  int cid = r->client_id();
  Request_id rid = r->request_id();
  PPBFT_C_Node& cn = reqs[cid];
  if (cn.r != 0) {
    // There is a request from client cid in reqs.
    if (rid > cn.r->request_id()) {
      remove(cid, rid);
    } else {
      return false;
    }
  }

  // Append request to queue.
  cn.r = r;
  nbytes += r->size();
  nelems++;

  if (head == 0) {
    head = tail = &cn;
    cn.prev = cn.next = 0;
  } else {
    tail->next = &cn;
    cn.prev = tail;
    cn.next = 0;
    tail = &cn;
  }

  return true;
}


PBFT_C_Request *PBFT_C_Req_queue::remove() {
  if (head == 0) return 0;

  PBFT_C_Request *ret = head->r;
  th_assert(ret != 0, "Invalid state");

  head->r = 0;
  head = head->next;
  if (head != 0)
    head->prev = 0;
  else
    tail = 0;

  nelems--;
  nbytes -= ret->size();

  return ret;
}


bool PBFT_C_Req_queue::remove(int cid, Request_id rid) {
  bool ret = false;
  PPBFT_C_Node& cn = reqs[cid];
  if (cn.r && cn.r->request_id() <= rid) {
    nelems--;
    nbytes -= cn.r->size();

    delete cn.r;
    cn.r = 0;

    if (cn.prev == 0) {
      th_assert(head == &cn, "Invalid state");
      head = cn.next;
      ret = true;
    } else {
      cn.prev->next = cn.next;
    }

    if (cn.next == 0) {
      th_assert(tail == &cn, "Invalid state");
      tail = cn.prev;
    } else {
      cn.next->prev = cn.prev;
    }
  }
  return ret;
}


void PBFT_C_Req_queue::clear() {
  for (int i=0; i < PBFT_C_node->np(); i++)
    reqs[i].clear();

  head = tail = 0;
  nelems = nbytes = 0;
}


void PBFT_C_Req_queue::PPBFT_C_Node::clear() {
  delete r;
  r = 0;
  next = prev = 0;
  out_rid = 0;
  out_v = -1;
}
