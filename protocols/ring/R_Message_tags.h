#ifndef _R_Message_tags_h
#define _R_Message_tags_h 1

//
// Each message type is identified by one of the tags in the set below.
//

const short R_Request_tag=1;
const short R_Reply_tag=2;
const short R_Checkpoint_tag=3;
const short R_ACK_tag=4;
const short R_Deliver_tag=5;

#endif // _R_Message_tags_h
