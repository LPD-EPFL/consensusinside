#include <string.h>
#include <stdio.h>
#include <limits.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>

#include "th_assert.h"
#include "PBFT_C_Message_tags.h"
#include "PBFT_C_ITimer.h"
#include "PBFT_C_Request.h"
#include "PBFT_C_Pre_prepare.h"
#include "PBFT_C_Prepare.h"
#include "PBFT_C_Commit.h"
#include "PBFT_C_Checkpoint.h"
#include "PBFT_C_New_key.h"
#include "PBFT_C_Status.h"
#include "PBFT_C_Fetch.h"
#include "PBFT_C_Data.h"
#include "PBFT_C_Meta_data.h"
#include "PBFT_C_Meta_data_d.h"
#include "PBFT_C_View_change.h"
#include "PBFT_C_View_change_ack.h"
#include "PBFT_C_New_view.h"
#include "PBFT_C_Principal.h"
#include "PBFT_C_Prepared_cert.h"
#include "PBFT_C_Reply.h"
#include "PBFT_C_Query_stable.h"
#include "PBFT_C_Reply_stable.h"
#include "K_max.h"

#include "PBFT_C_Replica.h"

#include "PBFT_C_Statistics.h"

#include "PBFT_C_State_defs.h"

// Global replica object.
PBFT_C_Replica *PBFT_C_replica;

// Force template instantiation
#include "PBFT_C_Certificate.t"
template class PBFT_C_Certificate<PBFT_C_Commit> ;
template class PBFT_C_Certificate<PBFT_C_Checkpoint> ;
template class PBFT_C_Certificate<PBFT_C_Reply> ;

#include "Log.t"
template class Log<PBFT_C_Prepared_cert> ;
template class Log<PBFT_C_Certificate<PBFT_C_Commit> > ;
template class Log<PBFT_C_Certificate<PBFT_C_Checkpoint> > ;

#include "Set.h"
template class Set<PBFT_C_Checkpoint> ;

template<class T>
void PBFT_C_Replica::retransmit(T *m, PBFT_C_Time &cur, PBFT_C_Time *tsent, PBFT_C_Principal *p)
{
	//XXXXXXXXXXXthere most be a bug in the way tsent is managed. Figure out
	// where and reinsert this protection against denial of service attacks.
	//if (diffPBFT_C_Time(cur, *tsent) > 1000) {
	if (1)
	{
		//if (p->is_stale(tsent)) {
		if (1)
		{
			// Authentication for this principal is stale in message -
			// re-authenticate.
			m->re_authenticate(p);
		}

		//    printf("RET: %s to %d \n", m->stag(), p->pid());

		// Retransmit message
		send(m, p->pid());

		*tsent = cur;
	}
}

#ifndef NO_STATE_TRANSLATION

PBFT_C_Replica::PBFT_C_Replica(FILE *config_file, FILE *config_priv, int num_objs,
		int (*get)(int, char **),
		void (*put)(int, int *, int *, char **),
		void (*shutdown_proc)(FILE *o),
		void (*restart_proc)(FILE *i),
		short port):
PBFT_C_Node(config_file, config_priv, port), rqueue(), ro_rqueue(),
plog(max_out), clog(max_out), elog(max_out*2,0), sset(n()),
replies(num_principals),
state(this, num_objs, get, put, shutdown_proc, restart_proc),
vi(node_id, 0),
n_mem_blocks(num_objs)
{

#else

PBFT_C_Replica::PBFT_C_Replica(FILE *config_file, FILE *config_priv, char *mem, int nbytes) :
	PBFT_C_Node(config_file, config_priv), rqueue(), ro_rqueue(), plog(max_out), clog(
			max_out), elog(max_out * 2, 0), sset(n()), replies(mem, nbytes,
			num_principals), state(this, mem, nbytes), vi(node_id, 0)
{

#endif

	// Fail if node is not a replica.
	if (!is_replica(id()))
		th_fail("PBFT_C_Node is not a replica");

	seqno = 0;
	last_stable = 0;
	low_bound = 0;

	last_prepared = 0;
	last_executed = 0;
	last_tentative_execute = 0;

	last_status = 0;

	limbo = false;
	has_nv_state = true;

	nbreqs = 0;
	nbrounds = 0;

	// Read view change, status, and recovery timeouts from replica's portion
	// of "config_file"
	int vt, st, rt;
	fscanf(config_file, "%d\n", &vt);
	fscanf(config_file, "%d\n", &st);
	fscanf(config_file, "%d\n", &rt);

	// Create timers and randomize times to avoid collisions.
	srand48(getpid());

	vtimer = new PBFT_C_ITimer(vt + lrand48() % 100, vtimer_handler);
	stimer = new PBFT_C_ITimer(st + lrand48() % 100, stimer_handler);

	// Skew recoveries. It is important for nodes to recover in the reverse order
	// of their node ids to avoid a view-change every recovery which would degrade
	// performance.
	rtimer = new PBFT_C_ITimer(rt, rec_timer_handler);
	rec_ready = false;
	rtimer->start();

	ntimer = new PBFT_C_ITimer(30000 / max_out, ntimer_handler);

	recovering = false;
	qs = 0;
	rr = 0;
	rr_views = new View[num_replicas];
	recovery_point = Seqno_max;
	max_rec_n = 0;

	exec_command = 0;
	non_det_choices = 0;

	join_mcast_group();

	// Disable loopback
	u_char l = 0;
	int error = setsockopt(sock, IPPROTO_IP, IP_MULTICAST_LOOP, &l, sizeof(l));
	if (error < 0)
	{
		perror("unable to disable loopback");
		exit(1);
	}

#ifdef LARGE_SND_BUFF
	int snd_buf_size = 262144;
	error = setsockopt(sock, SOL_SOCKET, SO_SNDBUF,
			(char*)&snd_buf_size, sizeof(snd_buf_size));
	if (error < 0)
	{
		perror("unable to increase send buffer size");
		exit(1);
	}
#endif

#ifdef LARGE_RCV_BUFF
	int rcv_buf_size = 131072;
	error = setsockopt(sock, SOL_SOCKET, SO_RCVBUF,
			(char*)&rcv_buf_size, sizeof(rcv_buf_size));
	if (error < 0)
	{
		perror("unable to increase send buffer size");
		exit(1);
	}
#endif
}

void PBFT_C_Replica::register_exec(int(*e)(Byz_req *, Byz_rep *, Byz_buffer *, int,
		bool))
{
	exec_command = e;
}

#ifndef NO_STATE_TRANSLATION
void PBFT_C_Replica::register_nondet_choices(void (*n)(Seqno, Byz_buffer *),
		int max_len,
		bool (*check)(Byz_buffer *))
{
	check_non_det = check;
#else
void PBFT_C_Replica::register_nondet_choices(void(*n)(Seqno, Byz_buffer *),
		int max_len)
{
#endif
	non_det_choices = n;
	max_nondet_choice_len = max_len;
}

void PBFT_C_Replica::compute_non_det(Seqno s, char *b, int *b_len)
{
	if (non_det_choices == 0)
	{
		*b_len = 0;
		return;
	}
	Byz_buffer buf;
	buf.contents = b;
	buf.size = *b_len;
	non_det_choices(s, &buf);
	*b_len = buf.size;
}

PBFT_C_Replica::~PBFT_C_Replica()
{
}

void PBFT_C_Replica::recv()
{
	// Compute session keys and send initial new-key message.
	PBFT_C_Node::send_new_key();

	// Compute digest of initial state and first checkpoint.
	state.compute_full_digest();

	// Start status and authentication freshness timers
	stimer->start();
	atimer->start();
	if (id() == primary())
		ntimer->start();

	// Allow recoveries
	rec_ready = true;

	fprintf(stderr, "PBFT_C_Replica ready\n");

	while (1)
	{
		if (state.in_check_state())
		{
			state.check_state();
		}

		PBFT_C_Message* m = PBFT_C_Node::recv();

//		fprintf(stderr, "recv() returned\n");

		if (qs)
		{
			if (m->tag() != PBFT_C_New_key_tag && m->tag() != PBFT_C_Query_stable_tag
					&& m->tag() != PBFT_C_Reply_stable_tag && m->tag() != PBFT_C_Status_tag)
			{
				// While estimating replica only handles certain messages.
				delete m;
				continue;
			}
		}

#if 1
		if (state.in_check_state())
		{
			if (m->tag() < 6 && m->tag() != PBFT_C_Reply_tag)
			{
				delete m;
				continue;
			}
		}
#endif

		// TODO: This should probably be a jump table.
		switch (m->tag())
		{
		case PBFT_C_Request_tag:
			gen_handle<PBFT_C_Request> (m);
			break;

		case PBFT_C_Pre_prepare_tag:
			gen_handle<PBFT_C_Pre_prepare> (m);
			break;

		case PBFT_C_Prepare_tag:
			gen_handle<PBFT_C_Prepare> (m);
			break;

		case PBFT_C_Commit_tag:
			gen_handle<PBFT_C_Commit> (m);
			break;

		case PBFT_C_Checkpoint_tag:
			gen_handle<PBFT_C_Checkpoint> (m);
			break;

		case PBFT_C_New_key_tag:
			gen_handle<PBFT_C_New_key> (m);
			break;

		case PBFT_C_View_change_ack_tag:
			gen_handle<PBFT_C_View_change_ack> (m);
			break;

		case PBFT_C_Status_tag:
			gen_handle<PBFT_C_Status> (m);
			break;

		case PBFT_C_Fetch_tag:
			gen_handle<PBFT_C_Fetch> (m);
			break;

		case PBFT_C_Reply_tag:
			gen_handle<PBFT_C_Reply> (m);
			break;

		case PBFT_C_Query_stable_tag:
			gen_handle<PBFT_C_Query_stable> (m);
			break;

		case PBFT_C_Reply_stable_tag:
			gen_handle<PBFT_C_Reply_stable> (m);
			break;

		case PBFT_C_Meta_data_tag:
			gen_handle<PBFT_C_Meta_data> (m);
			break;

		case PBFT_C_Meta_data_d_tag:
			gen_handle<PBFT_C_Meta_data_d> (m);
			break;

		case PBFT_C_Data_tag:
			gen_handle<PBFT_C_Data> (m);
			break;

		case PBFT_C_View_change_tag:
			gen_handle<PBFT_C_View_change> (m);
			break;

		case PBFT_C_New_view_tag:
			gen_handle<PBFT_C_New_view> (m);
			break;

		default:
			// Unknown message type.
			delete m;
		}
	}
}

void PBFT_C_Replica::handle(PBFT_C_Request *m)
{
//	fprintf(stderr, "PBFT_C_Replica::handle(PBFT_C_Request *m) called\n");
	int cid = m->client_id();
	bool ro = m->is_read_only();
	Request_id rid = m->request_id();

	if (has_new_view() && m->verify())
	{
		// PBFT_C_Replica's requests must be signed and cannot be read-only.
		if (!is_replica(cid) || (m->is_signed() & !ro))
		{
			if (ro)
			{
				// Read-only requests.
				if (execute_read_only(m) || !ro_rqueue.append(m))
					delete m;

				return;
			}

			Request_id last_rid = replies.req_id(cid);
			if (last_rid < rid)
			{
				// PBFT_C_Request has not been executed.
				if (id() == primary())
				{
					if (!rqueue.in_progress(cid, rid, v) && rqueue.append(m))
					{
						//	    fprintf(stderr, "RID %qd. ", rid);
						send_pre_prepare();
						return;
					}
				}
				else
				{
					if (m->size() > PBFT_C_Request::big_req_thresh && brt.add_request(
							m))
						return;

					if (rqueue.append(m))
					{
						if (!limbo)
						{
							send(m, primary());
							vtimer->start();
						}
						return;
					}
				}
			}
			else if (last_rid == rid)
			{
				// Retransmit reply.
				replies.send_reply(cid, view(), id());

				if (id() != primary() && !replies.is_committed(cid)
						&& rqueue.append(m))
				{
					vtimer->start();
					return;
				}
			}
		}
	}
	else
	{
		if (m->size() > PBFT_C_Request::big_req_thresh && !ro && brt.add_request(m,
				false))
			return;
	}

	delete m;
}

void PBFT_C_Replica::send_pre_prepare()
{
	th_assert(primary() == node_id, "Non-primary called send_pre_prepare");

	// If rqueue is empty there are no requests for which to send
	// pre_prepare and a pre-prepare cannot be sent if the seqno excedes
	// the maximum window or the replica does not have the new view.
	if (rqueue.size() > 0 && seqno + 1 <= last_executed + congestion_window
			&& seqno + 1 <= max_out + last_stable && has_new_view())
	{

		//  printf("requeu.size = %d\n", rqueue.size());
		//  if (seqno % checkpoint_interval == 0)
		//printf("SND: PRE-PREPARE seqno=%qd last_stable=%qd\n", seqno+1, last_stable);

		nbreqs += rqueue.size();
		nbrounds++;

		/*
		 if (nbreqs >= 10000) {
		 printf("Avg batch sz: %f\n", (float)nbreqs/(float)nbrounds);
		 nbreqs = nbrounds = 0;
		 }
		 */

		// Create new pre_prepare message for set of requests
		// in rqueue, log message and multicast the pre_prepare.
		seqno++;
		//    fprintf(stderr, "Sending PP seqno %qd\n", seqno);
		PBFT_C_Pre_prepare *pp = new PBFT_C_Pre_prepare(view(), seqno, rqueue);

		// TODO: should make code match my proof with request removed
		// only when executed rather than removing them from rqueue when the
		// pre-prepare is constructed.

		send(pp, All_replicas);
		plog.fetch(seqno).add_mine(pp);
	}
}

template<class T>
bool PBFT_C_Replica::in_w(T *m)
{
	const Seqno offset = m->seqno() - last_stable;

	if (offset > 0 && offset <= max_out)
		return true;

	if (offset > max_out && m->verify())
	{
		// Send status message to obtain missing messages. This works as a
		// negative ack.
		send_status();
	}

	return false;
}

template<class T>
bool PBFT_C_Replica::in_wv(T *m)
{
	const Seqno offset = m->seqno() - last_stable;

	if (offset > 0 && offset <= max_out && m->view() == view())
		return true;

	if ((m->view() > view() || offset > max_out) && m->verify())
	{
		// Send status message to obtain missing messages. This works as a
		// negative ack.
		send_status();
	}

	return false;
}

void PBFT_C_Replica::handle(PBFT_C_Pre_prepare *m)
{
//	fprintf(stderr, "PBFT_C_Replica::handle(PBFT_C_Pre_prepare *m) called\n");
	const Seqno ms = m->seqno();

	Byz_buffer b;

	b.contents = m->choices(b.size);

#ifndef NO_STATE_TRANSLATION
	if (in_wv(m) && ms > low_bound && has_new_view() && check_non_det(&b))
	{
#else
	if (in_wv(m) && ms > low_bound && has_new_view())
	{
#endif
		PBFT_C_Prepared_cert& pc = plog.fetch(ms);

		// Only accept message if we never accepted another pre-prepare
		// for the same view and sequence number and the message is valid.
		if (pc.add(m))
		{
			send_prepare(pc);
			if (pc.is_complete())
				send_commit(ms);
		}
		return;
	}

	if (!has_new_view())
	{
		// This may be an old pre-prepare that replica needs to complete
		// a view-change.
		vi.add_missing(m);
		return;
	}

	delete m;
}

void PBFT_C_Replica::send_prepare(PBFT_C_Prepared_cert& pc)
{
	if (pc.my_prepare() == 0 && pc.is_pp_complete())
	{
		// Send prepare to all replicas and log it.
		PBFT_C_Pre_prepare* pp = pc.pre_prepare();
		PBFT_C_Prepare *p = new PBFT_C_Prepare(v, pp->seqno(), pp->digest());
		send(p, All_replicas);
		pc.add_mine(p);
	}
}

void PBFT_C_Replica::send_commit(Seqno s)
{
	// Executing request before sending commit improves performance
	// for null requests. May not be true in general.
	if (s == last_executed + 1)
		execute_prepared();

	PBFT_C_Commit* c = new PBFT_C_Commit(view(), s);
	send(c, All_replicas);

	if (s > last_prepared)
		last_prepared = s;

	PBFT_C_Certificate<PBFT_C_Commit>& cs = clog.fetch(s);
	if (cs.add_mine(c) && cs.is_complete())
	{
		execute_committed();
	}
}

void PBFT_C_Replica::handle(PBFT_C_Prepare *m)
{
	//fprintf(stderr, "PBFT_C_Replica::handle(PBFT_C_Prepare *m) called\n");
	const Seqno ms = m->seqno();

	// Only accept prepare messages that are not sent by the primary for
	// current view.
	if (in_wv(m) && ms > low_bound && primary() != m->id() && has_new_view())
	{
		PBFT_C_Prepared_cert& ps = plog.fetch(ms);
		if (ps.add(m) && ps.is_complete())
			send_commit(ms);
		return;
	}

	if (m->is_proof() && !has_new_view())
	{
		// This may be an prepare sent to prove the authenticity of a
		// request to complete a view-change.
		vi.add_missing(m);
		return;
	}

	delete m;
	return;
}

void PBFT_C_Replica::handle(PBFT_C_Commit *m)
{
	const Seqno ms = m->seqno();

	// Only accept messages with the current view.  TODO: change to
	// accept commits from older views as in proof.
	if (in_wv(m) && ms > low_bound)
	{
		PBFT_C_Certificate<PBFT_C_Commit>& cs = clog.fetch(m->seqno());
		if (cs.add(m) && cs.is_complete())
		{
			execute_committed();
		}
		return;
	}
	delete m;
	return;
}

void PBFT_C_Replica::handle(PBFT_C_Checkpoint *m)
{
	const Seqno ms = m->seqno();
	if (ms > last_stable)
	{
		if (ms <= last_stable + max_out)
		{
			// PBFT_C_Checkpoint is within my window.  Check if checkpoint is
			// stable and it is above my last_executed.  This may signal
			// that messages I missed were garbage collected and I should
			// fetch the state.
			bool late = m->stable() && last_executed < ms;
			if (clog.within_range(last_executed))
			{
				PBFT_C_Time *t;
				clog.fetch(last_executed).mine(&t);
				late &= diffPBFT_C_Time(currentPBFT_C_Time(), *t) > 200000;
			}

			if (!late)
			{
				PBFT_C_Certificate<PBFT_C_Checkpoint> &cs = elog.fetch(ms);
				if (cs.add(m) && cs.mine() && cs.is_complete())
				{
					// I have enough PBFT_C_Checkpoint messages for m->seqno() to make it stable.
					// Truncate logs, discard older stable state versions.
					//	  fprintf(stderr, "CP MSG call MS %qd!!!\n", last_executed);
					mark_stable(ms, true);
				}
				//	else {
				//	  fprintf(stderr, "CP msg %qd not yet. Reason: ", ms);
				//	  if (!cs.mine())
				//	    fprintf(stderr, "does not have mine\n");
				//	  else if (!cs.is_complete())
				//	    fprintf(stderr, "Not complete\n");
				//	}
				return;
			}
		}

		if (m->verify())
		{
			// PBFT_C_Checkpoint message above my window.

			if (!m->stable())
			{
				// Send status message to obtain missing messages. This works as a
				// negative ack.
				send_status();
				delete m;
				return;
			}

			// Stable checkpoint message above my last_executed.
			PBFT_C_Checkpoint *c = sset.fetch(m->id());
			if (c == 0 || c->seqno() < ms)
			{
				delete sset.remove(m->id());
				sset.store(m);
				if (sset.size() > f())
				{
					if (last_tentative_execute > last_executed)
					{
						// Rollback to last checkpoint
						th_assert(!state.in_fetch_state(), "Invalid state");
						Seqno rc = state.rollback();
						last_tentative_execute = last_executed = rc;
						//	    fprintf(stderr, ":):):):):):):):) Set le = %d\n", last_executed);
					}

					// Stop view change timer while fetching state. It is restarted
					// in new state when the fetch ends.
					vtimer->stop();
					state.start_fetch(last_executed);
				}
				return;
			}
		}
	}
	delete m;
	return;
}

void PBFT_C_Replica::handle(PBFT_C_New_key *m)
{
	if (!m->verify())
	{
		//printf("BAD NKEY from %d\n", m->id());
	}
	delete m;
}

void PBFT_C_Replica::handle(PBFT_C_Status* m)
{
	static const int max_ret_bytes = 65536;

	if (m->verify() && qs == 0)
	{
		PBFT_C_Time current;
		PBFT_C_Time *t;
		current = currentPBFT_C_Time();
		PBFT_C_Principal *p = PBFT_C_node->i_to_p(m->id());

		// Retransmit messages that the sender is missing.
		if (last_stable > m->last_stable() + max_out)
		{
			// PBFT_C_Node is so out-of-date that it will not accept any
			// pre-prepare/prepare/commmit messages in my log.
			// Send a stable checkpoint message for my stable checkpoint.
			PBFT_C_Checkpoint *c = elog.fetch(last_stable).mine(&t);
			th_assert(c != 0 && c->stable(), "Invalid state");
			retransmit(c, current, t, p);
			delete m;
			return;
		}

		// Retransmit any checkpoints that the sender may be missing.
		int max = MIN(last_stable, m->last_stable()) + max_out;
		int min = MAX(last_stable, m->last_stable()+1);
		for (Seqno n = min; n <= max; n++)
		{
			if (n % checkpoint_interval == 0)
			{
				PBFT_C_Checkpoint *c = elog.fetch(n).mine(&t);
				if (c != 0)
				{
					retransmit(c, current, t, p);
					th_assert(n == last_stable || !c->stable(), "Invalid state");
				}
			}
		}

		if (m->view() < v)
		{
			// Retransmit my latest view-change message
			PBFT_C_View_change* vc = vi.my_view_change(&t);
			if (vc != 0)
				retransmit(vc, current, t, p);
			delete m;
			return;
		}

		if (m->view() == v)
		{
			if (m->has_nv_info())
			{
				min = MAX(last_stable+1, m->last_executed()+1);
				for (Seqno n = min; n <= max; n++)
				{
					if (m->is_committed(n))
					{
						// No need for retransmission of commit or pre-prepare/prepare
						// message.
						continue;
					}

					PBFT_C_Commit *c = clog.fetch(n).mine(&t);
					if (c != 0)
					{
						retransmit(c, current, t, p);
					}

					if (m->is_prepared(n))
					{
						// No need for retransmission of pre-prepare/prepare message.
						continue;
					}

					// If I have a pre-prepare/prepare send it, provided I have sent
					// a pre-prepare/prepare for view v.
					if (primary() == node_id)
					{
						PBFT_C_Pre_prepare *pp = plog.fetch(n).my_pre_prepare(&t);
						if (pp != 0)
						{
							retransmit(pp, current, t, p);
						}
					}
					else
					{
						PBFT_C_Prepare *pr = plog.fetch(n).my_prepare(&t);
						if (pr != 0)
						{
							retransmit(pr, current, t, p);
						}
					}
				}

				if (id() == primary())
				{
					// For now only primary retransmits big requests.
					PBFT_C_Status::BRS_iter gen(m);

					int count = 0;
					Seqno ppn;
					BR_map mrmap;
					while (gen.get(ppn, mrmap) && count <= max_ret_bytes)
					{
						if (plog.within_range(ppn))
						{
							PBFT_C_Pre_prepare_info::BRS_iter gen(
									plog.fetch(ppn).prep_info(), mrmap);
							PBFT_C_Request* r;
							while (gen.get(r))
							{
								send(r, m->id());
								count += r->size();
							}
						}
					}
				}
			}
			else
			{
				// m->has_nv_info() == false
				if (!m->has_vc(node_id))
				{
					// p does not have my view-change: send it.
					PBFT_C_View_change* vc = vi.my_view_change(&t);
					th_assert(vc != 0, "Invalid state");
					retransmit(vc, current, t, p);
				}

				if (!m->has_nv_m())
				{
					if (primary(v) == node_id && vi.has_new_view(v))
					{
						// p does not have new-view message and I am primary: send it
						PBFT_C_New_view* nv = vi.my_new_view(&t);
						if (nv != 0)
							retransmit(nv, current, t, p);
					}
				}
				else
				{
					if (primary(v) == node_id && vi.has_new_view(v))
					{
						// Send any view-change messages that p may be missing
						// that are referred to by the new-view message.  This may
						// be important if the sender of the original message is
						// faulty.
						//XXXXXXXXXX


					}
					else
					{
						// Send any view-change acks p may be missing.
						for (int i = 0; i < num_replicas; i++)
						{
							if (m->id() == i)
								continue;
							PBFT_C_View_change_ack* vca = vi.my_vc_ack(i);
							if (vca && !m->has_vc(i))
								// View-change acks are not being authenticated
								retransmit(vca, current, &current, p);
						}
					}

					// Send any pre-prepares that p may be missing and any proofs
					// of authenticity for associated requests.
					PBFT_C_Status::PPS_iter gen(m);

					int count = 0;
					Seqno ppn;
					View ppv;
					bool ppp;
					BR_map mrmap;
					while (gen.get(ppv, ppn, mrmap, ppp))
					{
						PBFT_C_Pre_prepare* pp = 0;
						if (m->id() == primary(v))
							pp = vi.pre_prepare(ppn, ppv);
						else
						{
							if (primary(v) == id() && plog.within_range(ppn))
								pp = plog.fetch(ppn).pre_prepare();
						}

						if (pp)
						{
							retransmit(pp, current, &current, p);

							if (count < max_ret_bytes && mrmap != ~0)
							{
								PBFT_C_Pre_prepare_info pi;
								pi.add_complete(pp);

								PBFT_C_Pre_prepare_info::BRS_iter gen(&pi, mrmap);
								PBFT_C_Request* r;
								while (gen.get(r))
								{
									send(r, m->id());
									count += r->size();
								}
								pi.zero(); // Make sure pp does not get deallocated
							}
						}

						if (ppp)
							vi.send_proofs(ppn, ppv, m->id());
					}
				}
			}
		}
	}
	else
	{
		// It is possible that we could not verify message because the
		// sender did not receive my last new_key message. It is also
		// possible message is bogus. We choose to retransmit last new_key
		// message.  TODO: should impose a limit on the frequency at which
		// we are willing to do it to prevent a denial of service attack.
		// This is not being done right now.
		if (last_new_key != 0 && (qs == 0 || !m->verify()))
		{
			send(last_new_key, m->id());
		}
	}

	delete m;
}

void PBFT_C_Replica::handle(PBFT_C_View_change *m)
{
	//  printf("RECV: view change v=%qd from %d\n", m->view(), m->id());
	if (m->id() == primary() && m->view() > v)
	{
		if (m->verify())
		{
			// "m" was sent by the primary for v and has a view number
			// higher than v: move to the next view.
			send_view_change();
		}
	}

	bool modified = vi.add(m);
	if (!modified)
		return;

	// TODO: memoize maxv and avoid this computation if it cannot change i.e.
	// m->view() <= last maxv. This also holds for the next check.
	View maxv = vi.max_view();
	if (maxv > v)
	{
		// PBFT_C_Replica has at least f+1 view-changes with a view number
		// greater than or equal to maxv: change to view maxv.
		v = maxv - 1;
		vc_recovering = true;
		send_view_change();

		return;
	}

	if (limbo && primary() != node_id)
	{
		maxv = vi.max_maj_view();
		th_assert(maxv <= v, "Invalid state");

		if (maxv == v)
		{
			// PBFT_C_Replica now has at least 2f+1 view-change messages with view  greater than
			// or equal to "v"

			// Start timer to ensure we move to another view if we do not
			// receive the new-view message for "v".
			vtimer->restart();
			limbo = false;
			vc_recovering = true;
		}
	}
}

void PBFT_C_Replica::handle(PBFT_C_New_view *m)
{
	//  printf("RECV: new view v=%qd from %d\n", m->view(), m->id());

	vi.add(m);
}

void PBFT_C_Replica::handle(PBFT_C_View_change_ack *m)
{
	//  printf("RECV: view-change ack v=%qd from %d for %d\n", m->view(), m->id(), m->vc_id());

	vi.add(m);
}

void PBFT_C_Replica::send_view_change()
{
	// Move to next view.
	v++;
	cur_primary = v % num_replicas;
	limbo = true;
	vtimer->stop(); // stop timer if it is still running
	ntimer->restop();

	if (last_tentative_execute > last_executed)
	{
		// Rollback to last checkpoint
		th_assert(!state.in_fetch_state(), "Invalid state");
		Seqno rc = state.rollback();
		//    printf("XXXRolled back in vc to %qd with last_executed=%qd\n", rc, last_executed);
		last_tentative_execute = last_executed = rc;
		//    fprintf(stderr, ":):):):):):):):) Set le = %d\n", last_executed);
	}

	last_prepared = last_executed;

	for (Seqno i = last_stable + 1; i <= last_stable + max_out; i++)
	{
		PBFT_C_Prepared_cert &pc = plog.fetch(i);
		PBFT_C_Certificate<PBFT_C_Commit> &cc = clog.fetch(i);

		if (pc.is_complete())
		{
			vi.add_complete(pc.rem_pre_prepare());
		}
		else
		{
			PBFT_C_Prepare *p = pc.my_prepare();
			if (p != 0)
			{
				vi.add_incomplete(i, p->digest());
			}
			else
			{
				PBFT_C_Pre_prepare *pp = pc.my_pre_prepare();
				if (pp != 0)
				{
					vi.add_incomplete(i, pp->digest());
				}
			}
		}

		pc.clear();
		cc.clear();
		// TODO: Could remember info about committed requests for efficiency.
	}

	// Create and send view-change message.
	printf("XXX SND: view change %qd\n", v);
	vi.view_change(v, last_executed, &state);
}

void PBFT_C_Replica::process_new_view(Seqno min, Digest d, Seqno max, Seqno ms)
{
	th_assert(ms >= 0 && ms <= min, "Invalid state");
	//  printf("XXX process new view: %qd\n", v);

	vtimer->restop();
	limbo = false;
	vc_recovering = true;

	if (primary(v) == id())
	{
		PBFT_C_New_view* nv = vi.my_new_view();
		send(nv, All_replicas);
	}

	// Setup variables used by mark_stable before calling it.
	seqno = max - 1;
	if (last_stable > min)
		min = last_stable;
	low_bound = min;

	if (ms > last_stable)
	{
		// Call mark_stable to ensure there is space for the pre-prepares
		// and prepares that are inserted in the log below.
		mark_stable(ms, last_executed >= ms);
	}

	// Update pre-prepare/prepare logs.
	th_assert(min >= last_stable && max-last_stable-1 <= max_out, "Invalid state");
	for (Seqno i = min + 1; i < max; i++)
	{
		Digest d;
		PBFT_C_Pre_prepare* pp = vi.fetch_request(i, d);
		PBFT_C_Prepared_cert& pc = plog.fetch(i);

		if (primary() == id())
		{
			pc.add_mine(pp);
		}
		else
		{
			PBFT_C_Prepare* p = new PBFT_C_Prepare(v, i, d);
			pc.add_mine(p);
			send(p, All_replicas);

			th_assert(pp != 0 && pp->digest() == p->digest(), "Invalid state");
			pc.add_old(pp);
		}
	}

	if (primary() == id())
	{
		send_pre_prepare();
		ntimer->start();
	}

	if (last_executed < min)
	{
		has_nv_state = false;
		state.start_fetch(last_executed, min, &d, min <= ms);
	}
	else
	{
		has_nv_state = true;

		// Execute any buffered read-only requests
		for (PBFT_C_Request *m = ro_rqueue.remove(); m != 0; m = ro_rqueue.remove())
		{
			execute_read_only(m);
			delete m;
		}
	}

	if (primary() != id() && rqueue.size() > 0)
		vtimer->restart();

	//  printf("XXX DONE:process new view: %qd\n", v);
}

PBFT_C_Pre_prepare* PBFT_C_Replica::prepared(Seqno n)
{
	PBFT_C_Prepared_cert& pc = plog.fetch(n);
	if (pc.is_complete())
	{
		return pc.pre_prepare();
	}
	return 0;
}

PBFT_C_Pre_prepare *PBFT_C_Replica::committed(Seqno s)
{
	// TODO: This is correct but too conservative: fix to handle case
	// where commit and prepare are not in same view; and to allow
	// commits without prepared requests, i.e., only with the
	// pre-prepare.
	PBFT_C_Pre_prepare *pp = prepared(s);
	if (clog.fetch(s).is_complete())
		return pp;
	return 0;
}

bool PBFT_C_Replica::execute_read_only(PBFT_C_Request *req)
{
	// JC: won't execute read-only if there's a current tentative execution
	// this probably isn't necessary if clients wait for 2f+1 RO responses
	if (last_tentative_execute == last_executed && !state.in_fetch_state()
			&& !state.in_check_state())
	{
		// Create a new PBFT_C_Reply message.
		PBFT_C_Reply *rep = new PBFT_C_Reply(view(), req->request_id(), node_id);

		// Obtain "in" and "out" buffers to call exec_command
		Byz_req inb;
		Byz_rep outb;

		inb.contents = req->command(inb.size);
		outb.contents = rep->store_reply(outb.size);

		// Execute command.
		int cid = req->client_id();
		PBFT_C_Principal *cp = i_to_p(cid);
		int error = exec_command(&inb, &outb, 0, cid, true);

		if (outb.size % ALIGNMENT_BYTES)
			for (int i = 0; i < ALIGNMENT_BYTES - (outb.size % ALIGNMENT_BYTES); i++)
				outb.contents[outb.size + i] = 0;

		if (!error)
		{
			// Finish constructing the reply and send it.
			rep->authenticate(cp, outb.size, true);
			if (outb.size < 50 || req->replier() == node_id || req->replier()
					< 0)
			{
				// Send full reply.
				send(rep, cid);
			}
			else
			{
				// Send empty reply.
				PBFT_C_Reply empty(view(), req->request_id(), node_id, rep->digest(),
						cp, true);
				send(&empty, cid);
			}
		}

		delete rep;
		return true;
	}
	else
	{
		return false;
	}
}

void PBFT_C_Replica::execute_prepared(bool committed)
{
	if (last_tentative_execute < last_executed + 1 && last_executed
			< last_stable + max_out && !state.in_fetch_state()
			&& !state.in_check_state() && has_new_view())
	{
		PBFT_C_Pre_prepare *pp = prepared(last_executed + 1);

		if (pp && pp->view() == view())
		{
			// Can execute the requests in the message with sequence number
			// last_executed+1.
			last_tentative_execute = last_executed + 1;
			th_assert(pp->seqno() == last_tentative_execute, "Invalid execution");

			// Iterate over the requests in the message, calling execute for
			// each of them.
			PBFT_C_Pre_prepare::PBFT_C_Requests_iter iter(pp);
			PBFT_C_Request req;

			while (iter.get(req))
			{
				int cid = req.client_id();
				if (replies.req_id(cid) >= req.request_id())
				{
					// PBFT_C_Request has already been executed and we have the reply to
					// the request. Resend reply and don't execute request
					// to ensure idempotence.
					replies.send_reply(cid, view(), id());
					continue;
				}

				// Obtain "in" and "out" buffers to call exec_command
				Byz_req inb;
				Byz_rep outb;
				Byz_buffer non_det;
				inb.contents = req.command(inb.size);
				outb.contents = replies.new_reply(cid, outb.size);
				non_det.contents = pp->choices(non_det.size);

				if (is_replica(cid))
				{
					// Handle recovery requests, i.e., requests from replicas,
					// differently.  TODO: make more general to allow other types
					// of requests from replicas.
					//	  printf("\n\n\nExecuting recovery request seqno=%qd rep id=%d\n", last_tentative_execute, cid);

					if (inb.size != sizeof(Seqno))
					{
						// Invalid recovery request.
						continue;
					}

					// Change keys. TODO: could change key only for recovering replica.
					if (cid != node_id)
						send_new_key();

					// Store seqno of execution.
					max_rec_n = last_tentative_execute;

					// PBFT_C_Reply includes sequence number where request was executed.
					outb.size = sizeof(last_tentative_execute);
					memcpy(outb.contents, &last_tentative_execute, outb.size);
				}
				else
				{
					// Execute command in a regular request.
					exec_command(&inb, &outb, &non_det, cid, false);
					if (outb.size % ALIGNMENT_BYTES)
						for (int i = 0; i < ALIGNMENT_BYTES - (outb.size
								% ALIGNMENT_BYTES); i++)
							outb.contents[outb.size + i] = 0;
					//if (last_tentative_execute%100 == 0)
					//  printf("%s - %qd\n",((node_id == primary()) ? "P" : "B"), last_tentative_execute);
				}

				// Finish constructing the reply.
				replies.end_reply(cid, req.request_id(), outb.size);

				// Send the reply. Replies to recovery requests are only sent
				// when the request is committed.
				if (outb.size != 0 && !is_replica(cid))
				{
					if (outb.size < 50 || req.replier() == node_id
							|| req.replier() < 0)
					{
						// Send full reply.
						replies.send_reply(cid, view(), id(), !committed);
					}
					else
					{
						// Send empty reply.
						PBFT_C_Reply empty(view(), req.request_id(), node_id,
								replies.digest(cid), i_to_p(cid), !committed);
						send(&empty, cid);
					}
				}
			}

			if ((last_executed + 1) % checkpoint_interval == 0)
			{
				state.checkpoint(last_executed + 1);
			}
		}
	}
}

void PBFT_C_Replica::execute_committed()
{
	if (!state.in_fetch_state() && !state.in_check_state() && has_new_view())
	{
		while (1)
		{
			if (last_executed >= last_stable + max_out || last_executed
					< last_stable)
				return;

			PBFT_C_Pre_prepare *pp = committed(last_executed + 1);

			if (pp && pp->view() == view())
			{
				// Tentatively execute last_executed + 1 if needed.
				execute_prepared(true);

				// Can execute the requests in the message with sequence number
				// last_executed+1.
				last_executed = last_executed + 1;
				//	fprintf(stderr, ":):):):):):):):) Set le = %d\n", last_executed);
				th_assert(pp->seqno() == last_executed, "Invalid execution");

				// Execute any buffered read-only requests
				for (PBFT_C_Request *m = ro_rqueue.remove(); m != 0; m
						= ro_rqueue.remove())
				{
					execute_read_only(m);
					delete m;
				}

				// Iterate over the requests in the message, marking the saved replies
				// as committed (i.e., non-tentative for each of them).
				PBFT_C_Pre_prepare::PBFT_C_Requests_iter iter(pp);
				PBFT_C_Request req;
				while (iter.get(req))
				{
					int cid = req.client_id();
					replies.commit_reply(cid);

					if (is_replica(cid))
					{
						// Send committed reply to recovery request.
						if (cid != node_id)
							replies.send_reply(cid, view(), id(), false);
						else
							handle(replies.reply(cid)->copy(cid), true);
					}

					// Remove the request from rqueue if present.
					if (rqueue.remove(cid, req.request_id()))
						vtimer->stop();
				}

				// Send and log PBFT_C_Checkpoint message for the new state if needed.
				if (last_executed % checkpoint_interval == 0)
				{
					Digest d_state;
					state.digest(last_executed, d_state);
					PBFT_C_Checkpoint *e = new PBFT_C_Checkpoint(last_executed, d_state);
					PBFT_C_Certificate<PBFT_C_Checkpoint> &cc = elog.fetch(last_executed);
					cc.add_mine(e);
					if (cc.is_complete())
					{
						mark_stable(last_executed, true);
						//	    fprintf(stderr, "EXEC call MS %qd!!!\n", last_executed);
					}
					//	  else
					//	    fprintf(stderr, "CP exec %qd not yet. ", last_executed);


					send(e, All_replicas);
					//	  printf(">>>>PBFT_C_Checkpointing "); d_state.print(); printf(" <<<<\n"); fflush(stdout);
				}
			}
			else
			{
				// No more requests to execute at this point.
				break;
			}
		}

		if (rqueue.size() > 0)
		{
			if (primary() == node_id)
			{
				// Send a pre-prepare with any buffered requests
				send_pre_prepare();
			}
			else
			{
				// If I am not the primary and have pending requests restart the
				// timer.
				vtimer->start();
			}
		}
	}
}

void PBFT_C_Replica::update_max_rec()
{
	// Update max_rec_n to reflect new state.
	bool change_keys = false;
	for (int i = 0; i < num_replicas; i++)
	{
		if (replies.reply(i))
		{
			int len;
			char *buf = replies.reply(i)->reply(len);
			if (len == sizeof(Seqno))
			{
				Seqno nr;
				memcpy(&nr, buf, sizeof(Seqno));

				if (nr > max_rec_n)
				{
					max_rec_n = nr;
					change_keys = true;
				}
			}
		}
	}

	// Change keys if state fetched reflects the execution of a new
	// recovery request.
	if (change_keys)
		send_new_key();
}

void PBFT_C_Replica::new_state(Seqno c)
{
	//  fprintf(stderr, ":n)e:w):s)t:a)t:e) ");
	if (vi.has_new_view(v) && c >= low_bound)
		has_nv_state = true;

	if (c > last_executed)
	{
		last_executed = last_tentative_execute = c;
		//    fprintf(stderr, ":):):):):):):):) (new_state) Set le = %d\n", last_executed);
		if (replies.new_state(&rqueue))
			vtimer->stop();

		update_max_rec();

		if (c > last_prepared)
			last_prepared = c;

		if (c > last_stable + max_out)
		{
			mark_stable(c - max_out, elog.within_range(c - max_out)
					&& elog.fetch(c - max_out).mine());
		}

		// Send checkpoint message for checkpoint "c"
		Digest d;
		state.digest(c, d);
		PBFT_C_Checkpoint* ck = new PBFT_C_Checkpoint(c, d);
		elog.fetch(c).add_mine(ck);
		send(ck, All_replicas);
	}

	// Check if c is known to be stable.
	int scount = 0;
	for (int i = 0; i < num_replicas; i++)
	{
		PBFT_C_Checkpoint* ck = sset.fetch(i);
		if (ck != 0 && ck->seqno() >= c)
		{
			th_assert(ck->stable(), "Invalid state");
			scount++;
		}
	}
	if (scount > f())
		mark_stable(c, true);

	if (c > seqno)
	{
		seqno = c;
	}

	// Execute any committed requests
	execute_committed();

	// Execute any buffered read-only requests
	for (PBFT_C_Request *m = ro_rqueue.remove(); m != 0; m = ro_rqueue.remove())
	{
		execute_read_only(m);
		delete m;
	}

	if (rqueue.size() > 0)
	{
		if (primary() == id())
		{
			// Send pre-prepares for any buffered requests
			send_pre_prepare();
		}
		else
			vtimer->restart();
	}
}

void PBFT_C_Replica::mark_stable(Seqno n, bool have_state)
{
	//XXXXXcheck if this should be < or <=

	//  fprintf(stderr, "mark stable n %qd laststable %qd\n", n, last_stable);

	if (n <= last_stable)
		return;

	last_stable = n;
	if (last_stable > low_bound)
	{
		low_bound = last_stable;
	}

	if (have_state && last_stable > last_executed)
	{
		last_executed = last_tentative_execute = last_stable;
		//    fprintf(stderr, ":):):):):):):):) (mark_stable) Set le = %d\n", last_executed);
		replies.new_state(&rqueue);
		update_max_rec();

		if (last_stable > last_prepared)
			last_prepared = last_stable;
	}
	//  else
	//    fprintf(stderr, "OH BASE! OH CLU!\n");

	if (last_stable > seqno)
		seqno = last_stable;

	//  fprintf(stderr, "mark_stable: Truncating plog to %ld have_state=%d\n", last_stable+1, have_state);
	plog.truncate(last_stable + 1);
	clog.truncate(last_stable + 1);
	vi.mark_stable(last_stable);
	elog.truncate(last_stable);
	state.discard_checkpoint(last_stable, last_executed);
	brt.mark_stable(last_stable);

	if (have_state)
	{
		// Re-authenticate my checkpoint message to mark it as stable or
		// if I do not have one put one in and make the corresponding
		// certificate complete.
		PBFT_C_Checkpoint *c = elog.fetch(last_stable).mine();
		if (c == 0)
		{
			Digest d_state;
			state.digest(last_stable, d_state);
			c = new PBFT_C_Checkpoint(last_stable, d_state, true);
			elog.fetch(last_stable).add_mine(c);
			elog.fetch(last_stable).make_complete();
		}
		else
		{
			c->re_authenticate(0, true);
		}

		try_end_recovery();
	}

	// Go over sset transfering any checkpoints that are now within
	// my window to elog.
	Seqno new_ls = last_stable;
	for (int i = 0; i < num_replicas; i++)
	{
		PBFT_C_Checkpoint* c = sset.fetch(i);
		if (c != 0)
		{
			Seqno cn = c->seqno();
			if (cn < last_stable)
			{
				c = sset.remove(i);
				delete c;
				continue;
			}

			if (cn <= last_stable + max_out)
			{
				PBFT_C_Certificate<PBFT_C_Checkpoint>& cs = elog.fetch(cn);
				cs.add(sset.remove(i));
				if (cs.is_complete() && cn > new_ls)
					new_ls = cn;
			}
		}
	}

	//XXXXXXcheck if this is safe.
	if (new_ls > last_stable)
	{
		//    fprintf(stderr, "@@@@@@@@@@@@@@@               @@@@@@@@@@@@@@@               @@@@@@@@@@@@@@@\n");
		mark_stable(new_ls, elog.within_range(new_ls)
				&& elog.fetch(new_ls).mine());
	}

	// Try to send any PBFT_C_Pre_prepares for any buffered requests.
	if (primary() == id())
		send_pre_prepare();
}

void PBFT_C_Replica::handle(PBFT_C_Data *m)
{
	state.handle(m);
}

void PBFT_C_Replica::handle(PBFT_C_Meta_data *m)
{
	state.handle(m);
}

void PBFT_C_Replica::handle(PBFT_C_Meta_data_d *m)
{
	state.handle(m);
}

void PBFT_C_Replica::handle(PBFT_C_Fetch *m)
{
	int mid = m->id();
	if (!state.handle(m, last_stable) && last_new_key != 0)
	{
		send(last_new_key, mid);
	}
}

void PBFT_C_Replica::send_new_key()
{
	PBFT_C_Node::send_new_key();

	// Cleanup messages in incomplete certificates that are
	// authenticated with the old keys.
	int max = last_stable + max_out;
	int min = last_stable + 1;
	for (Seqno n = min; n <= max; n++)
	{
		if (n % checkpoint_interval == 0)
			elog.fetch(n).mark_stale();
	}

	if (last_executed > last_stable)
		min = last_executed + 1;

	for (Seqno n = min; n <= max; n++)
	{
		plog.fetch(n).mark_stale();
		clog.fetch(n).mark_stale();
	}

	vi.mark_stale();
	state.mark_stale();
}

void PBFT_C_Replica::send_status()
{
	// Check how long ago we sent the last status message.
	PBFT_C_Time cur = currentPBFT_C_Time();
	if (diffPBFT_C_Time(cur, last_status) > 100000)
	{
		// Only send new status message if last one was sent more
		// than 100 milliseconds ago.
		last_status = cur;

		if (qs)
		{
			// Retransmit query stable if I am estimating last stable
			qs->re_authenticate();
			send(qs, All_replicas);
			return;
		}

		if (rr)
		{
			// Retransmit recovery request if I am waiting for one.
			send(rr, All_replicas);
		}

		// If fetching state, resend last fetch message instead of status.
		if (state.retrans_fetch(cur))
		{
			state.send_fetch(true);
			return;
		}

		PBFT_C_Status s(v, last_stable, last_executed, has_new_view(),
				vi.has_nv_message(v));

		if (has_new_view())
		{
			// Set prepared and committed bitmaps correctly
			int max = last_stable + max_out;
			for (Seqno n = last_executed + 1; n <= max; n++)
			{
				PBFT_C_Prepared_cert& pc = plog.fetch(n);
				if (pc.is_complete() || state.in_check_state())
				{ //XXXXXXadded state.in_check_state()
					s.mark_prepared(n);
					if (clog.fetch(n).is_complete() || state.in_check_state())
					{//XXXXXXadded state.in_check_state()
						s.mark_committed(n);
					}
				}
				else
				{
					// Ask for missing big requests
					if (!pc.is_pp_complete() && pc.pre_prepare()
							&& pc.num_correct() >= f())
						s.add_breqs(n, pc.missing_reqs());
				}
			}
		}
		else
		{
			vi.set_received_vcs(&s);
			vi.set_missing_pps(&s);
		}

		// Multicast status to all replicas.
		s.authenticate();
		send(&s, All_replicas);
	}
}

bool PBFT_C_Replica::shutdown()
{
	START_CC(shutdown_time);
	vtimer->stop();

	// Rollback to last checkpoint
	if (!state.in_fetch_state())
	{
		Seqno rc = state.rollback();
		last_tentative_execute = last_executed = rc;
		//    fprintf(stderr, ":):):):):):):):) shutd Set le = %d\n", last_executed);
	}

	if (id() == primary())
	{
		// Primary sends a view-change before shutting down to avoid
		// delaying client request processing for the view-change timeout
		// period.
		send_view_change();
	}

	char ckpt_name[1024];
	sprintf(ckpt_name, "/tmp/%s_%d", service_name, id());
	FILE* o = fopen(ckpt_name, "w");

	size_t sz = fwrite(&v, sizeof(View), 1, o);
	sz += fwrite(&limbo, sizeof(bool), 1, o);
	sz += fwrite(&has_nv_state, sizeof(bool), 1, o);

	sz += fwrite(&seqno, sizeof(Seqno), 1, o);
	sz += fwrite(&last_stable, sizeof(Seqno), 1, o);
	sz += fwrite(&low_bound, sizeof(Seqno), 1, o);
	sz += fwrite(&last_prepared, sizeof(Seqno), 1, o);
	sz += fwrite(&last_executed, sizeof(Seqno), 1, o);
	sz += fwrite(&last_tentative_execute, sizeof(Seqno), 1, o);

	bool ret = true;
	for (Seqno i = last_stable + 1; i <= last_stable + max_out; i++)
		ret &= plog.fetch(i).encode(o);

	for (Seqno i = last_stable + 1; i <= last_stable + max_out; i++)
		ret &= clog.fetch(i).encode(o);

	for (Seqno i = last_stable; i <= last_stable + max_out; i++)
		ret &= elog.fetch(i).encode(o);

	ret &= state.shutdown(o, last_stable);
	ret &= vi.shutdown(o);

	fclose(o);
	STOP_CC(shutdown_time);

	return ret & (sz == 9);
}

bool PBFT_C_Replica::restart(FILE* in)
{
	START_CC(restart_time);

	bool ret = true;
	size_t sz = fread(&v, sizeof(View), 1, in);
	sz += fread(&limbo, sizeof(bool), 1, in);
	sz += fread(&has_nv_state, sizeof(bool), 1, in);

	limbo = (limbo != 0);
	cur_primary = v % num_replicas;
	if (v < 0 || id() == primary())
	{
		ret = false;
		v = 0;
		limbo = false;
		has_nv_state = true;
	}

	sz += fread(&seqno, sizeof(Seqno), 1, in);
	sz += fread(&last_stable, sizeof(Seqno), 1, in);
	sz += fread(&low_bound, sizeof(Seqno), 1, in);
	sz += fread(&last_prepared, sizeof(Seqno), 1, in);
	sz += fread(&last_executed, sizeof(Seqno), 1, in);
	//  fprintf(stderr, ":):):):):):):):) restart read le = %d\n", last_executed);
	sz += fread(&last_tentative_execute, sizeof(Seqno), 1, in);

	ret &= (low_bound >= last_stable) & (last_tentative_execute
			>= last_executed);
	ret &= last_prepared >= last_tentative_execute;

	if (!ret)
	{
		//    fprintf(stderr, "Not ret!!! setting le to 0");
		low_bound = last_stable = last_tentative_execute = last_executed
				= last_prepared = 0;
	}

	plog.clear(last_stable + 1);
	for (Seqno i = last_stable + 1; ret && i <= last_stable + max_out; i++)
		ret &= plog.fetch(i).decode(in);

	clog.clear(last_stable + 1);
	for (Seqno i = last_stable + 1; ret && i <= last_stable + max_out; i++)
		ret &= clog.fetch(i).decode(in);

	elog.clear(last_stable);
	for (Seqno i = last_stable; ret && i <= last_stable + max_out; i++)
		ret &= elog.fetch(i).decode(in);

	ret &= state.restart(in, this, last_stable, last_tentative_execute, !ret);
	ret &= vi.restart(in, v, last_stable, !ret);

	STOP_CC(restart_time);

	return ret & (sz == 9);
}

void PBFT_C_Replica::recover()
{
	corrupt = false;

	char ckpt_name[1024];
	sprintf(ckpt_name, "/tmp/%s_%d", service_name, id());
	FILE* i = fopen(ckpt_name, "r");

	if (i == NULL || !restart(i))
	{
		// PBFT_C_Replica is faulty; start from initial state.
		fprintf(stderr, "Unable to restart from checkpoint\n");
		corrupt = true;
	}

	// Initialize recovery variables:
	recovering = true;
	vc_recovering = false;
	se.clear();
	delete qs;
	qs = 0;
	rr_reps.clear();
	delete rr;
	rr = 0;
	recovery_point = Seqno_max;
	for (int i = 0; i < num_replicas; i++)
		rr_views[i] = 0;

	// Change my incoming session keys and zero client's keys.
	START_CC(nk_time);
	send_new_key();

	unsigned zk[Key_size_u];
	bzero(zk, Key_size);
	for (int i = num_replicas; i < num_principals; i++)
	{
		PBFT_C_Principal* p = i_to_p(i);
		p->set_out_key(zk, p->last_tstamp() + 1);
	}STOP_CC(nk_time);

	//  printf("Starting estimation procedure\n");
	// Start estimation procedure.
	START_CC(est_time);
	qs = new PBFT_C_Query_stable();
	send(qs, All_replicas);

	// Add my own reply-stable message to the estimator.
	Seqno lc = last_executed / checkpoint_interval * checkpoint_interval;
	PBFT_C_Reply_stable* rs = new PBFT_C_Reply_stable(lc, last_prepared, qs->nonce(), i_to_p(
			id()));
	se.add(rs, true);
}

void PBFT_C_Replica::handle(PBFT_C_Query_stable* m)
{
	if (m->verify())
	{
		Seqno lc = last_executed / checkpoint_interval * checkpoint_interval;
		PBFT_C_Reply_stable rs(lc, last_prepared, m->nonce(), i_to_p(m->id()));

		// TODO: should put a bound on the rate at which I send these messages.
		send(&rs, m->id());
	}
	else
	{
		if (last_new_key != 0)
		{
			send(last_new_key, m->id());
		}
	}

	delete m;
}

void PBFT_C_Replica::enforce_bound(Seqno b)
{
	th_assert(recovering && se.estimate() >= 0, "Invalid state");

	bool correct = !corrupt && last_stable <= b - max_out && seqno <= b
			&& low_bound <= b && last_prepared <= b && last_tentative_execute
			<= b && last_executed <= b && (last_tentative_execute
			== last_executed || last_tentative_execute == last_executed + 1);

	for (Seqno i = b + 1; correct && (i <= plog.max_seqno()); i++)
	{
		if (!plog.fetch(i).is_empty())
			correct = false;
	}

	for (Seqno i = b + 1; correct && (i <= clog.max_seqno()); i++)
	{
		if (!clog.fetch(i).is_empty())
			correct = false;
	}

	for (Seqno i = b + 1; correct && (i <= elog.max_seqno()); i++)
	{
		if (!elog.fetch(i).is_empty())
			correct = false;
	}

	Seqno known_stable = se.low_estimate();
	if (!correct)
	{
		fprintf(stderr, "Incorrect state setting low bound to %qd\n",
				known_stable);
		seqno = last_prepared = low_bound = last_stable = known_stable;
		last_tentative_execute = last_executed = 0;
		limbo = false;
		plog.clear(known_stable + 1);
		clog.clear(known_stable + 1);
		elog.clear(known_stable);
	}

	correct &= vi.enforce_bound(b, known_stable, !correct);
	correct &= state.enforce_bound(b, known_stable, !correct);
	corrupt = !correct;
}

void PBFT_C_Replica::handle(PBFT_C_Reply_stable* m)
{
	if (qs && qs->nonce() == m->nonce())
	{
		if (se.add(m))
		{
			// Done with estimation.
			delete qs;
			qs = 0;
			recovery_point = se.estimate() + max_out;

			enforce_bound(recovery_point);
			STOP_CC(est_time);

			//      printf("sending recovery request\n");
			// Send recovery request.
			START_CC(rr_time);
			rr = new PBFT_C_Request(new_rid());

			int len;
			char* buf = rr->store_command(len);
			th_assert(len >= (int)sizeof(recovery_point), "PBFT_C_Request is too small");
			memcpy(buf, &recovery_point, sizeof(recovery_point));

			rr->sign(sizeof(recovery_point));
			send(rr, primary());
			STOP_CC(rr_time);

			//      printf("Starting state checking\n");

			// Stop vtimer while fetching state. It is restarted when the fetch ends
			// in new_state.
			vtimer->stop();
			state.start_check(last_executed);

			// Leave multicast group.
			//            printf("XXX Leaving mcast group\n");
			leave_mcast_group();

			rqueue.clear();
			ro_rqueue.clear();
		}
		return;
	}
	delete m;
}

void PBFT_C_Replica::enforce_view(View rec_view)
{
	th_assert(recovering, "Invalid state");

	if (rec_view >= v || vc_recovering || (limbo && rec_view + 1 == v))
	{
		// PBFT_C_Replica's view number is reasonable; do nothing.
		return;
	}

	corrupt = true;
	vi.clear();
	v = rec_view - 1;
	send_view_change();
}

void PBFT_C_Replica::handle(PBFT_C_Reply *m, bool mine)
{
	int mid = m->id();
	int mv = m->view();

	if (rr && rr->request_id() == m->request_id() && (mine
			|| !m->is_tentative()))
	{
		// Only accept recovery request replies that are not tentative.
		bool added = (mine) ? rr_reps.add_mine(m) : rr_reps.add(m);
		if (added)
		{
			if (rr_views[mid] < mv)
				rr_views[mid] = mv;

			if (rr_reps.is_complete())
			{
				// I have a valid reply to my outstanding recovery request.
				// Update recovery point
				int len;
				const char *rep = rr_reps.cvalue()->reply(len);
				th_assert(len == sizeof(Seqno), "Invalid message");

				Seqno rec_seqno;
				memcpy(&rec_seqno, rep, len);
				Seqno new_rp = rec_seqno / checkpoint_interval
						* checkpoint_interval + max_out;
				if (new_rp > recovery_point)
					recovery_point = new_rp;

				//	printf("XXX Complete rec reply with seqno %qd rec_point=%qd\n",rec_seqno,  recovery_point);

				// Update view number
				View rec_view = K_max<View> (f() + 1, rr_views, n(), View_max);
				enforce_view(rec_view);

				try_end_recovery();

				delete rr;
				rr = 0;
			}
		}
		return;
	}
	delete m;
}

void PBFT_C_Replica::send_null()
{
	th_assert(id() == primary(), "Invalid state");

	Seqno max_rec_point = max_out + (max_rec_n + checkpoint_interval - 1)
			/ checkpoint_interval * checkpoint_interval;

	if (max_rec_n && max_rec_point > last_stable && has_new_view())
	{
		if (rqueue.size() == 0 && seqno <= last_executed && seqno + 1
				<= max_out + last_stable)
		{
			// Send null request if there is a recovery in progress and there
			// are no outstanding requests.
			seqno++;
			PBFT_C_Req_queue empty;
			PBFT_C_Pre_prepare* pp = new PBFT_C_Pre_prepare(view(), seqno, empty);
			send(pp, All_replicas);
			plog.fetch(seqno).add_mine(pp);
		}
	}
	ntimer->restart();

	// TODO: backups should force view change if primary does not send null requests
	// to allow recoveries to complete.
}

//
// PBFT_C_Timeout handlers:
//

void vtimer_handler()
{
	th_assert(PBFT_C_replica, "replica is not initialized\n");
	if (!PBFT_C_replica->delay_vc())
		PBFT_C_replica->send_view_change();
	else
		PBFT_C_replica->vtimer->restart();
}

void stimer_handler()
{
	th_assert(PBFT_C_replica, "replica is not initialized\n");
	PBFT_C_replica->send_status();

	PBFT_C_replica->stimer->restart();
}

void rec_timer_handler()
{
	th_assert(PBFT_C_replica, "replica is not initialized\n");
	static int rec_count = 0;

	PBFT_C_replica->rtimer->restart();

	if (!PBFT_C_replica->rec_ready)
	{
		// PBFT_C_Replica is not ready to recover
		return;
	}

#ifdef RECOVERY
	if (PBFT_C_replica->n() - 1 - rec_count % PBFT_C_replica->n() == PBFT_C_replica->id())
	{
		// Start recovery:
		INIT_REC_STATS();

		if (PBFT_C_replica->recovering)
			INCR_OP(incomplete_recs);

		printf("* Starting recovery\n");

		// PBFT_C_Checkpoint
		PBFT_C_replica->shutdown();

		PBFT_C_replica->state.simulate_reboot();

		PBFT_C_replica->recover();
	}
	else
	{
		if (PBFT_C_replica->recovering)
			INCR_OP(rec_overlaps);
	}

#endif

	rec_count++;
}

void ntimer_handler()
{
	th_assert(PBFT_C_replica, "replica is not initialized\n");

	PBFT_C_replica->send_null();
}

bool PBFT_C_Replica::has_req(int cid, const Digest &d)
{
	PBFT_C_Request* req = rqueue.first_client(cid);

	if (req && req->digest() == d)
		return true;

	return false;
}

void PBFT_C_Replica::join_mcast_group()
{
	struct ip_mreq req;
	req.imr_multiaddr.s_addr = group->address()->sin_addr.s_addr;
	req.imr_interface.s_addr = INADDR_ANY;
	int error = setsockopt(sock, IPPROTO_IP, IP_ADD_MEMBERSHIP, (char *) &req,
			sizeof(req));
	if (error < 0)
	{
		perror("Unable to join group");
		exit(1);
	}
}

void PBFT_C_Replica::leave_mcast_group()
{
	struct ip_mreq req;
	req.imr_multiaddr.s_addr = group->address()->sin_addr.s_addr;
	req.imr_interface.s_addr = INADDR_ANY;
	int error = setsockopt(sock, IPPROTO_IP, IP_DROP_MEMBERSHIP, (char *) &req,
			sizeof(req));
	if (error < 0)
	{
		perror("Unable to join group");
		exit(1);
	}
}

void PBFT_C_Replica::try_end_recovery()
{
	if (recovering && last_stable >= recovery_point && !state.in_check_state()
			&& rr_reps.is_complete())
	{
		// Done with recovery.
		END_REC_STATS();

		//    printf("XXX join mcast group\n");
		join_mcast_group();

		//        printf("Done with recovery\n");
		recovering = false;

		// Execute any buffered read-only requests
		for (PBFT_C_Request *m = ro_rqueue.remove(); m != 0; m = ro_rqueue.remove())
		{
			execute_read_only(m);
			delete m;
		}
	}
}

#ifndef NO_STATE_TRANSLATION
char* PBFT_C_Replica::get_cached_obj(int i)
{
	return state.get_cached_obj(i);
}
#endif
