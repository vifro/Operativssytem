#include "tlv.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

int main(int argc, char* argv) {
    struct TLV_holder tlv_holder1, tlv_holder2;
    memset(&tlv_holder1, 0 , sizeof(tlv_holder1));
    memset(&tlv_holder2, 0 , sizeof(tlv_holder2));

    unsigned char buffer[4096] = {0};
    int buffer_length = 0;
    
    tlv_add_integer(&tlv_holder1, 123);
    tlv_add_string(&tlv_holder1, "name Jameson");
    
    printf("Calling serialize.. \n");
    /* Serialize buffer and send to target */
    int i = serialize_tlv(&tlv_holder1, buffer, &buffer_length);
    printf("strlen of buffer is %ld\n", sizeof(buffer)/sizeof(unsigned char));
    
    printf("return value from ser is : %d\n", i);
    print_tlv(&tlv_holder1);
    
    /* Receive buffer and deserialize to tlv */
    int j = deserialize_tlv(&tlv_holder2, buffer, buffer_length);
    printf("return from desentr is: %d\n ", j );
    printf("%ls\n", tlv_holder2.tlv_arr[0].data);
    /* Print tlv_objs */
    print_tlv(&tlv_holder2);

    free_tlv(&tlv_holder1);
    free_tlv(&tlv_holder2);
    
    return tlv_success;
}
