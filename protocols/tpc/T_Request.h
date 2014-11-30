#ifndef _T_Request_h
#define _T_Request_h 1

#include "T_Message.h"
#include "types.h"
#include "Digest.h"

#include "ARequest.h"

//class Digest;
class T_Principal;

//
// Request messages have the following format.
//
struct T_Request_rep : public T_Message_rep
{
		//Digest od; // Digest of rid, cid, command.
		short command_size;
		int cid; // unique id of client who sends the request
		int sender_id; // unique id of the forwarder
		Request_id rid; // unique request identifier
		//uint64_t unused;

		// Followed a command which is "command_size" bytes long and an
		// authenticator.
		// Digest resp_digest;
		
};

class T_Request : public T_Message, public ARequest
{
		//
		// Request messages:
		//
		// Requires: Requests that may have been allocated by library users
		// through the libbyz.h interface can not be trimmed (this could free
		// memory the user expects to be able to use.)
		//
	public:
		T_Request(T_Request_rep *r);
		T_Request(Request_id r);
		// Effects: Creates a new Request message with an empty
		// command and no authentication. The methods store_command and
		// authenticate should be used to finish message construction.
		// "rr" is the identifier of the replica from which the client
		// expects a full reply (if negative, client expects a full reply
		// from all replicas).

		virtual ~T_Request();

		static const int big_req_thresh = 8000; // Maximum size of not-big requests

		char* store_command(int &max_len);
		// Effects: Returns a pointer to the location within the message
		// where the command should be stored and sets "max_len" to the number of
		// bytes available to store the reply. The caller can copy any command
		// with length less than "max_len" into the returned buffer.

		int& client_id() const;
		int sender_id() const;

		Request_id& request_id();

		char* command(int &len);

		bool is_read_only() const;

		static bool convert(T_Message *m1, T_Request *&m2);

		void display() const;

		virtual Digest& digest() const { exit(1); }

	private:
		T_Request_rep &rep() const;
		// Effects: Casts "msg" to a Request_rep&

		//Added by Maysam Yabandeh
		//TODO: Clean this dirty imprivization
		friend class T_Replica;
		friend class T_Client;
};

inline Request_id &T_Request::request_id()
{
	return rep().rid;
}

inline char *T_Request::command(int &len)
{
	len = rep().command_size;
	return contents() + sizeof(T_Request_rep);
}

inline bool T_Request::is_read_only() const
{
	return rep().extra & 1;
}

inline int& T_Request::client_id() const {
	return rep().cid;
}

inline int T_Request::sender_id() const {
	return rep().sender_id;
}

#endif // _T_Request_h
