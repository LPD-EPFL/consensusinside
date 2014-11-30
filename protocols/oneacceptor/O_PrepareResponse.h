#ifndef _O_PrepareResponse_h
#define _O_PrepareResponse_h 1

#include "O_Message.h"
#include "types.h"
#include "Digest.h"

#include "O_Request.h"
#include "O_AcceptedProposals.h"
#include <map>
using namespace std;

//class Digest;
class O_Principal;


//
// PrepareResponse messages have the following format.
//
struct O_PrepareResponse_rep : public O_Message_rep
{
		short command_size;
		seq_t n;
		uint64_t index;
		int sender_id; // unique id of the forwarder
		AcceptedProposals acceptedProposals;

		// Followed a command which is "command_size" bytes long and an
		// authenticator.
		// Digest resp_digest;
		
};

class O_PrepareResponse : public O_Message
{
	public:
		O_PrepareResponse();
		O_PrepareResponse(O_PrepareResponse_rep *r);
		O_PrepareResponse(uint64_t index, seq_t n, AcceptedProposals& acc);
		// Effects: Creates a new PrepareResponse message with an empty
		// command and no authentication. The methods store_command and
		// authenticate should be used to finish message construction.
		// "rr" is the identifier of the replica from which the client
		// expects a full reply (if negative, client expects a full reply
		// from all replicas).

		virtual ~O_PrepareResponse();

		static const int big_req_thresh = 8000; // Maximum size of not-big requests

		char* store_command(int &max_len);
		// Effects: Returns a pointer to the location within the message
		// where the command should be stored and sets "max_len" to the number of
		// bytes available to store the reply. The caller can copy any command
		// with length less than "max_len" into the returned buffer.

		int& n() const;
		int& sender_id() const;
		uint64_t& index() const;
		AcceptedProposals& acceptedProposals() const;

		char* command(int &len);

		bool is_read_only() const;

		static bool convert(O_Message *m1, O_PrepareResponse *&m2);

		void display() const;

		virtual Digest& digest() const { exit(1); }

	private:
		O_PrepareResponse_rep &rep() const;
		// Effects: Casts "msg" to a PrepareResponse_rep&
};

inline char *O_PrepareResponse::command(int &len)
{
	len = rep().command_size;
	return contents() + sizeof(O_PrepareResponse_rep);
}

inline bool O_PrepareResponse::is_read_only() const
{
	return rep().extra & 1;
}

inline int& O_PrepareResponse::sender_id() const {
	return rep().sender_id;
}

inline seq_t& O_PrepareResponse::n() const {
	return rep().n;
}

inline uint64_t& O_PrepareResponse::index() const {
	return rep().index;
}

inline AcceptedProposals& O_PrepareResponse::acceptedProposals() const {
	return rep().acceptedProposals;
}

#endif // _O_PrepareResponse_h
