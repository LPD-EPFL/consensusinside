#include <stdlib.h>
#include <strings.h>
#include "th_assert.h"
#include "C_Message_tags.h"
#include "C_Checkpoint.h"
#include "C_Node.h"
#include "C_Principal.h"
#include "MD5.h"

C_Checkpoint::C_Checkpoint() :
	C_Message(C_Checkpoint_tag, C_Max_message_size)
{
	rep().id = C_node->id();
	// rep().is_init = 0;
	set_size(sizeof(C_Checkpoint_rep));
}

C_Checkpoint::~C_Checkpoint()
{
}

void C_Checkpoint::re_authenticate(C_Principal *p, bool stable)
{
  if (rep().extra != 1 && stable) {
    rep().extra = 1;
    C_node->gen_signature(contents(), sizeof(C_Checkpoint_rep), 
			contents()+sizeof(C_Checkpoint_rep));
  }
}

bool C_Checkpoint::verify()
{
  // C_Checkpoints must be sent by C_replicas.
  if (!C_node->is_replica(id())) return false;

  // Check signature size.
  if (size()-(int)sizeof(C_Checkpoint_rep) < C_node->sig_size(id())) 
    return false;

  return C_node->i_to_p(id())->verify_signature(contents(), sizeof(C_Checkpoint_rep),
					      contents()+sizeof(C_Checkpoint_rep));
}

bool C_Checkpoint::convert(C_Message *m1, C_Checkpoint *&m2)
{

	if (!m1->has_tag(C_Checkpoint_tag, sizeof(C_Checkpoint_rep)))
	{
		return false;
	}

	m2 = new C_Checkpoint();
	memcpy(m2->contents(), m1->contents(), m1->size());
	//free(m1->contents());
	// TODO : VIVIEN WE SHOULD NOT COMMENT THIS LINE (THIS CREATES A LIBC ERROR).
	//delete m1;
	m2->trim();
	return true;
}
