/**
 * @author Aleksandar Dragojevic aleksandar.dragojevic@epfl.ch
 *
 */

#include "../common/alloc.h"
#include "../common/lock.h"
#include "../common/constants.h"

#include "connection.h"

extern "C" {
    mpass_multicast_group mpass_bcast_group;
}

///////////////
// functions //
///////////////

extern "C" {

void mpass_global_init() {
    mpass::msg_epnt_glock.init();
    //mpass::msg_end_point_array.init();
    mpass::bcast_group_impl.rcv_array.init();
    mpass_bcast_group = &mpass::bcast_group_impl;
}

mpass_address mpass_create_message_end_point(unsigned this_idx) {
    mpass::msg_epnt_glock.lock();

    // allocate message end point data
    //unsigned this_idx = mpass::msg_end_point_array.append();
    mpass_address addr = (mpass_address)this_idx;
    //mpass::MessageEndPoint *mepnt = &mpass::msg_end_point_array[this_idx];
    mpass::MessageEndPoint *mepnt = &mpass::msg_end_point;
    mepnt->addr = addr;
    mepnt->conn_array.init();

    // create connections between this and all other endpoints
    for(unsigned i = 0;i < mpass::MAX_NUM_PROCESSES;i++) {
        // allocate two queues
		  uint32_t shmfd; //just a unique id in the OS, filled by any of the following calls
        mpass::MessageQueue *snd_q = new(shmfd, this_idx,i) mpass::MessageQueue();
        mpass::MessageQueue *rcv_q = (mpass::MessageQueue *) mpass::MessageQueue::createShmObj(sizeof(mpass::MessageQueue),shmfd,this_idx,i);

        // initialize connection from me to other end point
        mepnt->conn_array.append();
        mepnt->conn_array[i].snd_q = snd_q;
        mepnt->conn_array[i].rcv_q = rcv_q;

        // initialize connections from other end point to me
        //mpass::MessageEndPoint *other = &mpass::msg_end_point_array[i];
        //other->conn_array.append();
        //other->conn_array[this_idx].snd_q = rcv_q;
        //other->conn_array[this_idx].rcv_q = snd_q;
    }

	 // //Added by Maysam Yabandeh
	 // No need any more since I add the i_i connection in the loop above
    // add connection from me to me
    //{
        //mpass::MessageQueue *q = new mpass::MessageQueue();
        //mepnt->conn_array.append();
        //mepnt->conn_array[this_idx].snd_q = q;
        //mepnt->conn_array[this_idx].rcv_q = q;
    //}

    // add this address to the broadcast group
    mpass_join_multicast_group(mpass_bcast_group, addr);

    mpass::msg_epnt_glock.release();

    return addr;
}

// TODO: for now this is a very simple implementation that will copy everything
//       from the buffer regardless of message size
void mpass_send(void *msg, size_t size, mpass_address sender, mpass_address receiver) {
    void *buf = malloc(size);
    memcpy(buf, msg, size);
    mpass::Connection *conn = mpass::get_connection(sender, receiver);
    mpass::simple_msg to_snd(buf, size);
    conn->snd_q->enqueue(mpass::copy_msg_to_queue, &to_snd);
}

bool mpass_receive(void *msg, size_t *size, mpass_address *sender, mpass_address receiver) {
    mpass::MessageEndPoint *end_point = mpass::get_end_point(receiver);
    unsigned rcv_size = end_point->conn_array.get_size();

    for(unsigned i = 0;i < rcv_size;i++) {
        mpass::Connection *conn = &end_point->conn_array[i];

        if(!conn->rcv_q->is_empty()) {
            mpass::simple_msg msg_buf(msg, 0);
            conn->rcv_q->dequeue(mpass::copy_msg_from_queue, &msg_buf);
            *size = msg_buf.size;
            *sender = i;
            return true;
        }
    }

    return false;
}

mpass_multicast_group mpass_create_multicast_group() {
    mpass::MulticastGroup *ret = new mpass::MulticastGroup();
    ret->rcv_array.init();
    return ret;
}

void mpass_join_multicast_group(mpass_multicast_group mcast_group, mpass_address addr) {
    mpass::MulticastGroup *mg = (mpass::MulticastGroup *)mcast_group;
    unsigned idx = mg->rcv_array.append();
    mg->rcv_array[idx] = addr;
}

// very simple implementation of multicast now
// we can take advantage of multiprocessor configuration here
void mpass_send_mcast(void *msg, size_t size, mpass_address sender, mpass_multicast_group mcast_group) {
    mpass::MulticastGroup *mg = (mpass::MulticastGroup *)mcast_group;
    unsigned group_size = mg->rcv_array.get_size();

    for(unsigned i = 0;i < group_size;i++) {
        mpass_send(msg, size, sender, mg->rcv_array[i]);
    }
}

} // extern "C"

// local functions
inline void mpass::copy_msg_to_queue(mpass::CacheLine *item, void *data) {
    simple_msg *q_msg = (simple_msg *)item;
    simple_msg *snd_msg = (simple_msg *)data;
    q_msg->data = snd_msg->data;
    q_msg->size = snd_msg->size;
}

inline mpass::Connection *mpass::get_connection(mpass_address sender, mpass_address receiver) {
    //return &msg_end_point_array[sender].conn_array[receiver];
	 //Added by Maysam Yabandeh
	 //assert(sender==me);
    return &msg_end_point.conn_array[receiver];
}

inline mpass::MessageEndPoint *mpass::get_end_point(mpass_address address) {
    //return &msg_end_point_array[address];
	 //Added by Maysam Yabandeh
	 //assert(address==me);
    return &msg_end_point;
}

inline void mpass::copy_msg_from_queue(mpass::CacheLine *item, void *data) {
    simple_msg *q_msg = (simple_msg *)item;
    simple_msg *rcv_msg = (simple_msg *)data;
    rcv_msg->size = q_msg->size;
    memcpy(rcv_msg->data, q_msg->data, rcv_msg->size);
    free(q_msg->data);
}

