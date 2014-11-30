#include "PBFT_C_Request.h"
#include "PBFT_C_Pre_prepare.h"
#include "PBFT_C_Big_req_table.h"
#include "PBFT_C_Replica.h"

#include "Array.h"
#include "bhash.t"
#include "buckets.t"


struct Waiting_pp {
  Seqno n;
  int i;
};

class BPBFT_C_entry {
public:
  inline BPBFT_C_entry() : r(0), maxn(-1), maxv(-1) {}
  inline ~BPBFT_C_entry() { delete r; }

  Digest rd;            // PBFT_C_Request's digest
  PBFT_C_Request *r;           // PBFT_C_Request or 0 is request not received
  Array<Waiting_pp> waiting; // if r=0, Seqnos of pre-prepares waiting for request
  Seqno maxn;           // Maximum seqno of pre-prepare referencing request
  View maxv;            // Maximum view in which this entry was marked useful
};


PBFT_C_Big_req_table::PBFT_C_Big_req_table() : breqs(max_out), unmatched((PBFT_C_Request*)0, PBFT_C_node->np()) {
  max_entries = max_out*PBFT_C_Pre_prepare::big_req_max;
}


PBFT_C_Big_req_table::~PBFT_C_Big_req_table() {
  MapGenerator<Digest,BPBFT_C_entry*> g(breqs);
  Digest d;
  BPBFT_C_entry* bre;
  while (g.get(d, bre)) {
    delete bre;
  }
  breqs.clear();
}

inline void PBFT_C_Big_req_table::remove_unmatched(BPBFT_C_entry* bre) {
  if (bre->maxn < 0) {
    th_assert(bre->r != 0, "Invalid state");
    unmatched[bre->r->client_id()] = 0;
  }
}

bool PBFT_C_Big_req_table::add_pre_prepare(Digest& rd, int i, Seqno n, View v) {
  BPBFT_C_entry* bre;
  if (breqs.find(rd, bre)) {
    remove_unmatched(bre);

    if (n > bre->maxn)
      bre->maxn = n;

    if (v > bre->maxv)
      bre->maxv = v;

    if (bre->r) {
      return true;
    } else {
      Waiting_pp wp;
      wp.i = i;
      wp.n = n;
      bre->waiting.append(wp);
    }
  } else {
    // No entry in breqs for rd
    bre = new BPBFT_C_entry;
    bre->rd = rd;
    Waiting_pp wp;
    wp.i = i;
    wp.n = n;
    bre->waiting.append(wp);
    bre->maxn = n;
    bre->maxv = v;
    breqs.add(rd, bre);
  }

  return false;
}


void PBFT_C_Big_req_table::add_pre_prepare(PBFT_C_Request* r, Seqno n, View v) {
  BPBFT_C_entry* bre;
  Digest rd = r->digest();
  if (breqs.find(rd, bre)) {
    remove_unmatched(bre);

    if (n > bre->maxn)
      bre->maxn = n;

    if (v > bre->maxv)
      bre->maxv = v;

    if (bre->r == 0) {
      bre->r = r;
    } else {
      delete r;
    }
  } else {
    // No entry in breqs for rd
    bre = new BPBFT_C_entry;
    bre->rd = rd;
    bre->r = r;
    bre->maxn = n;
    bre->maxv = v;
    breqs.add(rd, bre);
  }
}


bool PBFT_C_Big_req_table::check_pcerts(BPBFT_C_entry* bre) {
  th_assert(PBFT_C_replica->has_new_view(), "Invalid state");

  for (int i=0; i < bre->waiting.size(); i++) {
    Waiting_pp wp = bre->waiting[i];
    if (PBFT_C_replica->plog.within_range(wp.n)) {
      PBFT_C_Prepared_cert& pc = PBFT_C_replica->plog.fetch(wp.n);
      if (pc.is_pp_correct())
	return true;
    }
  }
  return false;
}


bool PBFT_C_Big_req_table::add_request(PBFT_C_Request* r, bool verified) {
  th_assert(r->size() > PBFT_C_Request::big_req_thresh && !r->is_read_only(),
	    "Invalid Argument");
  BPBFT_C_entry* bre;
  Digest rd = r->digest();
  if (breqs.find(rd, bre)) {
    if (bre->r == 0
	&& (verified || !PBFT_C_replica->has_new_view() || check_pcerts(bre))) {
      bre->r = r;
      while (bre->waiting.size()) {
	Waiting_pp wp = bre->waiting.high();
	Seqno n = wp.n;
	int i = wp.i;

	if (PBFT_C_replica->has_new_view()) {
	  // Missing pre-prepare is in replica's plog.
	  if (PBFT_C_replica->plog.within_range(n)) {
	    PBFT_C_Prepared_cert& pc = PBFT_C_replica->plog.fetch(n);
	    pc.add(bre->rd, i);
	    PBFT_C_replica->send_prepare(pc);
	    if (pc.is_complete())
	    	PBFT_C_replica->send_commit(n);
	  }
	} else {
	  // Missing pre-prepare is in replica's view-info
		PBFT_C_replica->vi.add_missing(bre->rd, n, i);
	}

	bre->waiting.remove();
      }
      return true;
    }
  } else if (verified) {
    // Buffer the request with largest timestamp from client.
    int cid = r->client_id();
    Request_id rid = r->request_id();
    PBFT_C_Request* old_req = unmatched[cid];

    if (old_req == 0 || old_req->request_id() < rid) {
      bre = new BPBFT_C_entry;
      bre->rd = rd;
      bre->r = r;
      breqs.add(rd, bre);

      if (old_req) {
	breqs.remove_fast(old_req->digest());
	delete old_req;
      }

      unmatched[cid] = r;
      return true;
    }
  }
  return false;
}


PBFT_C_Request* PBFT_C_Big_req_table::lookup(Digest& rd) {
  BPBFT_C_entry* bre;
  if (breqs.find(rd, bre)) {
    return bre->r;
  }
  return 0;
}


void PBFT_C_Big_req_table::mark_stable(Seqno ls) {
  if (breqs.size() > 0) {
    MapGenerator<Digest,BPBFT_C_entry*> g(breqs);
    Digest d;
    BPBFT_C_entry* bre;
    while (g.get(d, bre)) {
      if (bre->maxn <= ls && bre->maxv >= 0) {
	remove_unmatched(bre);
	delete bre;
	g.remove();
      }
    }
  }
}


void PBFT_C_Big_req_table::view_change(View v) {
  if (breqs.size() > 0) {
    MapGenerator<Digest,BPBFT_C_entry*> g(breqs);
    Digest d;
    BPBFT_C_entry* bre;
    while (g.get(d, bre)) {
      if (bre->maxv < v) {
	remove_unmatched(bre);
	delete bre;
	g.remove();
      }
    }
  }
}

