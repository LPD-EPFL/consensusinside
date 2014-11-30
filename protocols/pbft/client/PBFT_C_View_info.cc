#include "PBFT_C_View_info.h"
#include "PBFT_C_View_change.h"
#include "PBFT_C_View_change_ack.h"
#include "PBFT_C_New_view.h"
#include "PBFT_C_Pre_prepare.h"
#include "PBFT_C_State.h"
#include "PBFT_C_Status.h"
#include "PBFT_C_Replica.h"
#include "PBFT_C_Big_req_table.h"
#include "th_assert.h"
#include "K_max.h"
#include "Log.t"
#include "Array.h"

PBFT_C_View_info::VCA_info::VCA_info() :
	v(0), vacks((PBFT_C_View_change_ack *) 0, PBFT_C_node->n())
{
}

void PBFT_C_View_info::VCA_info::clear()
{
	for (int i = 0; i < PBFT_C_node->n(); i++)
	{
		delete vacks[i];
		vacks[i] = 0;
	}
	v = 0;
}

PBFT_C_View_info::PBFT_C_View_info(int ident, View vi) :
	v(vi), id(ident), last_stable(0), oplog(max_out), last_views((View) 0,
			PBFT_C_node->n()), last_vcs((PBFT_C_View_change*) 0,
			PBFT_C_node->n()), my_vacks((PBFT_C_View_change_ack*) 0,
			PBFT_C_node->n()), vacks(PBFT_C_node->n()), last_nvs(
			PBFT_C_node->n())
{
	vacks._enlarge_by(PBFT_C_node->n());
	last_nvs._enlarge_by(PBFT_C_node->n());
}

PBFT_C_View_info::~PBFT_C_View_info()
{
	for (int i = 0; i < last_vcs.size(); i++)
	{
		delete last_vcs[i];
		delete my_vacks[i];
	}
}

void PBFT_C_View_info::add_complete(PBFT_C_Pre_prepare* pp)
{
	th_assert(pp->view() == v, "Invalid argument");

	OReq_info &ri = oplog.fetch(pp->seqno());
	th_assert(pp->view() >= 0 && pp->view() > ri.v, "Invalid argument");

	ri.clear();
	ri.v = pp->view();
	ri.lv = ri.v;
	ri.d = pp->digest();
	ri.m = pp;
}

void PBFT_C_View_info::add_incomplete(Seqno n, Digest const &d)
{
	OReq_info &ri = oplog.fetch(n);

	if (ri.d == d)
	{
		// PBFT_C_Message matches the one in the log
		if (ri.m != 0)
		{
			// Logged message was prepared
			ri.lv = v;
		}
		else
		{
			ri.v = v;
		}
	}
	else
	{
		// PBFT_C_Message is different from the one in log
		if (ri.m != 0)
		{
			delete ri.m;
			ri.m = 0;
		}
		else
		{
			ri.lv = ri.v;
		}

		// Remember last f()+2 digests.
		View minv = View_max;
		int mini = 0;
		for (int i = 0; i < PBFT_C_node->f() + 2; i++)
		{
			if (ri.ods[i].d == ri.d)
			{
				ri.ods[i].v = ri.lv;
				mini = -1;
				break;
			}

			if (ri.ods[i].v < minv)
			{
				mini = i;
				minv = ri.ods[i].v;
			}
		}

		if (mini >= 0)
		{
			ri.ods[mini].d = ri.d;
			ri.ods[mini].v = ri.lv;
		}

		ri.d = d;
		ri.v = v;
	}
}

void PBFT_C_View_info::send_proofs(Seqno n, View vi, int dest)
{
	if (oplog.within_range(n))
	{
		OReq_info &ri = oplog.fetch(n);
		PBFT_C_Principal* p = PBFT_C_node->i_to_p(dest);

		for (int i = 0; i < PBFT_C_node->f() + 2; i++)
		{
			if (ri.ods[i].v >= vi)
			{
				PBFT_C_Prepare prep(ri.ods[i].v, n, ri.ods[i].d, p);
				PBFT_C_node->send(&prep, dest);
			}
		}
	}
}

PBFT_C_Pre_prepare* PBFT_C_View_info::pre_prepare(Seqno n, Digest& d)
{
	if (oplog.within_range(n))
	{
		OReq_info& ri = oplog.fetch(n);
		if (ri.m && ri.d == d)
		{
			th_assert(ri.m->digest() == ri.d && ri.m->seqno() == n, "Invalid state");
			return ri.m;
		}
	}

	return 0;
}

PBFT_C_Pre_prepare* PBFT_C_View_info::pre_prepare(Seqno n, View v)
{
	if (oplog.within_range(n))
	{
		OReq_info& ri = oplog.fetch(n);
		if (ri.m && ri.v >= v)
		{
			th_assert(ri.m->seqno() == n, "Invalid state");
			return ri.m;
		}
	}

	return 0;
}

bool PBFT_C_View_info::prepare(Seqno n, Digest& d)
{
	// Effects: Returns true iff "this" logs that this replica sent a
	// prepare with digest "d" for sequence number "n".

	if (oplog.within_range(n))
	{
		OReq_info &ri = oplog.fetch(n);

		if (ri.d == d)
			return true;

		for (int i = 0; i < PBFT_C_node->f() + 2; i++)
		{
			if (ri.ods[i].d == d)
				return true;
		}
	}

	return false;
}

void PBFT_C_View_info::discard_old()
{
	// Discard view-changes, view-change acks, and new views with view
	// less than "v"
	for (int i = 0; i < PBFT_C_node->n(); i++)
	{
		if (last_vcs[i] && last_vcs[i]->view() < v)
		{
			delete last_vcs[i];
			last_vcs[i] = 0;
		}

		delete my_vacks[i];
		my_vacks[i] = 0;

		if (vacks[i].v < v)
		{
			vacks[i].clear();
			vacks[i].v = v;
		}

		if (last_nvs[i].view() < v)
		{
			last_nvs[i].clear();
		}
	}
}

void PBFT_C_View_info::view_change(View vi, Seqno last_executed,
		PBFT_C_State *state)
{
	v = vi;

	discard_old();

	// Create my view-change message for "v".
	PBFT_C_View_change *vc = new PBFT_C_View_change(v, last_stable, id);

	// Add checkpoint information to the message.
	for (Seqno i = last_stable; i <= last_executed; i += checkpoint_interval)
	{
		Digest dc;
		if (state->digest(i, dc))
			vc->add_checkpoint(i, dc);
	}

	PBFT_C_Big_req_table* brt = PBFT_C_replica->big_reqs();

	// Add request information to the message.
	for (Seqno i = last_stable + 1; i <= last_stable + max_out; i++)
	{
		OReq_info &ri = oplog.fetch(i);

		// Null requests are not added to message.
		if (ri.v >= 0)
		{
			vc->add_request(i, ri.v, ri.lv, ri.d, ri.m != 0);

			if (ri.m)
			{
				// Update replica's brt to prevent discarding of big requests
				// referenced by logged pre-prepares.
				for (int j = 0; j < ri.m->num_big_reqs(); j++)
					brt->add_pre_prepare(ri.m->big_req_digest(j), j, i, v);
			}
		}
	}

	// Discard stale big reqs.
	brt->view_change(v);

	vc->re_authenticate();
	vc_sent = currentPBFT_C_Time();
	PBFT_C_node->send(vc, PBFT_C_Node::All_replicas);

	// Record that this message was sent.
	last_vcs[id] = vc;
	last_views[id] = v;

	int primv = PBFT_C_node->primary(v);
	if (primv != id)
	{
		// If we are not the primary, send view-change acks for messages in
		// last_vcs with view v.
		for (int i = 0; i < PBFT_C_node->n(); i++)
		{
			PBFT_C_View_change *lvc = last_vcs[i];
			if (lvc && lvc->view() == v && i != id && i != primv)
			{
				PBFT_C_View_change_ack *vack = new PBFT_C_View_change_ack(v,
						id, i, lvc->digest());
				my_vacks[i] = vack;
				PBFT_C_node->send(vack, primv);
			}
		}
	}
	else
	{
		// If we are the primary create new view info for "v"
		PBFT_C_NV_info & n = last_nvs[id];
		th_assert(n.view() <= v, "Invalid state");

		// Create an empty new-view message and add it to "n". Information
		// will later be added to "n/nv".
		PBFT_C_New_view* nv = new PBFT_C_New_view(v);
		n.add(nv, this);

		// Move any view-change messages for view "v" to "n".
		for (int i = 0; i < PBFT_C_node->n(); i++)
		{
			PBFT_C_View_change* vc = last_vcs[i];
			if (vc && vc->view() == v && n.add(vc, true))
			{
				last_vcs[i] = 0;
			}
		}

		// Move any view-change acks for messages in "n" to "n"
		for (int i = 0; i < PBFT_C_node->n(); i++)
		{
			VCA_info& vaci = vacks[i];
			if (vaci.v == v)
			{
				for (int j = 0; j < PBFT_C_node->n(); j++)
				{
					if (vaci.vacks[j] && n.add(vaci.vacks[j]))
						vaci.vacks[j] = 0;
				}
			}
		}
	}
}

bool PBFT_C_View_info::add(PBFT_C_View_change* vc)
{
	int vci = vc->id();
	int vcv = vc->view();

	if (vcv < v)
	{
		delete vc;
		return false;
	}

	// Try to match vc with a new-view message.
	PBFT_C_NV_info & n = last_nvs[PBFT_C_node->primary(vcv)];
	bool stored = false;
	int primv = PBFT_C_node->primary(v);
	if (n.view() == vcv)
	{
		// There is a new-view message corresponding to "vc"
		bool verified = vc->verify();
		stored = n.add(vc, verified);

		if (stored && id == primv && vcv == v)
		{
			// Try to add any buffered view-change acks that match vc to "n"
			for (int i = 0; i < PBFT_C_node->n(); i++)
			{
				VCA_info& vaci = vacks[i];
				if (vaci.v == v && vaci.vacks[vci] && n.add(vaci.vacks[vci]))
				{
					vaci.vacks[vci] = 0;
				}
			}
		}

		if (verified && vcv > last_views[vci])
		{
			last_views[vci] = vcv;
		}
	}
	else
	{
		// There is no matching new-view.
		if (vcv > last_views[vci] && vc->verify())
		{
			delete last_vcs[vci];
			last_vcs[vci] = vc;
			last_views[vci] = vcv;
			stored = true;

			if (id != primv && vci != primv && vcv == v)
			{
				// Send view-change ack.
				PBFT_C_View_change_ack *vack = new PBFT_C_View_change_ack(v,
						id, vci, vc->digest());
				th_assert(my_vacks[vci] == 0, "Invalid state");

				my_vacks[vci] = vack;
				PBFT_C_node->send(vack, primv);
			}
		}
	}

	if (!stored)
		delete vc;

	return stored;
}

void PBFT_C_View_info::add(PBFT_C_New_view* nv)
{
	int nvi = nv->id();
	int nvv = nv->view();

	if (nvv >= v && nv->verify())
	{
		PBFT_C_NV_info & n = last_nvs[nvi];
		if (nv->view() > n.view())
		{
			bool stored = n.add(nv, this);
			if (stored)
			{
				// Move any view-change messages for view "nvv" to "n".
				for (int i = 0; i < last_vcs.size(); i++)
				{
					PBFT_C_View_change* vc = last_vcs[i];
					if (vc && vc->view() == nvv && n.add(vc, true))
					{
						last_vcs[i] = 0;
					}
				}
			}
			return;
		}
	}

	delete nv;
}

void PBFT_C_View_info::add(PBFT_C_View_change_ack* vca)
{
	int vci = vca->vc_id();
	int vcv = vca->view();

	if (vca->verify())
	{
		int primvcv = PBFT_C_node->primary(vcv);

		PBFT_C_NV_info & n = last_nvs[primvcv];
		if (n.view() == vcv && n.add(vca))
		{
			// There is a new-view message corresponding to "vca"
			return;
		}

		if (id == primvcv)
		{
			VCA_info &vcai = vacks[vca->id()];
			if (vcai.v <= vcv)
			{
				if (vcai.v < vcv)
				{
					vcai.clear();
				}

				delete vcai.vacks[vci];
				vcai.vacks[vci] = vca;
				vcai.v = v;
				return;
			}
		}
	}

	delete vca;
}

inline View PBFT_C_View_info::k_max(int k) const
{
	return K_max<View> (k, last_views.as_pointer(), PBFT_C_node->n(), View_max);
}

View PBFT_C_View_info::max_view() const
{
	View ret = k_max(PBFT_C_node->f() + 1);
	return ret;
}

View PBFT_C_View_info::max_maj_view() const
{
	View ret = k_max(PBFT_C_node->n_f());
	return ret;
}

void PBFT_C_View_info::set_received_vcs(PBFT_C_Status *m)
{
	th_assert(m->view() == v, "Invalid argument");

	PBFT_C_NV_info& nvi = last_nvs[PBFT_C_node->primary(v)];
	if (nvi.view() == v)
	{
		// There is a new-view message for the current view.
		nvi.set_received_vcs(m);
	}
	else
	{
		for (int i = 0; i < PBFT_C_node->n(); i++)
		{
			if (last_vcs[i] != 0 && last_vcs[i]->view() == v)
			{
				m->mark_vcs(i);
			}
		}
	}
}

void PBFT_C_View_info::set_missing_pps(PBFT_C_Status *m)
{
	th_assert(m->view() == view(), "Invalid argument");

	if (last_nvs[PBFT_C_node->primary(view())].new_view())
		last_nvs[PBFT_C_node->primary(view())].set_missing_pps(m);
}

PBFT_C_View_change *PBFT_C_View_info::my_view_change(PBFT_C_Time **t)
{
	PBFT_C_View_change *myvc;
	if (last_vcs[id] == 0)
	{
		myvc = last_nvs[PBFT_C_node->primary(v)].view_change(id);
	}
	else
	{
		myvc = last_vcs[id];
	}
	if (t && myvc)
		*t = &vc_sent;
	return myvc;
}

PBFT_C_New_view *PBFT_C_View_info::my_new_view(PBFT_C_Time **t)
{
	return last_nvs[id].new_view(t);
}

void PBFT_C_View_info::mark_stable(Seqno ls)
{
	last_stable = ls;
	oplog.truncate(last_stable + 1);

	last_nvs[PBFT_C_node->primary(v)].mark_stable(ls);
}

void PBFT_C_View_info::clear()
{
	oplog.clear(last_stable + 1);

	for (int i = 0; i < PBFT_C_node->n(); i++)
	{
		delete last_vcs[i];
		last_vcs[i] = 0;
		last_views[i] = v;
		vacks[i].clear();

		delete my_vacks[i];
		my_vacks[i] = 0;

		last_nvs[i].clear();
	}
	vc_sent = zeroPBFT_C_Time();
}

bool PBFT_C_View_info::shutdown(FILE* o)
{
	int wb = 0, ab = 0;
	bool ret = true;
	for (Seqno i = last_stable + 1; i <= last_stable + max_out; i++)
	{
		OReq_info& ori = oplog.fetch(i);

		wb += fwrite(&ori.lv, sizeof(View), 1, o);
		wb += fwrite(&ori.v, sizeof(View), 1, o);
		wb += fwrite(&ori.d, sizeof(Digest), 1, o);

		bool hasm = ori.m != 0;
		wb += fwrite(&hasm, sizeof(bool), 1, o);
		ab += 4;

		if (hasm)
			ret &= ori.m->encode(o);
	}

	bool is_complete = has_new_view(v);
	wb += fwrite(&is_complete, sizeof(bool), 1, o);
	ab++;

	return ret & (ab == wb);
}

bool PBFT_C_View_info::restart(FILE* in, View rv, Seqno ls, bool corrupt)
{
	v = rv;
	last_stable = ls;
	id = PBFT_C_node->id();

	for (int i = 0; i < PBFT_C_node->n(); i++)
		last_views[i] = v;

	vc_sent = zeroPBFT_C_Time();

	oplog.clear(last_stable + 1);

	bool ret = !corrupt;
	int rb = 0, ab = 0;
	if (!corrupt)
	{
		for (Seqno i = last_stable + 1; i <= last_stable + max_out; i++)
		{
			OReq_info& ori = oplog.fetch(i);

			rb += fread(&ori.lv, sizeof(View), 1, in);
			rb += fread(&ori.v, sizeof(View), 1, in);
			rb += fread(&ori.d, sizeof(Digest), 1, in);

			bool hasm;
			rb += fread(&hasm, sizeof(bool), 1, in);
			delete ori.m;
			if (hasm)
			{
				ori.m = (PBFT_C_Pre_prepare*) new PBFT_C_Message;
				ret &= ori.m->decode(in);
			}
			else
			{
				ori.m = 0;
			}

			// Check invariants
			if (hasm)
			{
				ret &= (ori.m->digest() == ori.d) & (ori.m->view() == ori.v)
						& (ori.lv >= ori.v);
			}
			else
			{
				ret &= (ori.lv < ori.v || ori.v == -1);
			}

			if (!ret)
			{
				ori = OReq_info();
				return false;
			}

			ab += 4;
		}
	}

	bool is_complete;
	rb += fread(&is_complete, sizeof(bool), 1, in);
	ab++;
	if (is_complete)
		last_nvs[PBFT_C_node->primary(v)].make_complete(v);

	return ret & (ab == rb);
}

bool PBFT_C_View_info::enforce_bound(Seqno b, Seqno ks, bool corrupt)
{
	if (corrupt || last_stable > b - max_out)
	{
		last_stable = ks;
		oplog.clear(ks + 1);
		return false;
	}

	for (Seqno i = b; i <= last_stable + max_out; i++)
	{
		OReq_info& ori = oplog.fetch(i);
		if (ori.v >= 0)
		{
			oplog.clear(ks + 1);
			return false;
		}
	}

	return true;
}

void PBFT_C_View_info::mark_stale()
{
	for (int i = 0; i < PBFT_C_node->n(); i++)
	{
		if (i != id)
		{
			delete last_vcs[i];
			last_vcs[i] = 0;
			if (last_views[i] >= v)
				last_views[i] = v;
		}

		delete my_vacks[i];
		my_vacks[i] = 0;

		PBFT_C_View_change *vc = last_nvs[i].mark_stale(id);
		if (vc && vc->view() == view())
		{
			last_vcs[id] = vc;
		}
		else
		{
			delete vc;
		}

		vacks[i].clear();
	}
}

