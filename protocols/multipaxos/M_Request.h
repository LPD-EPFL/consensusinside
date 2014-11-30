#ifndef _M_Request_h
#define _M_Request_h 1

#include "M_Message.h"
#include "types.h"
#include "Digest.h"

#include "ARequest.h"
#include "M_Certificate.h"

//class Digest;
class M_Principal;


//
// Request messages have the following format.
//
struct M_Request_rep : public M_Message_rep
{
		//Digest od; // Digest of rid, cid, command.
		short command_size;
		int cid; // unique id of client who sends the request
		int sender_id; // unique id of the forwarder
		Request_id rid; // unique request identifier
		//Accept & Learn
		seq_t n;
		uint64_t index;
		//PrepareResponse
		bool isNull;
		seq_t responseN;
		//acceptor change
		uint8_t acceptor;

		//uint64_t unused;

		// Followed a command which is "command_size" bytes long and an
		// authenticator.
		// Digest resp_digest;
		
};

class M_Request : public M_Message, public ARequest
{
		//
		// Request messages:
		//
		// Requires: Requests that may have been allocated by library users
		// through the libbyz.h interface can not be trimmed (this could free
		// memory the user expects to be able to use.)
		//
	public:
		M_Request(int t, unsigned sz);
		M_Request(M_Request_rep *r);
		M_Request(Request_id r);
		// Effects: Creates a new Request message with an empty
		// command and no authentication. The methods store_command and
		// authenticate should be used to finish message construction.
		// "rr" is the identifier of the replica from which the client
		// expects a full reply (if negative, client expects a full reply
		// from all replicas).

		virtual ~M_Request();

		static const int big_req_thresh = 8000; // Maximum size of not-big requests

		char* store_command(int &max_len);
		// Effects: Returns a pointer to the location within the message
		// where the command should be stored and sets "max_len" to the number of
		// bytes available to store the reply. The caller can copy any command
		// with length less than "max_len" into the returned buffer.

		int& client_id() const;
		int& sender_id() const;
		//Added by Maysam Yabandeh
		//used by one acceptor
		uint64_t& index();

		uint8_t& acceptor_id() const;

		Request_id& request_id();

		char* command(int &len);

		bool is_read_only() const;

		static bool convert(M_Message *m1, M_Request *&m2);

		void display() const;

		virtual Digest& digest() const { exit(1); }

	protected:
		M_Request_rep &rep() const;
		// Effects: Casts "msg" to a Request_rep&

		//Added by Maysam Yabandeh
		//TODO: Clean this dirty imprivization
		//friend class M_Replica;
		//friend class M_Client;
};

//Added by Maysam Yabandeh
//just to cleanly give access to n
class M_ForwardedRequest : public M_Request
{
public: 
		seq_t& n() { return rep().n; }
};
//Because the accept message must essential contain the whole request
//It still uses M_Request_rep but the public method allow access for more vars
/*
class M_Accept : public M_Request
{
	public:
		M_Accept() : M_Request(M_Accept_tag, sizeof(M_Request_rep)) {
		   rep().command_size = 0;
			set_size(sizeof(M_Request_rep));
		}
	   //No other constructor is necessary
		//must use the received M_Request message
		
		static bool convert(M_Message *m1, M_Accept *&m2);

		void convert() { msg->tag = M_Accept_tag; }
		//must be called after casting M_Request to M_Accept
		
		seq_t& n() { return rep().n; }
		uint64_t& index() { return rep().index; }
};
class M_Learn : public M_Request
{
	public:
		M_Learn() : M_Request(M_Learn_tag, sizeof(M_Request_rep)) {
		   rep().command_size = 0;
			set_size(sizeof(M_Request_rep));
		}
	   //No other constructor is necessary
		//must use the received M_Request message

		static bool convert(M_Message *m1, M_Learn *&m2);
		
		void convert() { msg->tag = M_Learn_tag; }
		//must be called after casting M_Request to M_Learn

		seq_t& n() { return rep().n; }
		uint64_t& index() { return rep().index; }
};
class M_PrepareResponse : public M_Request
{
	public:
		M_PrepareResponse() : M_Request(M_PrepareResponse_tag, sizeof(M_Request_rep)) {
		   rep().command_size = 0;
			set_size(sizeof(M_Request_rep));
			rep().isNull = false;
		}
		M_PrepareResponse(uint64_t index, seq_t n) : M_Request(M_PrepareResponse_tag, sizeof(M_Request_rep)) {
			set_size(sizeof(M_Request_rep));
         rep().index = index;
			rep().responseN = n;
			rep().isNull = true;
			rep().sender_id = M_node->id();
		}
	   //No other constructor is necessary
		//must use the received M_Request message

		static bool convert(M_Message *m1, M_PrepareResponse *&m2);
		
		void convert() { msg->tag = M_PrepareResponse_tag; }
		//must be called after casting M_Request to M_PrepareResponse

		seq_t& n() { return rep().n; }
		seq_t& responseN() { return rep().responseN; }
		uint64_t& index() { return rep().index; }
		bool isValueNull() { return rep().isNull; }
};
*/

inline Request_id &M_Request::request_id()
{
	return rep().rid;
}

inline char *M_Request::command(int &len)
{
	len = rep().command_size;
	return contents() + sizeof(M_Request_rep);
}

inline bool M_Request::is_read_only() const
{
	return rep().extra & 1;
}

inline int& M_Request::client_id() const {
	return rep().cid;
}

inline int& M_Request::sender_id() const {
	return rep().sender_id;
}

inline uint64_t& M_Request::index() {
	return rep().index;
}

inline uint8_t& M_Request::acceptor_id() const {
	return rep().acceptor;
}

#endif // _M_Request_h
