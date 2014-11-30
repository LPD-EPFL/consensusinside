/**
 * @author Aleksandar Dragojevic aleksandar.dragojevic@epfl.ch
 *
 */

#include <stdio.h>

#include "../common/dynamic_array.h"

void test_dynamic_array() {
    mpass::DynamicArray<int, 8> array;
    array.init();

    for(int i = 0;i < 30;i++) {
        printf("%u ", array.append());
    }

    printf("\n");
    printf("%d  ", array[14]);

    array[14] = 100;

    printf("%d\n", array[14]);

    array.destroy();
}

