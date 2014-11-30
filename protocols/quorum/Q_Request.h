#ifndef _Q_Request_h
#define _Q_Request_h 1

#include "Q_Message.h"
#include "types.h"
#include "Digest.h"

#include "ARequest.h"

//class Digest;
class Q_Principal;

//
// Request messages have the following format.
//
struct Q_Request_rep : public Q_Message_rep
{
		//Added by Maysam Yabandeh
		int order; //assigned order by the primary
		//this filed is -1 when the primary itself receive the request

		Digest od; // Digest of rid, cid, command.
		short replier; // id of replica from which client
		// expects to receive a full reply
		// (if negative, it means all replicas).
		short command_size;
		int cid; // unique id of client who sends the request
		Request_id rid; // unique request identifier
		uint64_t unused;
		// Followed a command which is "command_size" bytes long and an
		// authenticator.
		// Digest resp_digest;
		
};

class Q_Request : public Q_Message, public ARequest
{
		//
		// Request messages:
		//
		// Requires: Requests that may have been allocated by library users
		// through the libbyz.h interface can not be trimmed (this could free
		// memory the user expects to be able to use.)
		//
	public:
	        Q_Request(Q_Request_rep *r);
		Q_Request(Request_id r, short rr=-1);
		// Effects: Creates a new Request message with an empty
		// command and no authentication. The methods store_command and
		// authenticate should be used to finish message construction.
		// "rr" is the identifier of the replica from which the client
		// expects a full reply (if negative, client expects a full reply
		// from all replicas).

		virtual ~Q_Request();

		static const int big_req_thresh = 8000; // Maximum size of not-big requests

		char* store_command(int &max_len);
		// Effects: Returns a pointer to the location within the message
		// where the command should be stored and sets "max_len" to the number of
		// bytes available to store the reply. The caller can copy any command
		// with length less than "max_len" into the returned buffer.

		void authenticate(int act_len, bool read_only=false);
		// Effects: Terminates the construction of a request message by
		// setting the length of the command to "act_len", and appending an
		// authenticator. read-only should be true iff the request is read-only
		// (i.e., it will not change the service state).

		void set_unused() { rep().unused = 1; }
		void clear_unused() { rep().unused = 0; }
		int is_unused_set() const { return rep().unused != 0; }
		int& client_id() const;

		Request_id& request_id();

		char* command(int &len);

		int replier() const;
		// Effects: Returns the identifier of the replica from which
		// the client expects a full reply. If negative, client expects
		// a full reply from all replicas.

		void set_replier(int replier) const;
		// Effects: Sets the identifier of the replica from which
		// the client expects a full reply.

		bool is_read_only() const;

		bool verify();
		// Effects: Verifies if the message is authenticated by the client
		// "client_id()" using an authenticator.

		static bool convert(Q_Message *m1, Q_Request *&m2);

		void comp_digest(Digest& d);

		void display() const;

		virtual Digest& digest() const { return rep().od; }

		//Added by Maysam Yabandeh
		void setOrder(int);
		int getOrder();

	private:
		Q_Request_rep &rep() const;
		// Effects: Casts "msg" to a Request_rep&

		//Added by Maysam Yabandeh
		//TODO: Clean this dirty imprivization
		friend class Q_Replica;

};

inline Request_id &Q_Request::request_id()
{
	return rep().rid;
}

inline char *Q_Request::command(int &len)
{
	len = rep().command_size;
	return contents() + sizeof(Q_Request_rep);
}

inline int Q_Request::replier() const
{
	return rep().replier;
}

inline void Q_Request::set_replier(int replier) const
{
	rep().replier = replier;
}

inline bool Q_Request::is_read_only() const
{
	return rep().extra & 1;
}

inline int& Q_Request::client_id() const {
	return rep().cid;
}
//Added by Maysam Yabandeh
inline void Q_Request::setOrder(int _order) 
{
	rep().order = _order;
}
inline int Q_Request::getOrder()
{
	return rep().order;
}

#endif // _Q_Request_h
