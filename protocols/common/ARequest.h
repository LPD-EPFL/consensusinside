#ifndef _A_Request_h
#define _A_Request_h 1

#include "Digest.h"
#include "types.h"

class ARequest
{
	// This is a virtual class which will hold necessary interface for dealing with request history.
	// Interface should provide digesting and serialization
public:

   virtual Request_id& request_id()=0;
   // Effects: Fetches the request identifier from the message.

   virtual Digest& digest() const =0;
   // Effects: Returns the digest of the string obtained by
   // concatenating the client_id, the request_id, and the command.

   virtual int& client_id() const =0;
   // Effects: Returns the id of the client who generated the request.

   virtual ~ARequest() {};
};

#endif // _A_Request_h
