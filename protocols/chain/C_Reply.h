#ifndef _C_Reply_h
#define _C_Reply_h 1

#include "types.h"
#include "C_Message.h"
#include "Digest.h"

class C_Principal;
class C_Rep_info;

//
// C_Reply messages have the following format.
//
struct C_Reply_rep : public C_Message_rep
{
		View v; // current view
		Request_id rid; // unique request identifier
		Digest digest; // digest of reply.
		int replica; // id of replica sending the reply
		int reply_size; // if negative, reply is not full.
		int cid;
		int unused;
		// Followed by a reply that is "reply_size" bytes long and
		// a MAC authenticating the reply to the client. The MAC is computed
		// only over the C_Reply_rep. Replies can be empty or full. Full replies
		// contain the actual reply and have reply_size >= 0. Empty replies
		// do not contain the actual reply and have reply_size < 0.
};

class C_Reply : public C_Message
{
		//
		// C_Reply messages
		//
	public:
		C_Reply();

		C_Reply(C_Reply_rep *r);

		virtual ~C_Reply();

		char *store_reply(int &max_len);
		// Effects: Returns a pointer to the location within the message
		// where the reply should be stored and sets "max_len" to the number of
		// bytes available to store the reply. The caller can copy any reply
		// with length less than "max_len" into the returned buffer.

		View view() const;
		// Effects: Fetches the view from the message

		Request_id request_id() const;
		// Effects: Fetches the request identifier from the message.

		int id() const;
		// Effects: Fetches the reply identifier from the message.
		int cid() const;
		// Effects: Fetches the identifier of the client who issued the request this message is in response to.

		Digest &digest() const;
		// Effects: Fetches the digest from the message.

		char *reply(int &len);
		// Effects: Returns a pointer to the reply and sets len to the
		// reply size.

		int& reply_size();
		// Return the reply size

		bool verify();
		// Effects: Verifies if the message is authenticated by rep().replica.

		bool match(C_Reply *r);
		// Effects: Returns true if the replies match.

		static bool convert(C_Message *m1, C_Reply *&m2);
		// Effects: If "m1" has the right size and tag of a "C_Reply", casts
		// "m1" to a "C_Reply" pointer, returns the pointer in "m2" and
		// returns true. Otherwise, it returns false. Convert also trims any
		// surplus storage from "m1" when the conversion is successfull.

	private:
		C_Reply_rep &rep() const;
		// Effects: Casts "msg" to a C_Reply_rep&

		friend class C_Rep_info;
};

inline C_Reply_rep& C_Reply::rep() const
{
	th_assert(ALIGNED(msg), "Improperly aligned pointer");
	return *((C_Reply_rep*)msg);
}

inline View C_Reply::view() const
{
	return rep().v;
}

inline Request_id C_Reply::request_id() const
{
	return rep().rid;
}
inline int C_Reply::cid() const
{
	return rep().cid;
}
inline int C_Reply::id() const
{
	return rep().replica;
}
inline Digest& C_Reply::digest() const
{
	return rep().digest;
}

inline char* C_Reply::reply(int &len)
{
	len = rep().reply_size;
	return contents()+sizeof(C_Reply_rep);
}
inline int& C_Reply::reply_size()
{
	return rep().reply_size;
}
inline bool C_Reply::match(C_Reply *r)
{
	/*
	 Digest &d1 = digest();
	 Digest &d2 = r->digest();
	 Digest &d3 = request_history_digest();
	 Digest &d4 = r->request_history_digest();
	 */
	// TODO this is an HACK that is temporary because empty messages do not have a request_history_digest
	// TODO reproduce the bug using reply size = 4096
	if ((r->reply_size() < 0) || (reply_size() < 0))
	{
		return true;
	}

	bool toRet = ((rep().v == r->rep().v) && (rep().rid == r->rep().rid) && (digest()== r->digest()));
	/*
	 fprintf(stderr, "Reply::match [%u,%u,%u,%u]==[%u,%u,%u,%u] && [%u,%u,%u,%u]==[%u,%u,%u,%u]\n", d1.udigest()[0], d1.udigest()[1], d1.udigest()[2], d1.udigest()[3]
	 , d2.udigest()[0], d2.udigest()[1], d2.udigest()[2], d2.udigest()[3]
	 , d3.udigest()[0], d3.udigest()[1], d3.udigest()[2], d3.udigest()[3]
	 , d4.udigest()[0], d4.udigest()[1], d4.udigest()[2], d4.udigest()[3]);
	 */
	if (toRet)
	{
		//      fprintf(stderr, "reply_match returning true\n");
		return true;
	}
	//   fprintf(stderr, "reply_match returning false\n");
	return false;
}
#endif // _C_Reply_h
