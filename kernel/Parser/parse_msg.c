#include <linux/kernel.h>
#include <linux/module.h>
#include "../myheader.h"
#include "../TLV/tlv.h"
#include<linux/slab.h>
#include <asm/types.h>
#include <linux/string.h>

DATATYPES parse_string = TLV_STRING;
DATATYPES parse_int    = TLV_INTEGER;


#ifndef INSTR_INDEX
#define INSTR_INDEX 0
#endif


struct TLV_holder recieved, construct;

enum {TYPE_READ, TYPE_WRITE, TYPE_INSTR = 255 }; 

int write_to_storage (void) {
    
    if(recieved.tlv_arr[INSTR_INDEX + 1].type != parse_string){
        pr_info("[read_from_storage] - not a valid value");
        return tlv_failed;
    }
    
    pr_info("[write_to_storage] - data: %s", 
            (char*)recieved.tlv_arr[INSTR_INDEX + 1].data);
    return tlv_success;
}

int read_from_storage(void) {
    int value_key;
    
    if(recieved.tlv_arr[INSTR_INDEX + 1].type != parse_int){
        pr_info("[read_from_storage] - not a key");
        return tlv_failed;
    }
    memcpy(&value_key, recieved.tlv_arr[INSTR_INDEX + 1].data, sizeof(int32_t));
    pr_info("[read_from_storage] - data %d ", value_key);
    return tlv_success;
}


/*
* 
*
*/
int check_instr(int rec_pid, int seq)
{
    int incoming_instr;
    
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
            read_from_storage();
            break;
        case TYPE_WRITE:
            pr_info("[check_instr] - WRITE " );
            write_to_storage();
            break;
        default:
            //TODO send error message back. 
            return tlv_failed;
    }
    
    return tlv_success;
}


/*
*   parse_message
*
*/
int parse_message(int seq, int rec_pid ,unsigned char* buffer, int buf_len) {
    int err;
    
    memset(&recieved, 0 , sizeof(recieved));
    err = deserialize_tlv(&recieved, buffer,  buf_len);
    if(err != 0) {
        pr_info("Error deseriali");
        return tlv_failed;
    }
        pr_info("%d", TYPE_INSTR);
    print_tlv(&recieved); // Just for checking attributes
        
    if(check_instr(rec_pid, seq) < 0) {
        return tlv_failed;
    } //Check which instruction
    
	pr_info("Should call correctfunction in module.. \n");
	free_tlv(&recieved);

    //return tlv_success;
	return nl_send_msg(rec_pid, seq);
}



