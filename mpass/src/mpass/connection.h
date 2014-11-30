/**
 * @author Aleksandar Dragojevic aleksandar.dragojevic@epfl.ch
 *
 */

#ifndef MPASS_CONNECTION_H_
#define MPASS_CONNECTION_H_

#include <stdint.h>
//Added by Maysam Yabandeh
#include "queue.h"
#include "../common/dynamic_array.h"

//#define maysam_dbg

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */
    
///////////
// types //
///////////

typedef int32_t mpass_address;
typedef void *mpass_multicast_group;

/////////////////
// shared data //
/////////////////

// global broadcast multicast group
extern mpass_multicast_group mpass_bcast_group;

///////////////
// functions //
///////////////

/// This function initializes shared data and should be called before any other function.
void mpass_global_init();

/// This function creates and initializes a mpass_message_end_point and needs to be called before
/// any messages can be sent or received using this particular end_point.
mpass_address mpass_create_message_end_point(unsigned this_idx);

/// Send a message of specified size from a receiver to the sender.
void mpass_send(void *msg, size_t size, mpass_address sender, mpass_address receiver);

/// Receive a message of specified size from any receiver. Returns true if the
/// message was received and false if there is no message.
bool mpass_receive(void *msg, size_t *size, mpass_address *sender, mpass_address receiver);

mpass_multicast_group mpass_create_multicast_group();
void mpass_join_multicast_group(mpass_multicast_group mcast_group, mpass_address addr);
void mpass_send_mcast(void *msg, size_t size, mpass_address sender, mpass_multicast_group mcast_group);

#ifdef __cplusplus
}
#endif /* __cplusplus */


///////////
// types //
///////////

namespace mpass {
    typedef Queue<> MessageQueue;

    // this is a data structure for communicating with
    // another messaging endpoint
    struct Connection {
        MessageQueue *snd_q;
        MessageQueue *rcv_q;
    };

    struct MessageEndPoint {
        mpass_address addr;
        
        // connection to all other message endpoints
        DynamicArray<Connection> conn_array;
    };

    struct MulticastGroup : public MpassAlloced {
        DynamicArray<mpass_address> rcv_array;
    };

    struct simple_msg {
        simple_msg(void *d, size_t s) : data(d), size(s) {
            // do nothing
        }

        void *data;
        size_t size;
    };

     void copy_msg_to_queue(mpass::CacheLine *item, void *data);
     void copy_msg_from_queue(mpass::CacheLine *item, void *data);

     MessageEndPoint *get_end_point(mpass_address address);
     Connection *get_connection(mpass_address sender, mpass_address receiver);
};


/////////////////
// global data //
/////////////////

namespace mpass {
    //static DynamicArray<MessageEndPoint> msg_end_point_array;
	 //Added by Maysam Yabandeh
	 //In process-replication model, every process needs to keep track of only its own end points
    extern MessageEndPoint msg_end_point;

    extern MulticastGroup bcast_group_impl;

    //extern cas_lock msg_epnt_glock;
};


#endif /* MPASS_CONNECTION_H_ */
