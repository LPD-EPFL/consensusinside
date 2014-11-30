#ifndef _T_CommitAck_h
#define _T_CommitAck_h 1

#include "T_Message.h"
#include "types.h"
#include "Digest.h"

class T_Principal;

//
// CommitAck messages have the following format.
//
struct T_CommitAck_rep : public T_Message_rep
{
		int sender; // unique id of the sender
		Request_id rid; // unique request identifier
};

class T_CommitAck : public T_Message
{
		//
		// CommitAck messages:
		//
		// Requires: CommitAcks that may have been allocated by library users
		// through the libbyz.h interface can not be trimmed (this could free
		// memory the user expects to be able to use.)
		//
	public:
		T_CommitAck(T_CommitAck_rep *r);
		T_CommitAck(Request_id r);
		T_CommitAck();

		virtual ~T_CommitAck();

		static bool convert(T_Message *m1, T_CommitAck *&m2);

		inline Request_id &request_id() const;
		int sender_id() const;
		int id() const;
		bool match(T_CommitAck *r) {return rep().rid == r->rep().rid;}
		bool full() const {return true;}
		bool verify() {return true;}

		void display() const;

		virtual Digest& digest() const { exit(1); }

	private:
		T_CommitAck_rep &rep() const;
		// Effects: Casts "msg" to a CommitAck_rep&

		//Added by Maysam Yabandeh
		//TODO: Clean this dirty improvization
		friend class T_Replica;
};

inline Request_id &T_CommitAck::request_id() const
{
	return rep().rid;
}

inline int T_CommitAck::sender_id() const {
	return rep().sender;
}

inline int T_CommitAck::id() const
{
	return rep().sender;
}
#endif // _T_CommitAck_h
