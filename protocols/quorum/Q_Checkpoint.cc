#include <stdlib.h>
#include <strings.h>
#include "th_assert.h"
#include "Q_Message_tags.h"
#include "Q_Checkpoint.h"
#include "Q_Node.h"
#include "Q_Principal.h"
#include "MD5.h"

Q_Checkpoint::Q_Checkpoint() :
	Q_Message(Q_Checkpoint_tag, sizeof(Q_Checkpoint_rep)+Q_node->sig_size())
{
	rep().id = Q_node->id();
}

Q_Checkpoint::~Q_Checkpoint()
{
}

void Q_Checkpoint::re_authenticate(Q_Principal *p, bool stable)
{
  if (rep().extra != 1 && stable) {
    rep().extra = 1;
    //Q_node->gen_signature(contents(), sizeof(Q_Checkpoint_rep), 
			//contents()+sizeof(Q_Checkpoint_rep));
  }
}

bool Q_Checkpoint::verify()
{
  // Q_Checkpoints must be sent by Q_replicas.
  if (!Q_node->is_replica(id())) return false;
  return true;

  // Check signature size.
  //if (size()-(int)sizeof(Q_Checkpoint_rep) < Q_node->sig_size(id())) 
    //return false;
//
  //return Q_node->i_to_p(id())->verify_signature(contents(), sizeof(Q_Checkpoint_rep),
					      //contents()+sizeof(Q_Checkpoint_rep));
}

bool Q_Checkpoint::convert(Q_Message *m1, Q_Checkpoint *&m2)
{

	if (!m1->has_tag(Q_Checkpoint_tag, sizeof(Q_Checkpoint_rep)))
	{
		return false;
	}

	m2 = new Q_Checkpoint();
	memcpy(m2->contents(), m1->contents(), m1->size());
	//free(m1->contents());
	// TODO : VIVIEN WE SHOULD NOT COMMENT THIS LINE (THIS CREATES A LIBC ERROR).
	delete m1;
	m2->trim();
	return true;
}
