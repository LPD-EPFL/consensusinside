#ifndef _O_Prepare_h
#define _O_Prepare_h 1

#include "O_Message.h"
#include "types.h"
#include "Digest.h"

//class Digest;
class O_Principal;


//
// Prepare messages have the following format.
//
struct O_Prepare_rep : public O_Message_rep
{
		short command_size;
		seq_t n;
		uint64_t index;
		int sender_id; // unique id of the forwarder

		// Followed a command which is "command_size" bytes long and an
		// authenticator.
		// Digest resp_digest;
		
};

class O_Prepare : public O_Message
{
	public:
		O_Prepare();
		O_Prepare(O_Prepare_rep *r);
		O_Prepare(uint64_t _index, seq_t _globaln);
		// Effects: Creates a new Prepare message with an empty
		// command and no authentication. The methods store_command and
		// authenticate should be used to finish message construction.
		// "rr" is the identifier of the replica from which the client
		// expects a full reply (if negative, client expects a full reply
		// from all replicas).

		virtual ~O_Prepare();

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

		static bool convert(O_Message *m1, O_Prepare *&m2);

		void display() const;

		virtual Digest& digest() const { exit(1); }

	private:
		O_Prepare_rep &rep() const;
		// Effects: Casts "msg" to a Prepare_rep&
};

inline char *O_Prepare::command(int &len)
{
	len = rep().command_size;
	return contents() + sizeof(O_Prepare_rep);
}

inline bool O_Prepare::is_read_only() const
{
	return rep().extra & 1;
}

inline int& O_Prepare::sender_id() const {
	return rep().sender_id;
}

inline seq_t& O_Prepare::n() const {
	return rep().n;
}

inline uint64_t& O_Prepare::index() const {
	return rep().index;
}

#endif // _O_Prepare_h
