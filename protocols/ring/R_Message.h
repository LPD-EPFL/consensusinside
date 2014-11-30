#ifndef _R_Message_h
#define _R_Message_h 1

#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include "th_assert.h"
#include "types.h"
#include "R_Message_tags.h"
#include "Traces.h"

const short R_MESSAGE_NORMAL = 0;
const short R_MESSAGE_BATCH = 4;
const short R_MESSAGE_PIGGYBACKED = 8;
const short R_MESSAGE_ACK_BATCH = 16;

#define ALIGNMENT 8

// bool ALIGNED(void *ptr) or bool ALIGNED(long sz)
// Effects: Returns true iff the argument is aligned to ALIGNMENT
#define ALIGNED(ptr) (((long)(ptr))%ALIGNMENT == 0)

// int ALIGNED_SIZE(int sz)
// Effects: Increases sz to the least multiple of ALIGNMENT greater
// than size.
#define ALIGNED_SIZE(sz) ((ALIGNED(sz)) ? (sz) : (sz)-(sz)%ALIGNMENT+ALIGNMENT)

// Maximum message size. Must verify ALIGNED_SIZE.
const int R_Max_message_size = 16000;

//
// All messages have the following format:
//
struct R_Message_rep
{
	short tag;
	short extra; // May be used to store extra information.
	int size; // Must be a multiple of 8 bytes to ensure proper
	uint32_t unused;
	// alignment of embedded messages.
	//Followed by char payload[size-sizeof(R_Message_rep)];
};

class R_Message
{
	//
	// Generic messages
	//
public:
	R_Message(unsigned sz=0);
	// Effects: Creates an untagged R_Message object that can hold up
	// to "sz" bytes and holds zero bytes. Useful to create message
	// buffers to receive messages from the network.

	~R_Message();
	// Effects: Deallocates all storage associated with this message.

	void trim();
	// Effects: Deallocates surplus storage.

	char* contents();
	// Effects: Return a byte string with the message contents.

	int size() const;
	// Effects: Fetches the message size.

	int tag() const;
	// Effects: Fetches the message tag.

	bool has_tag(int t, int sz) const;
	// Effects: If message has tag "t", its size is greater than "sz",
	// its size less than or equal to "max_size", and its size is a
	// multiple of ALIGNMENT, returns true.  Otherwise, returns false.

	// Message-specific heap management operators.
	void *operator new(size_t s);
	void operator delete(void *x, size_t s);

	int msize() const;
	// Effects: Fetches the maximum number of bytes that can be stored in
	// this message.

	void set_size(int size);
	// Effects: Sets message size to the smallest multiple of 8 bytes
	// greater than equal to "size" and pads the message with zeros
	// between "size" and the new message size. Important to ensure
	// proper alignment of embedded messages.

protected:
	R_Message(int t, unsigned sz);
	// Effects: Creates a message with tag "t" that can hold up to "sz"
	// bytes. Useful to create messages to send to the network.

	R_Message(R_Message_rep *contents);
	// Requires: "contents" contains a valid R_Message_rep.
	// Effects: Creates a message from "contents". No copy is made of
	// "contents" and the storage associated with "contents" is not
	// deallocated if the message is later deleted. Useful to create
	// messages from reps contained in other messages.

	R_Message_rep *msg; // Pointer to the contents of the message.
	int max_size; // Maximum number of bytes that can be stored in "msg"
	// or "-1" if this instance is not responsible for
	// deallocating the storage in msg.
	// Invariant: max_size <= 0 || 0 < msg->size <= max_size

private:
};

// Methods inlined for speed

inline int R_Message::size() const
{
	return msg->size;
}

inline int R_Message::tag() const
{
	return msg->tag;
}

inline bool R_Message::has_tag(int t, int sz) const
{
	if (max_size >= 0&& msg->size > max_size)
	{
		return false;
	}

	if (!msg || msg->tag != t || msg->size < sz || !ALIGNED(msg->size))
	{
		return false;
	}
	return true;
}

inline void* R_Message::operator new(size_t s)
{
	void *ret = (void*)malloc(ALIGNED_SIZE(s));
	th_assert(ret != 0, "Ran out of memory\n");
	return ret;
}

inline void R_Message::operator delete(void *x, size_t s)
{
	if (x != 0)
	{
		free((char*)x);
	}
}

inline int R_Message::msize() const
{
	return (max_size >= 0) ? max_size : msg->size;
}

inline char *R_Message::contents()
{
	return (char *)msg;
}

#endif //_R_Message_h
