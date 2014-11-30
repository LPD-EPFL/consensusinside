#ifndef _zl_Request_h
#define _zl_Request_h 1

#include "zl_Message.h"
#include "types.h"
#include "Digest.h"

#include "ARequest.h"

//class Digest;
class zl_Principal;

//
// Request messages have the following format.
//
struct zl_Request_rep : public zl_Message_rep
{
		Digest od; // Digest of rid, cid, command.
		short replier; // id of replica from which client
		// expects to receive a full reply
		// (if negative, it means all replicas).
		short command_size;
		int cid; // unique id of client who sent the request
		Request_id rid; // unique request identifier
		// Followed a command which is "command_size" bytes long and an
		// authenticator.
		// Digest resp_digest;
};

class zl_Request : public zl_Message, public ARequest
{
		//
		// Request messages:
		//
		// Requires: Requests that may have been allocated by library users
		// through the libbyz.h interface can not be trimmed (this could free
		// memory the user expects to be able to use.)
		//
	public:
	        zl_Request(zl_Request_rep *r);
		zl_Request(Request_id r, short rr=-1);
		// Effects: Creates a new Request message with an empty
		// command and no authentication. The methods store_command and
		// authenticate should be used to finish message construction.
		// "rr" is the identifier of the replica from which the client
		// expects a full reply (if negative, client expects a full reply
		// from all replicas).

		virtual ~zl_Request();

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

		virtual int& client_id() const;

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

		static bool convert(zl_Message *m1, zl_Request *&m2);

		void comp_digest(Digest& d);

		void display() const;

		virtual Digest& digest() const { return rep().od; }

	private:
		zl_Request_rep &rep() const;
		// Effects: Casts "msg" to a Request_rep&

};

inline Request_id &zl_Request::request_id()
{
	return rep().rid;
}

inline char *zl_Request::command(int &len)
{
	len = rep().command_size;
	return contents() + sizeof(zl_Request_rep);
}

inline int zl_Request::replier() const
{
	return rep().replier;
}

inline void zl_Request::set_replier(int replier) const
{
	rep().replier = replier;
}

inline bool zl_Request::is_read_only() const
{
	return rep().extra & 1;
}

inline int& zl_Request::client_id() const {
	return rep().cid;
}
#endif // _zl_Request_h
