#include <stdlib.h>
#include <strings.h>
#include "zl_Principal.h"
#include "zl_Node.h"

#include "crypt.h"
#include "rabin.h"

long long zl_Principal::umac_nonce = 0;

zl_Principal::zl_Principal(int i, int np, Addr a, char *p)
{
	id = i;
	addr = a;

	ctxs = (umac_ctx_t*)malloc(np*sizeof(umac_ctx_t*));

	umac_ctx_t *ctxs_t = ctxs;

	for (int index=0; index < np; index++)
	{
		int temp = i*1000 + index;

		unsigned k[zl_Key_size_u];
		for (unsigned int j = 0; j<zl_Key_size_u; j++)
		{
			k[j] = temp + j;
		}
		*ctxs_t = umac_new((char*)k);
		ctxs_t++;
	}
	if (p == 0) {
	    pkey = 0;
	    ssize = 0;
	} else {
	    bigint b(p,16);
	    ssize = (mpz_sizeinbase2(&b) >> 3) + 1 + sizeof(unsigned);
	    pkey = new rabin_pub(b);
	}
}

zl_Principal::~zl_Principal()
{
	free(ctxs);
}

bool zl_Principal::verify_mac(const char *src, unsigned src_len,
		const char *mac, const char *unonce)
{
	umac_ctx_t ctx = ctxs[zl_node->id()];
	// Do not accept MACs sent with uninitialized keys.
	if (ctx == 0)
	{
		fprintf(stderr,
				"zl_Principal::verify_mac: MACs sent with uninitialized keys\n");
		return false;
	}

	char tag[20];
	umac(ctx, (char *)src, src_len, tag, (char *)unonce);
	umac_reset(ctx);

	bool toRet = !memcmp(tag, mac, zl_UMAC_size);

	return toRet;
}

void zl_Principal::gen_mac(const char *src, unsigned src_len, char *dst,
		int dest_pid, const char *unonce)
{
	umac_ctx_t ctx = ctxs[dest_pid];
	umac(ctx, (char *)src, src_len, dst, (char *)unonce);
	umac_reset(ctx);
}

bool zl_Principal::verify_signature(const char *src, unsigned src_len,
				 const char *sig, bool allow_self) {
  // zl_Principal never verifies its own authenticator.
  if ((id == zl_node->id()) && !allow_self) return false;

  bigint bsig;
  unsigned int s_size;
  memcpy((char*)&s_size, sig, sizeof(int));
  sig += sizeof(int);
  if (s_size+(int)sizeof(int) > sig_size()) {
    return false;
  }

  mpz_set_raw(&bsig, sig, s_size);
  bool ret = pkey->verify(str(src, src_len), bsig);

  return ret;
}


unsigned zl_Principal::encrypt(const char *src, unsigned src_len, char *dst, 
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
