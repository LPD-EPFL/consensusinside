#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include "Request_history.h"
#include "Array.h"
DSum* DSum::M = 0;

Req_history_log::Req_history_log() :
   rh(512)
{
   // The random modulus for computing sums in AdHASH.
   others = DSum::M;
   DSum::M = my_dsum = new DSum;
   mpn_set_str(
         DSum::M->sum,
         (unsigned char*)"d2a10a09a80bc599b4d60bbec06c05d5e9f9c369954940145b63a1e2",
         DSum::nbytes, 16);

   DSum::M = others;
   if (sizeof(Digest)%sizeof(mp_limb_t) != 0)
   {
      th_fail("Invalid assumption: sizeof(Digest)%sizeof(mp_limb_t)");
   }
}

Req_history_log::~Req_history_log()
{
    for (int i=0; i<rh.size(); i++)
	if (rh[i].req != NULL)
	    delete rh[i].req;
    rh.clear();
    delete my_dsum;
    DSum::M = others;
}

Digest Req_history_log::rh_digest()
{
   // MD5(i, last modification seqno, (data,size)
   Digest d_h;

   MD5_CTX ctx;
   MD5Init(&ctx);
   MD5Update(&ctx, (char*)&rh_d.sum, DSum::nbytes);
   MD5Final(d_h.udigest(), &ctx);
   return d_h;
}

Seqno Req_history_log::get_top_seqno() const
{
    if (rh.size() == 0)
	return 0;
    return rh.high().seqno();
}

bool Req_history_log::add_request(ARequest *req, Seqno s, Digest &d)
{
   others = DSum::M;
   DSum::M = my_dsum;
   rh_d.add(req->digest());
   d = rh_digest();
	//fprintf(stderr, "\n--- %u, %u", req->digest().udigest()[0], d.udigest()[0]);

   Rh_entry rhe(req, s, d);

   rh.append(rhe);

   DSum::M = others;
   return true;
}

bool Req_history_log::truncate_history(Seqno seq)
{
   // return false when rid is not in the list
   bool found = false;
   int pos = 0;
   for (int i=0; i<rh.size(); i++) {
       if (rh[i].seqno() == seq) {
	   found = true;
	   pos = i;
	   break;
       }
   }
   if (!found)
       return false;

   for (int i=0; i<=pos; i++) {
       if (rh[i].req != NULL) {
          delete rh[i].req;
       }
       rh[i].req = NULL;
   }

   for (int i=pos+1; i<rh.size(); i++)
       rh[i-pos-1] = rh[i];

   rh.enlarge_to(rh.size()-pos-1);
   return true;
}

int Req_history_log::size() const
{
   return rh.size();
}

Array<Rh_entry>& Req_history_log::array()
{
   return rh;
}

bool Req_history_log::should_checkpoint()
{
    if (size() && (size() % TRUNCATE_SIZE) == 0)
	return true;

    return false;
}

Rh_entry *Req_history_log::find(int cid, Request_id rid)
{
    for(int i=0; i<rh.size(); i++) {
	if (rh.slot(i).req
		&& rh.slot(i).req->client_id() == cid
		&& rh.slot(i).req->request_id() == rid) {
	    return &rh.slot(i);
	}
    }
    return NULL;
}
