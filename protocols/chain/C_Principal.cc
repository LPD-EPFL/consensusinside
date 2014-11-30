#include <stdlib.h>
#include <strings.h>
#include "C_Principal.h"
#include "C_Node.h"
#include "C_Reply.h"

#include "crypt.h"
#include "rabin.h"

long long C_Principal::umac_nonce = 0;

C_Principal::C_Principal(int i, int np, Addr TCP_a, Addr TCP_a_for_clients, char *p)
{
	// Understand how to factorize this code...
	TCP_addr = TCP_a;
	TCP_addr_for_clients = TCP_a_for_clients;

	id = i;
	last_fetch = 0;

    if (id == C_node->id())
    {
	ctxs = (umac_ctx_t*)malloc(np*sizeof(umac_ctx_t*));

	umac_ctx_t *ctxs_t = ctxs;

	for (int index=0; index < np; index++)
	{
		int temp = i*1000 + index;

		unsigned k[C_Key_size_u];
		for (int j = 0; j<C_Key_size_u; j++)
		{
			k[j] = temp + j;
		}
		*ctxs_t = umac_new((char*)k);
		ctxs_t++;
	}
    } else {
        ctxs = (umac_ctx_t*)malloc(sizeof(umac_ctx_t*));
        umac_ctx_t *ctxs_t = ctxs;
        int temp = i*1000 + C_node->id();
        unsigned k[C_Key_size_u];
        for (int j = 0; j<C_Key_size_u; j++)
	{
	    k[j] = temp + j;
	}
	*ctxs_t = umac_new((char*)k);
    }

	if (p == 0) {
	    pkey = 0;
	    ssize = 0;
	} else {
	    bigint b(p,16);
	    ssize = (mpz_sizeinbase2(&b) >> 3) + 1 + sizeof(unsigned);
	    pkey = new rabin_pub(b);
	}
	num_principals = np;
}

C_Principal::~C_Principal()
{
    if (id == C_node->id()) {
	for (int index=0; index < num_principals; index++)
	    if (ctxs[index] != NULL)
		free(ctxs[index]);
    } else {
	free(ctxs[0]);
    }
	free(ctxs);
}

bool C_Principal::verify_mac(const char *src, unsigned src_len,
		const char *mac, const char *unonce)
{
        int node_index = C_node->id();
        if (id != C_node->id())
            node_index = 0;

        umac_ctx_t ctx = ctxs[node_index];
	// Do not accept MACs sent with uninitialized keys.
	if (ctx == 0)
	{
		fprintf(stderr,
				"Q_Principal::verify_mac: MACs sent with uninitialized keys\n");
		return false;
	}

	char tag[20];
	umac(ctx, (char *)src, src_len, tag, (char *)unonce);
	umac_reset(ctx);

	bool toRet = !memcmp(tag, mac, C_UMAC_size);

	return toRet;
}

void C_Principal::gen_mac(const char *src, unsigned src_len, char *dst,
		int dest_pid, const char *unonce)
{
	umac_ctx_t ctx = ctxs[dest_pid];
	umac(ctx, (char *)src, src_len, dst, (char *)unonce);
	umac_reset(ctx);
}

bool C_Principal::verify_signature(const char *src, unsigned src_len,
				 const char *sig, bool allow_self) {
  // C_Principal never verifies its own authenticator.
  if ((id == C_node->id()) && !allow_self) return false;

  bigint bsig;
  int s_size;
  memcpy((char*)&s_size, sig, sizeof(int));
  sig += sizeof(int);
  if (s_size+(int)sizeof(int) > sig_size()) {
    return false;
  }

  mpz_set_raw(&bsig, sig, s_size);  
  bool ret = pkey->verify(str(src, src_len), bsig);

  return ret;
}


unsigned C_Principal::encrypt(const char *src, unsigned src_len, char *dst, 
			    unsigned dst_len) {
  // This is rather inefficient if message is big but messages will
  // be small.
  bigint ctext = pkey->encrypt(str(src, src_len));
  unsigned size = mpz_rawsize(&ctext);
  if (dst_len < size+2*sizeof(unsigned))
    return 0;

  memcpy(dst, (char*)&src_len, sizeof(unsigned));
  dst += sizeof(unsigned);
  memcpy(dst, (char*)&size, sizeof(unsigned));
  dst += sizeof(unsigned);

  mpz_get_raw(dst, size, &ctext);
  return size+2*sizeof(unsigned);
}
