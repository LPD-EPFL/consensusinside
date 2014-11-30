#include "PBFT_R_Node.h"
#include "PBFT_R_Prepared_cert.h"

#include "PBFT_R_Certificate.t"
template class PBFT_R_Certificate<PBFT_R_Prepare>;

PBFT_R_Prepared_cert::PBFT_R_Prepared_cert() :
	pc(PBFT_R_node->f()*2), primary(false)
{
}

PBFT_R_Prepared_cert::~PBFT_R_Prepared_cert()
{
	pi.clear();
}

bool PBFT_R_Prepared_cert::is_pp_correct()
{
	if (pi.pre_prepare())
	{
		PBFT_R_Certificate<PBFT_R_Prepare>::Val_iter viter(&pc);
		int vc;
		PBFT_R_Prepare* val;
		while (viter.get(val, vc))
		{
			if (vc >= PBFT_R_node->f() && pi.pre_prepare()->match(val))
			{
				return true;
			}
		}
	}
	return false;
}

bool PBFT_R_Prepared_cert::add(PBFT_R_Pre_prepare *m)
{
//	fprintf(stderr, "PBFT_R_Prepared_cert::add making progress 1\n");
	if (pi.pre_prepare() == 0)
	{
		//fprintf(stderr, "PBFT_R_Prepared_cert::add making progress 2\n");
		PBFT_R_Prepare* p = pc.mine();

		if (p == 0)
		{
//			fprintf(stderr, "PBFT_R_Prepared_cert::add making progress 3\n");
			if (m->verify())
			{
	//			fprintf(stderr, "PBFT_R_Prepared_cert::add making progress 3 bis\n");
				pi.add(m);
				return true;
			}
		//	fprintf(stderr, "PBFT_R_Prepared_cert::add making progress 4\n");

			if (m->verify(PBFT_R_Pre_prepare::NRC))
			{
//				fprintf(stderr, "PBFT_R_Prepared_cert::add making progress 5\n");

				// Check if there is some value that matches pp and has f
				// senders.
				PBFT_R_Certificate<PBFT_R_Prepare>::Val_iter viter(&pc);
				int vc;
				PBFT_R_Prepare* val;
				while (viter.get(val, vc))
				{
					if (vc >= PBFT_R_node->f() && m->match(val))
					{
						pi.add(m);
						return true;
					}
				}
			}
		} else
		{
//			fprintf(stderr, "PBFT_R_Prepared_cert::add making progress 10\n");

			// If we sent a prepare, we only accept a matching pre-prepare.
			if (m->match(p) && m->verify(PBFT_R_Pre_prepare::NRC))
			{
				pi.add(m);
				return true;
			}
		}
	}
	delete m;
	return false;
}

bool PBFT_R_Prepared_cert::encode(FILE* o)
{
	bool ret = pc.encode(o);
	ret &= pi.encode(o);
	int sz = fwrite(&primary, sizeof(bool), 1, o);
	return ret & (sz == 1);
}

bool PBFT_R_Prepared_cert::decode(FILE* i)
{
	th_assert(pi.pre_prepare() == 0, "Invalid state");

	bool ret = pc.decode(i);
	ret &= pi.decode(i);
	int sz = fread(&primary, sizeof(bool), 1, i);
	t_sent = zeroPBFT_R_Time();

	return ret & (sz == 1);
}

bool PBFT_R_Prepared_cert::is_empty() const
{
	return pi.pre_prepare() == 0 && pc.is_empty();
}
