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

int construct_kwstring(struct TLV_holder received) {

	char temp_string[200]; 
	int value = -1;
	
	pr_info("[write_to_storage] - dataKey: %s", (char*)received.tlv_arr[INSTR_INDEX + 1].data);
    
    memcpy(&value, received.tlv_arr[INSTR_INDEX + 2].data, sizeof(int32_t));
    pr_info("[write_to_storage] - data: %d", value);
    
    sprintf(temp_string, "key:%s - value%d",
    					 (char*)received.tlv_arr[INSTR_INDEX + 1].data, value);
    pr_info("%s", temp_string);
    
    return 0;

}

/*
 * write_to_storage
 * Given a key and a value. Saves that infromation in a temporary hash table.
 * If successfully completed, send key and value to a new function.
 */
int write_to_storage(struct TLV_holder received)
{
    char * key, * value;
    int value_len;

    if(received.tlv_arr[INSTR_INDEX + 1].type != parse_string
        || received.tlv_arr[INSTR_INDEX + 2].type != parse_string)
    {
        pr_info("[write_to_storage] - incorrect key/value field type");

        return tlv_failed;
    }

    key = (char *)received.tlv_arr[INSTR_INDEX + 1].data;
    value = (char *)received.tlv_arr[INSTR_INDEX + 2].data;
    value_len = received.tlv_arr[INSTR_INDEX + 2].len;

    pr_info("[write_to_storage] - key: %s", key);
    pr_info("[write_to_storage] - data: %s (%d bytes)", value, value_len);
    kvs_insert(key, value, value_len);
    construct_kwstring(received);

    return tlv_success;
}

/*
 * read_from_storage
 * Given a key values, retrieves the value from storage.
 */
int read_from_storage(struct TLV_holder received, pid_t pid)
{
    struct TLV_holder transmitted;

    char msgbuf[MAX_PAYLOAD];
    int msglen = -1;

    char * key, * value;

    /*
     * Make sure the string is actually a string first.
     */

    if(received.tlv_arr[INSTR_INDEX + 1].type != parse_string)
    {
        pr_info("[read_from_storage] - incorrect key field type");
        return tlv_failed;
    }

    /*
     * Now, retrieve the value from the KVS using the given key.
     * If it exists, copy the value into a TLV buffer and send it.
     */

    key = (char *)received.tlv_arr[INSTR_INDEX + 1].data;

    value = kvs_get(key);
    if(value != NULL)
    {
        memset(&transmitted, 0, sizeof(struct TLV_holder));

        tlv_add_string(&transmitted, value);
    }


    serialize_tlv(&transmitted, msgbuf, &msglen);
    free_tlv(&transmitted);

	if(nl_send < 0)
	
    return tlv_success;
}

/*
 * parse_tlv_message
 * Deserializes a buffer into a TLV structure for message processing.  
 */
int parse_tlv_message(int seq, int rec_pid, unsigned char* buffer, int buf_len)
{
    int err, op;

    struct TLV_holder received;
    int received_maxobjs, received_type, received_len;

    /* Clear the TLV structure before deserializing the buffer. */
    memset(&received, 0, sizeof(received));

    err = deserialize_tlv(&received, buffer,  buf_len);
    if(err != 0)
    {
        pr_err("[parse_tlv_message] - TLV message deserialization error");
        return tlv_failed;
    }

    /*
     * First, make sure that the received message is valid.
     * Then, attempt to determine what to do with the message.
     */

    received_maxobjs = received.nr_of_structs;

    received_type = received.tlv_arr[INSTR_INDEX].type;
    received_len = received.tlv_arr[INSTR_INDEX].len;

    if(received_maxobjs != MAX_OBJS || received_type != TYPE_INSTR || received_len != INT32_SIZE)
    {
        pr_err("[parse_tlv_message] - Malformed TLV message!");
        pr_err("[parse_tlv_message] - max_objs = %d, type = %d, len = %d",
            received_maxobjs, received_type, received_len);

        free_tlv(&received);
        return tlv_failed;
    }

    /*
     * Message appears to be consistent, now process it!
     */

    memcpy(&op, received.tlv_arr[INSTR_INDEX].data, sizeof(int));

    switch(op)
    {
        case TYPE_READ:
            read_from_storage(&received);
            break;

        case TYPE_WRITE:
            write_to_storage(&received);
            break;

        default:
            free_tlv(&received);
            return tlv_failed;
    }

    /*
     * Clean up after processing and return.
     */

    //return nl_send_msg(rec_pid, seq, 1, NULL);

    free_tlv(&received);
    return tlv_success;
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
        tlv_add_integer(&construct, 0);
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


