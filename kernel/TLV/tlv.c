
/*
 * Create tlv message, serialize and deserialize as unsigned char *buffer
 *
 * Author: Victor From <c13vfm@cs.umu.se>
 * 
 *
 *
 */

#include <linux/kernel.h>
#include <linux/module.h>
#include "tlv.h"
#include <asm/types.h>
#include <linux/slab.h>
#include <linux/string.h>

#define INT8_SIZE   1
#define INT16_SIZE  2
#define INT32_SIZE  4

DATATYPES type_string = TLV_STRING;
DATATYPES type_int    = TLV_INTEGER;
DATATYPES type_instr  = TLV_INSTR;



int byte_counter;

int32_t tlv_add_integer(struct TLV_holder* holders, int32_t number)
{
	return add_raw_tlv(holders, type_int, INT32_SIZE, &number); 
}

int32_t tlv_add_string(struct TLV_holder* holders, const char* string) 
{
	return add_raw_tlv(holders, type_string, strlen(string) + 1, string);
}


/*
* add_raw_tlv fills a TLV_holder.data with given parameters and increments
* TLV_. 
*
* Important, not more then 3 structs
*
*
*/
int32_t add_raw_tlv(struct TLV_holder* holders, const unsigned char type, 
					const int16_t len, const void *data)
{
    int index;    
    
	if(holders->nr_of_structs > 2 || data == NULL || holders == NULL) 
        return tlv_failed;
	
	index  = holders->nr_of_structs++;
	
    holders->tlv_arr[index].type = type;
    holders->tlv_arr[index].len = len;
    holders->tlv_arr[index].data = kmalloc(len + 1, GFP_ATOMIC); 
    
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
            kfree(tlvs->tlv_arr[index].data); //Use kfree or vfree  in kernel       
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
    int32_t tot_bytes  = 0;
    int index = 0;
    
    if(src == NULL || dest == NULL){
        pr_err("[kernel] - Serialize got Null object");
        return tlv_failed;    
    }
    
    for(; index < src->nr_of_structs; index++) {
        dest[tot_bytes] = src->tlv_arr[index].type;
        tot_bytes += INT8_SIZE;
        
        memcpy(&dest[tot_bytes], &src->tlv_arr[index].len, INT16_SIZE);
        tot_bytes += INT16_SIZE;

        memcpy(&dest[tot_bytes], src->tlv_arr[index].data, 
                                                    src->tlv_arr[index].len);
        tot_bytes += src->tlv_arr[index].len;
    }
    *byte_counter = tot_bytes;
    
    return tlv_success;
}

/*
* deserializeing a byte array to a TLV struct.
*
*/
int32_t deserialize_tlv(struct TLV_holder *dest, unsigned char* src, 
                            int tot_bytes)
{

    byte_counter = 0;

    if(dest->tlv_arr[dest->nr_of_structs].len) {
        pr_info("nothing to parse");
        return -1;
    }

    while(byte_counter < tot_bytes) {   
        if(dest->nr_of_structs > MAX_OBJS - 1) 
            return tlv_failed;        
        
        dest->tlv_arr[dest->nr_of_structs].type = src[byte_counter];
        byte_counter += INT8_SIZE;
        
        
        memcpy(&dest->tlv_arr[dest->nr_of_structs].len, &src[byte_counter], 
                                                                    INT16_SIZE);
        byte_counter += INT16_SIZE;

        if(dest->tlv_arr[dest->nr_of_structs].len != 0) {
            dest->tlv_arr[dest->nr_of_structs].data = 
                    kmalloc(dest->tlv_arr[dest->nr_of_structs].len, GFP_ATOMIC);
    
            memcpy(dest->tlv_arr[dest->nr_of_structs].data, &src[byte_counter], 
                    dest->tlv_arr[dest->nr_of_structs].len);
        
            byte_counter += dest->tlv_arr[dest->nr_of_structs].len;
        } else {
            //TODO handle
        }
        
        dest->nr_of_structs++;
    }

    return tlv_success;
}

/* 
* Print function for checking TLV attributes.
*/
int32_t print_tlv(struct TLV_holder *src) {
    int temp;
    int index = 0;    
    
    pr_info("--------------> In print_tlv <----------------\n");   
    while (index < src->nr_of_structs) {
        
        if(src->tlv_arr[index].type == type_int || 
           src->tlv_arr[index].type == type_instr) {
            pr_info("type is: %d\n", (int)src->tlv_arr[index].type);
            memcpy(&temp, src->tlv_arr[index].data, sizeof(int32_t));
            pr_info("data is: %d\n", temp);
        } else if(src->tlv_arr[index].type == type_string) {
            pr_info("type is: %d\n", (int)src->tlv_arr[index].type);
            pr_info("len is: %d\n", (int)src->tlv_arr[index].len);
            pr_info("data: %s\n", (char*)src->tlv_arr[index].data);
        } else if(src->tlv_arr[index].type == type_instr){
            pr_info("this <-- is not a valid data type");
        }
              
        index++;
    }
	pr_info("number of structs: %d\n", index);
    return tlv_success;
}
