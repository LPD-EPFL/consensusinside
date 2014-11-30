#ifndef _M_Abandon_h
#define _M_Abandon_h 1

#include "M_Message.h"
#include "types.h"
#include "Digest.h"

//class Digest;
class M_Principal;


//
// Abandon messages have the following format.
//
struct M_Abandon_rep : public M_Message_rep
{
		short command_size;
		seq_t n;
		int sender_id; // unique id of the forwarder
		// Followed a command which is "command_size" bytes long and an
		// authenticator.
		// Digest resp_digest;
};

class M_Abandon : public M_Message
{
	public:
		M_Abandon(M_Abandon_rep *r);
		M_Abandon(seq_t);
		M_Abandon();
		// Effects: Creates a new Abandon message with an empty
		// command and no authentication. The methods store_command and
		// authenticate should be used to finish message construction.
		// "rr" is the identifier of the replica from which the client
		// expects a full reply (if negative, client expects a full reply
		// from all replicas).

		virtual ~M_Abandon();

		static const int big_req_thresh = 8000; // Maximum size of not-big requests

		char* store_command(int &max_len);
		// Effects: Returns a pointer to the location within the message
		// where the command should be stored and sets "max_len" to the number of
		// bytes available to store the reply. The caller can copy any command
		// with length less than "max_len" into the returned buffer.

		int& n() const;
		int& sender_id() const;

		char* command(int &len);

		bool is_read_only() const;

		static bool convert(M_Message *m1, M_Abandon *&m2);

		void display() const;

		virtual Digest& digest() const { exit(1); }

	private:
		M_Abandon_rep &rep() const;
		// Effects: Casts "msg" to a Abandon_rep&
};

inline char *M_Abandon::command(int &len)
{
	len = rep().command_size;
	return contents() + sizeof(M_Abandon_rep);
}

inline bool M_Abandon::is_read_only() const
{
	return rep().extra & 1;
}

inline int& M_Abandon::sender_id() const {
	return rep().sender_id;
}

inline seq_t& M_Abandon::n() const {
	return rep().n;
}

#endif // _M_Abandon_h
