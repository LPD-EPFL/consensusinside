#ifndef _T_Ack_h
#define _T_Ack_h 1

#include "T_Message.h"
#include "types.h"
#include "Digest.h"

class T_Principal;

//
// Ack messages have the following format.
//
struct T_Ack_rep : public T_Message_rep
{
		int sender; // unique id of client who sends the request
		Request_id rid; // unique request identifier
};

class T_Ack : public T_Message
{
		//
		// Ack messages:
		//
		// Requires: Acks that may have been allocated by library users
		// through the libbyz.h interface can not be trimmed (this could free
		// memory the user expects to be able to use.)
		//
	public:
		T_Ack(T_Ack_rep *r);
		T_Ack(Request_id r);
		T_Ack();

		virtual ~T_Ack();

		static bool convert(T_Message *m1, T_Ack *&m2);

		inline Request_id &request_id() const;
		int sender_id() const;
		int id() const;
		bool match(T_Ack *r) {return rep().rid == r->rep().rid;}
		bool full() const {return true;}
		bool verify() {return true;}

		void display() const;

		virtual Digest& digest() const { exit(1); }

	private:
		T_Ack_rep &rep() const;
		// Effects: Casts "msg" to a Ack_rep&

		//Added by Maysam Yabandeh
		//TODO: Clean this dirty improvization
		friend class T_Replica;
};

inline Request_id &T_Ack::request_id() const
{
	return rep().rid;
}

inline int T_Ack::sender_id() const {
	return rep().sender;
}

inline int T_Ack::id() const
{
	return rep().sender;
}
#endif // _T_Ack_h
