#ifndef _M_Learn_h
#define _M_Learn_h 1

#include "M_Message.h"
#include "types.h"
#include "Digest.h"

#include "M_Certificate.h"

//class Digest;
class M_Principal;


//
// Request messages have the following format.
//
struct M_Learn_rep : public M_Message_rep
{
		//Digest od; // Digest of rid, cid, command.
		short command_size;
		int cid; // unique id of client who sends the request
		int sender_id; // unique id of the forwarder
		Request_id rid; // unique request identifier
		//Learn & Learn
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
//It still uses M_Learn_rep but the public method allow access for more vars
class M_Learn : public M_Message
{
	public:
		M_Learn() : M_Message(M_Learn_tag, sizeof(M_Learn_rep)) {
			rep().command_size = 0;
			set_size(sizeof(M_Learn_rep));
		}
	   //No other constructor is necessary
		//must use the received M_Learn message
		
		static bool convert(M_Message *m1, M_Learn *&m2);

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
	protected:
		M_Learn_rep &rep() const;
};

inline Request_id &M_Learn::request_id()
{
	return rep().rid;
}

inline char *M_Learn::command(int &len)
{
	len = rep().command_size;
	return contents() + sizeof(M_Learn_rep);
}

inline bool M_Learn::is_read_only() const
{
	return rep().extra & 1;
}

inline int& M_Learn::client_id() const {
	return rep().cid;
}

inline int& M_Learn::sender_id() const {
	return rep().sender_id;
}

#endif // _M_Learn_h
