#ifndef _M_PrepareResponse_h
#define _M_PrepareResponse_h 1

#include "M_Message.h"
#include "types.h"
#include "Digest.h"

#include "M_Certificate.h"

//class Digest;
class M_Principal;


//
// Request messages have the following format.
//
struct M_PrepareResponse_rep : public M_Message_rep
{
		//Digest od; // Digest of rid, cid, command.
		short command_size;
		int cid; // unique id of client who sends the request
		int sender_id; // unique id of the forwarder
		Request_id rid; // unique request identifier
		//PrepareResponse & Learn
		seq_t n;
		uint64_t index;
		//PrepareResponse
		bool isNull;
		seq_t responseN;

		//uint64_t unused;

		// Followed a command which is "command_size" bytes long and an
		// authenticator.
		// Digest resp_digest;
		
};

//Added by Maysam Yabandeh
//Because the accept message must essential contain the whole request
//It still uses M_PrepareResponse_rep but the public method allow access for more vars
class M_PrepareResponse : public M_Message
{
	public:
		M_PrepareResponse() : M_Message(M_PrepareResponse_tag, sizeof(M_PrepareResponse_rep)) {
			rep().command_size = 0;
			rep().isNull = false;
			rep().sender_id = M_node->id();
			set_size(sizeof(M_PrepareResponse_rep));
		}
		M_PrepareResponse(uint64_t index, seq_t n) : M_Message(M_PrepareResponse_tag, sizeof(M_PrepareResponse_rep)) {
         rep().index = index;
			rep().responseN = n;
			rep().isNull = true;
			rep().sender_id = M_node->id();
			set_size(sizeof(M_PrepareResponse_rep));
		}
	   //No other constructor is necessary
		//must use the received M_PrepareResponse message
		
		static bool convert(M_Message *m1, M_PrepareResponse *&m2);

		char* store_command(int &max_len);
		int& client_id() const;
		int& sender_id() const;
		Request_id& request_id();
		char* command(int &len);
		bool is_read_only() const;
		void display() const;
		virtual Digest& digest() const { exit(1); }

		seq_t& n() { return rep().n; }
		uint64_t& index() { return rep().index; }
		seq_t& responseN() { return rep().responseN; }
		bool isValueNull() { return rep().isNull; }
	protected:
		M_PrepareResponse_rep &rep() const;
};

inline Request_id &M_PrepareResponse::request_id()
{
	return rep().rid;
}

inline char *M_PrepareResponse::command(int &len)
{
	len = rep().command_size;
	return contents() + sizeof(M_PrepareResponse_rep);
}

inline bool M_PrepareResponse::is_read_only() const
{
	return rep().extra & 1;
}

inline int& M_PrepareResponse::client_id() const {
	return rep().cid;
}

inline int& M_PrepareResponse::sender_id() const {
	return rep().sender_id;
}

#endif // _M_PrepareResponse_h
