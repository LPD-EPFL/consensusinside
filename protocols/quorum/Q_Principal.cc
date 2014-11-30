#include <stdlib.h>
#include <strings.h>
#include "Q_Principal.h"
#include "Q_Node.h"

//#include "crypt.h"
//#include "rabin.h"

//Added by Maysam Yabandeh
#include <task.h>

#define Q_Key_size_u 0

//Added by Maysam Yabandeh
Q_Principal::Q_Principal(int i, int np, Addr a, char * _ip, int _port, char *p)
{
	id = i;
	addr = a;

	//Added by Maysam Yabandeh
	strcpy(ip,_ip);
	port=_port;
	remotefd = -1;

	//ctxs = (umac_ctx_t*)malloc(np*sizeof(umac_ctx_t*));

	//umac_ctx_t *ctxs_t = ctxs;

	for (int index=0; index < np; index++)
	{
		int temp = i*1000 + index;

		unsigned k[Q_Key_size_u];
		for (unsigned int j = 0; j<Q_Key_size_u; j++)
		{
			k[j] = temp + j;
		}
		//*ctxs_t = umac_new((char*)k);
		//ctxs_t++;
	}
	if (p == 0) {
	    //pkey = 0;
	    //ssize = 0;
	} else {
	    //bigint b(p,16);
	    //ssize = (mpz_sizeinbase2(&b) >> 3) + 1 + sizeof(unsigned);
	    //pkey = new rabin_pub(b);
	}
}

Q_Principal::Q_Principal(int i, int np, Addr a, char *p)
{
	id = i;
	addr = a;

	//ctxs = (umac_ctx_t*)malloc(np*sizeof(umac_ctx_t*));

	//umac_ctx_t *ctxs_t = ctxs;

	for (int index=0; index < np; index++)
	{
		int temp = i*1000 + index;

		unsigned k[Q_Key_size_u];
		for (unsigned int j = 0; j<Q_Key_size_u; j++)
		{
			k[j] = temp + j;
		}
		//*ctxs_t = umac_new((char*)k);
		//ctxs_t++;
	}
	if (p == 0) {
	    //pkey = 0;
	    //ssize = 0;
	} else {
	    //bigint b(p,16);
	    //ssize = (mpz_sizeinbase2(&b) >> 3) + 1 + sizeof(unsigned);
	    //pkey = new rabin_pub(b);
	}
}

Q_Principal::~Q_Principal()
{
if (remotefd >= 0)
close(remotefd);
	//free(ctxs);
}



