#ifndef _O_Request_h
#define _O_Request_h 1

#include "O_Message.h"
#include "types.h"
#include "Digest.h"

#include "ARequest.h"

//class Digest;
class O_Principal;


//
// Request messages have the following format.
//
struct O_Request_rep : public O_Message_rep
{
		//Digest od; // Digest of rid, cid, command.
		short command_size;
		int cid; // unique id of client who sends the request
		int sender_id; // unique id of the forwarder
		Request_id rid; // unique request identifier
		//Accept & Learn
		seq_t n;
		uint64_t index;

		//uint64_t unused;

		// Followed a command which is "command_size" bytes long and an
		// authenticator.
		// Digest resp_digest;
		
};

class O_Request : public O_Message, public ARequest
{
		//
		// Request messages:
		//
		// Requires: Requests that may have been allocated by library users
		// through the libbyz.h interface can not be trimmed (this could free
		// memory the user expects to be able to use.)
		//
	public:
		O_Request(int t, unsigned sz);
		O_Request(O_Request_rep *r);
		O_Request(Request_id r);
		// Effects: Creates a new Request message with an empty
		// command and no authentication. The methods store_command and
		// authenticate should be used to finish message construction.
		// "rr" is the identifier of the replica from which the client
		// expects a full reply (if negative, client expects a full reply
		// from all replicas).

		virtual ~O_Request();

		static const int big_req_thresh = 8000; // Maximum size of not-big requests

		char* store_command(int &max_len);
		// Effects: Returns a pointer to the location within the message
		// where the command should be stored and sets "max_len" to the number of
		// bytes available to store the reply. The caller can copy any command
		// with length less than "max_len" into the returned buffer.

		int& client_id() const;
		int& sender_id() const;

		Request_id& request_id();

		char* command(int &len);

		bool is_read_only() const;

		static bool convert(O_Message *m1, O_Request *&m2);

		void display() const;

		virtual Digest& digest() const { exit(1); }

	protected:
		O_Request_rep &rep() const;
		// Effects: Casts "msg" to a Request_rep&

		//Added by Maysam Yabandeh
		//TODO: Clean this dirty imprivization
		//friend class O_Replica;
		//friend class O_Client;
};

//Added by Maysam Yabandeh
//Because the accept message must essential contain the whole request
//It still uses O_Request_rep but the public method allow access for more vars
class O_Accept : public O_Request
{
	public:
		O_Accept() : O_Request(O_Accept_tag, sizeof(O_Request_rep)) {
			set_size(sizeof(O_Request_rep));
		}
	   //No other constructor is necessary
		//must use the received O_Request message
		
		static bool convert(O_Message *m1, O_Accept *&m2);

		void convert();// { msg->tag = O_Accept_tag; sender_id() = O_node->id();}
		//must be called after casting O_Request to O_Accept
		
		seq_t& n() { return rep().n; }
		uint64_t& index() { return rep().index; }
};
class O_Learn : public O_Request
{
	public:
		O_Learn() : O_Request(O_Learn_tag, sizeof(O_Request_rep)) {
			set_size(sizeof(O_Request_rep));
		}
	   //No other constructor is necessary
		//must use the received O_Request message

		static bool convert(O_Message *m1, O_Learn *&m2);
		
		void convert();// { msg->tag = O_Learn_tag; sender_id() = O_node->id();}
		//must be called after casting O_Request to O_Learn

		seq_t& n() { return rep().n; }
		uint64_t& index() { return rep().index; }
};

inline Request_id &O_Request::request_id()
{
	return rep().rid;
}

inline char *O_Request::command(int &len)
{
	len = rep().command_size;
	return contents() + sizeof(O_Request_rep);
}

inline bool O_Request::is_read_only() const
{
	return rep().extra & 1;
}

inline int& O_Request::client_id() const {
	return rep().cid;
}

inline int& O_Request::sender_id() const {
	return rep().sender_id;
}

#endif // _O_Request_h
