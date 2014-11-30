#include "zl_Certificate.h"
#include "zl_Node.h"

template<class T> zl_Certificate<T>::zl_Certificate(int comp) :
	bmap(Max_num_replicas)
{
	count = 0;
	complete = comp;
	value = 0;
}

template<class T> zl_Certificate<T>::~zl_Certificate()
{
}

template<class T> bool zl_Certificate<T>::add(T *m, zl_Node *n)
{
	const int id = m->id();
	// th_assert(id != n->id(), "verify should return false for messages from self");

	//fprintf(stderr,"In cert: Adding %d to replica %d\n",id,n->id());
	if (id == n->id())
	{
		return false;
	}
	//fprintf(stderr,"In cert: After Adding %d to replica %d\n",id,n->id());
	if (m->verify() && n->is_replica(id))
	{
		if (value == 0|| value->match(m))
		{
			if (!bmap.test(id))
			{
				bmap.set(id);
				count++;
			}

			if (value)
			{
				if (!value->full() && m->full())
				{
					delete value;
					value = m;
				} else
				{
					delete m;
				}
			} else
			{
				value = m;
			}
			return true;
		} else
			fprintf(stderr, "Values do not match\n");
	}
	fprintf(stderr, "Add: Message is not verified...\n");
	return false;
}

template<class T> bool zl_Certificate<T>::add_mine(T *m, zl_Node *n)
{
	const int id = m->id();
	th_assert(id == n->id(), "verify should return true for messages from self");

	if (n->is_replica(id))
	{
		if (value == 0|| value->match(m))
		{
			if (!bmap.test(id))
			{
				bmap.set(id);
				count++;
			}

			if (value)
			{
				if (!value->full() && m->full())
				{
					delete value;
					value = m;
				} else
				{
					delete m;
				}
				//	    fprintf(stderr, "Values already here\n");
			} else
			{
				value = m;
			}
			return true;
		}
#ifdef KOOSHA_TRACES_2
		fprintf(stderr,"zl_Certificate.t: Replica %d: ", n->id());
		fprintf(stderr,"value = %d , m = %d \n", value->id(), m->id());
		// TODO change this. If there was a switch in the middle of checkpointing, it should be handled.
		// I am just replacing the old checkpoint with the new one
		if (m->id()==0)
		{
			//value = m;
			//return true;
		}
#endif
		fprintf(stderr, "Values does not match\n");
	}
	fprintf(stderr, "Add mine: Message is not verified...\n");
	return false;
}

