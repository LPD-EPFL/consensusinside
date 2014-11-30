#include <stdlib.h>
#include <stdio.h>
#include <string.h>

//Added by Maysam Yabandeh
#include <unistd.h>


#include "../mpass/connection.h"


void rcv_message2(mpass_address addr);

void test_shm(int type) {
    printf("test infrastructure start...\n");
mpass_address addr_1, addr_2;//, addr_3, addr_4;
//mpass_multicast_group mcast_group_123;

    // init the library
    mpass_global_init();

    // messages
    char *msg_1 = (char *)"message 1";
    char *msg_2 = (char *)"message 2";
	 char *msg;
    //char *msg_3 = (char *)"message 3";

    // create 4 end_points
	 if (type == 1)
	 {
		 addr_1 = mpass_create_message_end_point(1);
		 addr_2 = 2;
		 msg = msg_1;
	 }
	 else 
	 {
		 addr_1 = mpass_create_message_end_point(2);
		 addr_2 = 1;
		 msg = msg_2;
	 }

	 sleep(5);
	 printf("type is %d\n", type);

    // send a msg_1 from addr_1 to addr_2
	 if (type == 1)
    mpass_send(msg, strlen(msg) + 1, addr_1, addr_2);
    rcv_message2(addr_2);

    printf("test infrastructure end...\n");
}

void rcv_message2(mpass_address addr) {
    char rcv_buf[512];
    size_t rcv_size;
    mpass_address rcv_sender;

    if(mpass_receive(rcv_buf, &rcv_size, &rcv_sender, addr)) {
        printf("end point [%d] received message [%s] from [%d]\n", addr, rcv_buf, rcv_sender);
    }
}
