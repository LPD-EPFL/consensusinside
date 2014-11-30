#ifndef _O_Abandon_h
#define _O_Abandon_h 1

#include "O_Message.h"
#include "types.h"
#include "Digest.h"

//class Digest;
class O_Principal;


//
// Abandon messages have the following format.
//
struct O_Abandon_rep : public O_Message_rep
{
		short command_size;
		seq_t n;
		int sender_id; // unique id of the forwarder
		uint8_t leader_id; //id of the current leader
};

class O_Abandon : public O_Message
{
	public:
		O_Abandon(O_Abandon_rep *r);
		O_Abandon(seq_t, int);
		O_Abandon();
		// Effects: Creates a new Abandon message with an empty
		// command and no authentication. The methods store_command and
		// authenticate should be used to finish message construction.
		// "rr" is the identifier of the replica from which the client
		// expects a full reply (if negative, client expects a full reply
		// from all replicas).

		virtual ~O_Abandon();

		static const int big_req_thresh = 8000; // Maximum size of not-big requests

		char* store_command(int &max_len);
		// Effects: Returns a pointer to the location within the message
		// where the command should be stored and sets "max_len" to the number of
		// bytes available to store the reply. The caller can copy any command
		// with length less than "max_len" into the returned buffer.

		int& n() const;
		int& sender_id() const;
		uint8_t& leader_id() const;

		char* command(int &len);

		bool is_read_only() const;

		static bool convert(O_Message *m1, O_Abandon *&m2);

		void display() const;

		virtual Digest& digest() const { exit(1); }

	private:
		O_Abandon_rep &rep() const;
		// Effects: Casts "msg" to a Abandon_rep&
};

inline char *O_Abandon::command(int &len)
{
	len = rep().command_size;
	return contents() + sizeof(O_Abandon_rep);
}

inline bool O_Abandon::is_read_only() const
{
	return rep().extra & 1;
}

inline int& O_Abandon::sender_id() const {
	return rep().sender_id;
}

inline uint8_t& O_Abandon::leader_id() const {
	return rep().leader_id;
}

inline seq_t& O_Abandon::n() const {
	return rep().n;
}

#endif // _O_Abandon_h
