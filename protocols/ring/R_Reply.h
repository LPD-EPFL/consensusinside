#ifndef _R_Reply_h
#define _R_Reply_h 1

#include "types.h"
#include "R_Message.h"
#include "Digest.h"

class R_Principal;
class R_Rep_info;

//
// R_Reply messages have the following format.
//
struct R_Reply_rep : public R_Message_rep
{
		Request_id rid; // unique request identifier
		Digest digest; // digest of reply.
		int reply_size; // if negative, reply is not full.
		int replica; // id of the replica which sends the message
		// Followed by a reply that is "reply_size" bytes long and
		// a MAC authenticating the reply to the client. The MAC is computed
		// only over the R_Reply_rep. Replies can be empty or full. Full replies
		// contain the actual reply and have reply_size >= 0. Empty replies
		// do not contain the actual reply and have reply_size < 0.
};

class R_Reply : public R_Message
{
		//
		// R_Reply messages
		//
	public:
		R_Reply();

		R_Reply(R_Reply_rep *r);

		virtual ~R_Reply();

		char *store_reply(int &max_len);
		// Effects: Returns a pointer to the location within the message
		// where the reply should be stored and sets "max_len" to the number of
		// bytes available to store the reply. The caller can copy any reply
		// with length less than "max_len" into the returned buffer.

		Request_id request_id() const;
		// Effects: Fetches the request identifier from the message.

		Digest &digest() const;
		// Effects: Fetches the digest from the message.

		char *reply(int &len);
		// Effects: Returns a pointer to the reply and sets len to the
		// reply size.

		int& reply_size();
		// Return the reply size

		bool verify();
		// Effects: Verifies if the message is authenticated by rep().replica.

		bool match(R_Reply *r);
		// Effects: Returns true if the replies match.

		static bool convert(R_Message *m1, R_Reply *&m2);
		// Effects: If "m1" has the right size and tag of a "R_Reply", casts
		// "m1" to a "R_Reply" pointer, returns the pointer in "m2" and
		// returns true. Otherwise, it returns false. Convert also trims any
		// surplus storage from "m1" when the conversion is successfull.

		R_Reply_rep &rep() const;
		// Effects: Casts "msg" to a R_Reply_rep&
	private:

		friend class R_Rep_info;
};

inline R_Reply_rep& R_Reply::rep() const
{
	th_assert(ALIGNED(msg), "Improperly aligned pointer");
	return *((R_Reply_rep*)msg);
}

inline Request_id R_Reply::request_id() const
{
	return rep().rid;
}
inline Digest& R_Reply::digest() const
{
	return rep().digest;
}

inline char* R_Reply::reply(int &len)
{
	len = rep().reply_size;
	return contents()+sizeof(R_Reply_rep);
}
inline int& R_Reply::reply_size()
{
	return rep().reply_size;
}
inline bool R_Reply::match(R_Reply *r)
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

	bool toRet = ((rep().rid == r->rep().rid) && (digest()== r->digest()));
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
#endif // _R_Reply_h
