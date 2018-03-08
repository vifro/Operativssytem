/*
* File for serializing and deserializing a tlv obj
*
*/

#include "tlv.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define INT8_SIZE   1
#define INT16_SIZE  2
#define INT32_SIZE  4

datatypes type_string = TLV_STRING;
datatypes type_int    = TLV_INTEGER;
datatypes type_instr  = TLV_INSTR;

int32_t tlv_add_instruction(struct TLV_holder* holders, int32_t number)
{
    printf("add_integer with type %d\n", type_instr);
    printf("size: %d\n", INT32_SIZE);
    printf("integer: %d\n", number);

	return add_raw_tlv(holders, type_instr, INT32_SIZE, &number); 
}

int32_t tlv_add_integer(struct TLV_holder* holders, int32_t number)
{
    printf("add_integer with type %d\n", type_int );
    printf("size: %d\n", INT32_SIZE);
    printf("integer: %d\n", number);
	return add_raw_tlv(holders, type_int, INT32_SIZE, &number); 
}

int32_t tlv_add_string(struct TLV_holder* holders, const char* string) 
{
    printf("add_string with type %d\n", type_string );
    printf("size: %ld\n", strlen(string) + 1);
    printf("string: %s\n", string);
	return add_raw_tlv(holders, type_string, strlen(string) + 1, string);
}

/*
* add_raw_tlv fills a TLV_holder.data with given parameters and increments
* TLV_. 
*
*
*
*
*/
int32_t add_raw_tlv(struct TLV_holder* holders, const unsigned char type, 
					const int16_t len, const void *data)
{
	if(holders->nr_of_structs > 1 || data == NULL || holders == NULL) 
        return tlv_failed;
    
    int index = holders->nr_of_structs++; 
    
    if(index  > 1) 
        return tlv_failed;

    holders->tlv_arr[index].type = type;
    holders->tlv_arr[index].len = len;
    holders->tlv_arr[index].data = malloc(len); // Change to kmalloc in kernel, GFP_ATOMIC!!!!
    
    memcpy(holders->tlv_arr[index].data, data, len); 
    
    return tlv_success;
}

/*
* TLV.data is a pointer which is har to be free'd if allocated 
*
*/
int32_t free_tlv(struct TLV_holder *tlvs)
{
    int index = 0;
    
    while(index < tlvs->nr_of_structs) {
        if(tlvs->tlv_arr[index].data != NULL)
            free(tlvs->tlv_arr[index].data); //Use kfree or vfree  in kernel       
        index++;
    }
    return tlv_success;
}

/*
* Serializing a TLV struct to a array of bytes.  
*
*/

int32_t serialize_tlv(struct TLV_holder *src, unsigned char* dest, 
                        int *byte_counter)
{
    printf("\n\n------> IN SERIALIZE <----------\n");
    //TODO check if src of dest is null
    int32_t tot_bytes  = 0;
    int index = 0;
    printf("nr_of_structs is: %d\n", src->nr_of_structs);
    for(; index < src->nr_of_structs; index++) {
        printf("index is: %d\n", index);
        dest[tot_bytes] = src->tlv_arr[index].type;
        tot_bytes += INT8_SIZE;
        
        memcpy(&dest[tot_bytes], &src->tlv_arr[index].len, INT16_SIZE);
        tot_bytes += INT16_SIZE;

        memcpy(&dest[tot_bytes], src->tlv_arr[index].data, 
                                            src->tlv_arr[index].len);
        tot_bytes += src->tlv_arr[index].len;
        
 
    }
    printf("total bytes is: %d\n",tot_bytes);
    *byte_counter = tot_bytes;
    
    return tlv_success;
}
/*
* 
* deserializeing a byte array to a TLV struct.
*
*/
int32_t deserialize_tlv(struct TLV_holder *dest, unsigned char* src, 
                            int tot_bytes)
{
    //TODO Check if src and dest is null
    printf("\n\n------> IN deSERIALIZE <----------\n"); 
    printf("tot_bytes = %d\n", tot_bytes );
    
    int byte_counter = 0;

    while(byte_counter < tot_bytes) {
        printf("byte counter is %d, and tot bytes is %d \n", 
                                                    byte_counter, tot_bytes);

        printf("dest->nr_of_structs = %d\n", dest->nr_of_structs);       
        if(dest->nr_of_structs > MAX_OBJS - 1) 
            return tlv_failed;        
        
        dest->tlv_arr[dest->nr_of_structs].type = src[byte_counter];
        byte_counter += INT8_SIZE;
        printf("byte counter is %d\n", byte_counter);
        
        memcpy(&dest->tlv_arr[dest->nr_of_structs].len, 
                                                &src[byte_counter], INT16_SIZE);
        byte_counter += INT16_SIZE;
        printf("byte counter is %d\n", byte_counter);

        if(dest->tlv_arr[dest->nr_of_structs].len != 0) {
            dest->tlv_arr[dest->nr_of_structs].data = 
                                malloc(dest->tlv_arr[dest->nr_of_structs].len);
            memcpy(dest->tlv_arr[dest->nr_of_structs].data, &src[byte_counter], 
                    dest->tlv_arr[dest->nr_of_structs].len);
        
            byte_counter += dest->tlv_arr[dest->nr_of_structs].len;
            printf("byte counter is %d\n", byte_counter);
        } else {
            dest->tlv_arr[dest->nr_of_structs].data == NULL;
        }
        
        dest->nr_of_structs++;
    }

    return tlv_success;
}


/* 
* Print function for checking TLV attributes.
*/
int32_t print_tlv(struct TLV_holder *src) {
        
    int index = 0;    
    printf("\n\n--------------> In print_tlv <----------------\n"); 
       
    while (index < src->nr_of_structs) {
        printf("index is: %d\n", index);
        //printf("obj_nr: %d\n", index); 
        //printf("size: %d\n", &src->tlv_arr[index].len);
        if(src->tlv_arr[index].type == type_int || 
           src->tlv_arr[index].type == type_instr) {
            printf("type is: %d\n", (int)src->tlv_arr[index].type);
            int temp;
            memcpy(&temp, src->tlv_arr[index].data, sizeof(int32_t));
            printf("data is: %d\n", temp);
        } else if(src->tlv_arr[index].type == type_string) {
            printf("type is: %d\n", (int)src->tlv_arr[index].type);
            printf("len is: %d\n", (int)src->tlv_arr[index].len);
            printf("data: %s\n", (char*)src->tlv_arr[index].data);
        } else {
            printf("this <-- is not a valid data type");
        }      
        index++;
    }
    return tlv_success;
}





