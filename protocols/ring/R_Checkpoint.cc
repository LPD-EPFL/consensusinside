#include <stdlib.h>
#include <strings.h>
#include "th_assert.h"
#include "R_Message_tags.h"
#include "R_Checkpoint.h"
#include "R_Node.h"
#include "R_Principal.h"
#include "MD5.h"

R_Checkpoint::R_Checkpoint() :
	R_Message(R_Checkpoint_tag, R_Max_message_size)
{
	rep().id = R_node->id();
	// rep().is_init = 0;
	set_size(sizeof(R_Checkpoint_rep));
}

R_Checkpoint::~R_Checkpoint()
{
}

void R_Checkpoint::re_authenticate(R_Principal *p, bool stable)
{
  if (rep().extra != 1 && stable) {
    rep().extra = 1;
    R_node->gen_signature(contents(), sizeof(R_Checkpoint_rep), 
			contents()+sizeof(R_Checkpoint_rep));
  }
}

bool R_Checkpoint::verify()
{
  // R_Checkpoints must be sent by R_replicas.
  if (!R_node->is_replica(id())) return false;

  // Check signature size.
  if (size()-(int)sizeof(R_Checkpoint_rep) < R_node->sig_size(id())) 
    return false;

  return R_node->i_to_p(id())->verify_signature(contents(), sizeof(R_Checkpoint_rep),
					      contents()+sizeof(R_Checkpoint_rep));
}

bool R_Checkpoint::convert(R_Message *m1, R_Checkpoint *&m2)
{

	if (!m1->has_tag(R_Checkpoint_tag, sizeof(R_Checkpoint_rep)))
	{
		return false;
	}

	m2 = new R_Checkpoint();
	memcpy(m2->contents(), m1->contents(), m1->size());
	//free(m1->contents());
	// TODO : VIVIEN WE SHOULD NOT COMMENT THIS LINE (THIS CREATES A LIBC ERROR).
	//delete m1;
	m2->trim();
	return true;
}
