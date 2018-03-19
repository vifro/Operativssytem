#include <linux/kernel.h>
#include <linux/module.h>

#include "../myheader.h"
#include "../TLV/tlv.h"


#include <linux/slab.h>
#include <asm/types.h>
#include <linux/string.h>

#include "../myheader.h"
#include "../TLV/tlv.h"
#include "../KVStore/keyvalue.h"

DATATYPES parse_string = TLV_STRING;
DATATYPES parse_int    = TLV_INTEGER;

#ifndef INSTR_INDEX
#define INSTR_INDEX 0
#endif

enum { TYPE_READ, TYPE_WRITE, TYPE_INSTR = 255 };

int construct_kwstring(struct TLV_holder recieved) {

	char temp_string[200]; 
	int value = -1;
	
	pr_info("[write_to_storage] - dataKey: %s", (char*)recieved.tlv_arr[INSTR_INDEX + 1].data);
    
    memcpy(&value, recieved.tlv_arr[INSTR_INDEX + 2].data, sizeof(int32_t));
    pr_info("[write_to_storage] - data: %d", value);
    
    sprintf(temp_string, "key:%s - value%d",
    					 (char*)recieved.tlv_arr[INSTR_INDEX + 1].data, value);
    pr_info("%s", temp_string);
    
    
    return 0;

}

/* 
 * Given a key and a value. Saves that infromation in a temporary hash table. 
 * If successfully completed, send key and value to a new function. 
 *
 */
int write_to_storage(struct TLV_holder recieved)
{
    char * key, * value;
    int value_len;

    if(recieved.tlv_arr[INSTR_INDEX + 1].type != parse_string
        || recieved.tlv_arr[INSTR_INDEX + 2].type != parse_string)
    {
        pr_info("[write_to_storage] - incorrect key/value field type");

        return tlv_failed;
    }

    key = (char *)recieved.tlv_arr[INSTR_INDEX + 1].data;
    value = (char *)recieved.tlv_arr[INSTR_INDEX + 2].data;
    value_len = recieved.tlv_arr[INSTR_INDEX + 2].len;
    
    // Commented out for the moment, uncomment if necessary!
    //construct_kwstring(recieved);

    pr_info("[write_to_storage] - key: %s", key);
    pr_info("[write_to_storage] - data: %s (%d bytes)", value, value_len);
    kvs_insert(key, value, value_len);
    
    construct_kwstring(recieved);
    
    return tlv_success;
}

/*

 * Given a key values, retrieves the value from storage. 
 */
int read_from_storage(struct TLV_holder recieved) { 
    if(recieved.tlv_arr[INSTR_INDEX + 1].type != parse_string)
    {
        pr_info("[read_from_storage] - incorrect key field type");
        return tlv_failed;
    }
    
    //TODO Return the value from key-value storage

    return tlv_success;
}

/*
 * Check instruction type. THen call appropriate function depending on the
 * message type. 
 */
int check_instr(struct TLV_holder recieved) {
    int incoming_instr;
	
    pr_info("nr: %d - maxObj %d", recieved.nr_of_structs, MAX_OBJS);

    if(recieved.nr_of_structs != MAX_OBJS){
        pr_err("[check_instr] - Not correct number of objs");
        return tlv_failed;
    }

    pr_info("[check_instr] - type is %d", recieved.tlv_arr[INSTR_INDEX].type);
    if(recieved.tlv_arr[INSTR_INDEX].type != TYPE_INSTR) {
        pr_err("[check_instr] - Not a valid instruction\n");
        return tlv_failed;
    }

    if(recieved.tlv_arr[INSTR_INDEX].len != INT32_SIZE) {
        pr_err("[check_instr] - not a valid size \n");
        return tlv_failed;
    }

    memcpy(&incoming_instr, recieved.tlv_arr[INSTR_INDEX].data, sizeof(int32_t));

    switch (incoming_instr) {
        case TYPE_READ:
            pr_info("[check_instr] - READ");

            read_from_storage(recieved);
            break;
        case TYPE_WRITE:
            pr_info("[check_instr] - WRITE" );

            write_to_storage(recieved);
            break;
        default:
            //TODO send error message back.
            return tlv_failed;
    }

    return tlv_success;
}


/*
 * parse_message
 * 	Checks incoming buffer and parses it as a tlv message.    
 */
int parse_tlv_message(int seq, int rec_pid ,unsigned char* buffer, int buf_len) {
    int err;
	struct TLV_holder recieved;
    memset(&recieved, 0 , sizeof(recieved));

    err = deserialize_tlv(&recieved, buffer,  buf_len);
    if(err != 0) {
        pr_info("Error deseriali");
        return tlv_failed;
    }

    pr_info("%d", TYPE_INSTR);
    //print_tlv(&recieved); // Just for checking attributes

    if(check_instr(recieved) < 0) {
    	//TODO check instructions
        return tlv_failed;
    } //Check which instruction

    free_tlv(&recieved);

    //return tlv_success;
    return nl_send_msg(rec_pid, seq, 1);
}

/**
* Construct a TLV message. 
*   Takes the status which will be put in a tlv message which will be  
*   serialized in to the buffer.
*
* Returns a serialized buffer.
*
*/
int create_tlv_message(int status, unsigned char* buffer) {
    struct TLV_holder construct; 
    int pload_length = 0;
    int err = 0;
    
    memset(&construct, 0 , sizeof(construct));
    
    if(status == 0){
        tlv_add_integer(&construct, 1);
        err = serialize_tlv(&construct, buffer, &pload_length);
        if(err != 0) {
            free_tlv(&construct);
            pr_err("falied to serialize message");            
            return pload_length;
        }
            
    } else {
        tlv_add_integer(&construct, 1);
        err = serialize_tlv(&construct, buffer, &pload_length);
        if(err != 0) {
            free_tlv(&construct);
            pr_err("falied to serialize message");            
            return pload_length;
        }  
    }
    free_tlv(&construct);
    return pload_length;
}


