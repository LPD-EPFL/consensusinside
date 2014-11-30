#ifndef _M_Prepare_h
#define _M_Prepare_h 1

#include "M_Message.h"
#include "types.h"
#include "Digest.h"

//class Digest;
class M_Principal;


//
// Prepare messages have the following format.
//
struct M_Prepare_rep : public M_Message_rep
{
		short command_size;
		seq_t n;
		uint64_t index;
		int sender_id; // unique id of the forwarder

		// Followed a command which is "command_size" bytes long and an
		// authenticator.
		// Digest resp_digest;
		
};

class M_Prepare : public M_Message
{
	public:
		M_Prepare();
		M_Prepare(M_Prepare_rep *r);
		M_Prepare(uint64_t _index, seq_t _globaln);
		// Effects: Creates a new Prepare message with an empty
		// command and no authentication. The methods store_command and
		// authenticate should be used to finish message construction.
		// "rr" is the identifier of the replica from which the client
		// expects a full reply (if negative, client expects a full reply
		// from all replicas).

		virtual ~M_Prepare();

		static const int big_req_thresh = 8000; // Maximum size of not-big requests

		char* store_command(int &max_len);
		// Effects: Returns a pointer to the location within the message
		// where the command should be stored and sets "max_len" to the number of
		// bytes available to store the reply. The caller can copy any command
		// with length less than "max_len" into the returned buffer.

		int& n() const;
		int& sender_id() const;
		uint64_t& index() const;

		char* command(int &len);

		bool is_read_only() const;

		static bool convert(M_Message *m1, M_Prepare *&m2);

		void display() const;

		virtual Digest& digest() const { exit(1); }

	private:
		M_Prepare_rep &rep() const;
		// Effects: Casts "msg" to a Prepare_rep&
};

inline char *M_Prepare::command(int &len)
{
	len = rep().command_size;
	return contents() + sizeof(M_Prepare_rep);
}

inline bool M_Prepare::is_read_only() const
{
	return rep().extra & 1;
}

inline int& M_Prepare::sender_id() const {
	return rep().sender_id;
}

inline seq_t& M_Prepare::n() const {
	return rep().n;
}

inline uint64_t& M_Prepare::index() const {
	return rep().index;
}

#endif // _M_Prepare_h
