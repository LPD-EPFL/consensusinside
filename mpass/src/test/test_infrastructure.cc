/**
 * @author Aleksandar Dragojevic aleksandar.dragojevic@epfl.ch
 *
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "../mpass/connection.h"

mpass_address addr_1, addr_2, addr_3, addr_4;
mpass_multicast_group mcast_group_123;

void rcv_message(mpass_address addr);

void test_infrastructure() {
    printf("test infrastructure start...\n");

    // init the library
    mpass_global_init();

    // create 4 end_points
    addr_1 = mpass_create_message_end_point(1);
    addr_2 = mpass_create_message_end_point(2);
    addr_3 = mpass_create_message_end_point(3);
    addr_4 = mpass_create_message_end_point(4);

    mcast_group_123 = mpass_create_multicast_group();
    mpass_join_multicast_group(mcast_group_123, addr_1);
    mpass_join_multicast_group(mcast_group_123, addr_2);
    mpass_join_multicast_group(mcast_group_123, addr_3);

    // messages
    char *msg_1 = (char *)"message 1";
    char *msg_2 = (char *)"message 2";
    char *msg_3 = (char *)"message 3";

    // send a msg_1 from addr_1 to addr_2
    mpass_send(msg_1, strlen(msg_1) + 1, addr_1, addr_2);
    rcv_message(addr_2);

    // broadcast msg_2 from addr_3
    mpass_send_mcast(msg_2, strlen(msg_2) + 1, addr_3, mpass_bcast_group);
    rcv_message(addr_1);
    rcv_message(addr_2);
    rcv_message(addr_3);
    rcv_message(addr_4);

    // multicast msg_3 from addr_4 to mcast_123
    mpass_send_mcast(msg_3, strlen(msg_3) + 1, addr_4, mcast_group_123);
    rcv_message(addr_1);
    rcv_message(addr_2);
    rcv_message(addr_3);
    rcv_message(addr_4);

    printf("test infrastructure end...\n");
}

void rcv_message(mpass_address addr) {
    char rcv_buf[512];
    size_t rcv_size;
    mpass_address rcv_sender;

    if(mpass_receive(rcv_buf, &rcv_size, &rcv_sender, addr)) {
        printf("end point [%d] received message [%s] from [%d]\n", addr, rcv_buf, rcv_sender);
    }
}
