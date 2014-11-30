#ifndef _T_Commit_h
#define _T_Commit_h 1

#include "T_Message.h"
#include "types.h"
#include "Digest.h"

class T_Principal;

//
// Commit messages have the following format.
//
struct T_Commit_rep : public T_Message_rep
{
		int sender; // unique id of client who sends the request
		Request_id rid; // unique request identifier
};

class T_Commit : public T_Message
{
		//
		// Commit messages:
		//
		// Requires: Commits that may have been allocated by library users
		// through the libbyz.h interface can not be trimmed (this could free
		// memory the user expects to be able to use.)
		//
	public:
		T_Commit(T_Commit_rep *r);
		T_Commit(Request_id r);
		T_Commit();

		virtual ~T_Commit();

		static bool convert(T_Message *m1, T_Commit *&m2);

		inline Request_id &request_id() const;
		int sender_id() const;

		void display() const;

		virtual Digest& digest() const { exit(1); }

	private:
		T_Commit_rep &rep() const;
		// Effects: Casts "msg" to a Commit_rep&

		//Added by Maysam Yabandeh
		//TODO: Clean this dirty improvization
		friend class T_Replica;
};

inline Request_id &T_Commit::request_id() const
{
	return rep().rid;
}

inline int T_Commit::sender_id() const {
	return rep().sender;
}

#endif // _T_Commit_h
