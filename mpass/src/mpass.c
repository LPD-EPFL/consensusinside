/**
 * @author Aleksandar Dragojevic aleksandar.dragojevic@epfl.ch
 *
 */

#include <stdio.h>

#include "test/test_queue_seq.h"
#include "test/test_queue_busy_wait.h"
#include "test/test_queue_pthread_wait.h"
#include "test/test_dynamic_array.h"

//Added by Maysam Yabandeh
#include "test/test_maysam_shm.h"

int main(int argc, char **argv) {
	printf("Starting mpass...\n");

//	test_queue_seq();
//	test_queue_busy_wait();
//	test_queue_pthread_wait();

    test_dynamic_array();

//for the first process do not send an arg and for the second do
    test_shm(argc);
	
	printf("Ending mpass...\n");
	return 0;
}


