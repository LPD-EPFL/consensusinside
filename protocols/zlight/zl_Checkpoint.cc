#include <stdlib.h>
#include <strings.h>
#include "th_assert.h"
#include "zl_Message_tags.h"
#include "zl_Checkpoint.h"
#include "zl_Node.h"
#include "zl_Principal.h"
#include "MD5.h"

zl_Checkpoint::zl_Checkpoint() :
	zl_Message(zl_Checkpoint_tag, sizeof(zl_Checkpoint_rep)+zl_node->sig_size())
{
	rep().id = zl_node->id();
}

zl_Checkpoint::~zl_Checkpoint()
{
}

void zl_Checkpoint::re_authenticate(zl_Principal *p, bool stable)
{
  if (rep().extra != 1 && stable) {
    rep().extra = 1;
    zl_node->gen_signature(contents(), sizeof(zl_Checkpoint_rep), 
			contents()+sizeof(zl_Checkpoint_rep));
  }
}

bool zl_Checkpoint::verify()
{
  // zl_Checkpoints must be sent by zl_replicas.
  if (!zl_node->is_replica(id())) return false;

  // Check signature size.
  if (size()-(int)sizeof(zl_Checkpoint_rep) < zl_node->sig_size(id())) 
    return false;

  return zl_node->i_to_p(id())->verify_signature(contents(), sizeof(zl_Checkpoint_rep),
					      contents()+sizeof(zl_Checkpoint_rep));
}

bool zl_Checkpoint::convert(zl_Message *m1, zl_Checkpoint *&m2)
{

	if (!m1->has_tag(zl_Checkpoint_tag, sizeof(zl_Checkpoint_rep)))
	{
		return false;
	}

	m2 = new zl_Checkpoint();
	memcpy(m2->contents(), m1->contents(), m1->size());
	//free(m1->contents());
	// TODO : VIVIEN WE SHOULD NOT COMMENT THIS LINE (THIS CREATES A LIBC ERROR).
	//delete m1;
	m2->trim();
	return true;
}
