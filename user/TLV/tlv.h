/**
* 
* TLV is used for parsing and creating a type - length - value struct 
* which can be serialized in to a byte array. deserialize_tlv will convert 
* the byte string back to a tlv object. 
* 
* Supported data types is currently strings and integers. 
* 
* ex: of payload
*   1 = int || 2 = char*
*
*	type---len---value
*	First obj  - 1/4/WRITE_INSTR  
*
*	Second obj - 2/sizeof(char*)/"Name Namesson" 
*   
*/		


#ifndef TLV_H
#define TLV_H

#include <stdint.h>

#ifndef tlv_failed 
#define tlv_failed -1
#endif

#ifndef tlv_success 
#define tlv_success 0
#endif

#ifndef MAX_OBJS    
#define MAX_OBJS    2
#endif



/*
* Type -length -value struct
*/
struct TLV {
	uint8_t type;   // max types is 255
	int16_t len;   // not a bigger size then 65535
	int32_t *data; // pntr to data
};

struct TLV_holder {
	struct TLV tlv_arr[MAX_OBJS];
	int nr_of_structs; 
};



typedef enum {TLV_INTEGER = 1, TLV_STRING, TLV_INSTR = 255 } datatypes; // 1 and 2

/* Enums for kernel to know how to process a tlv message. Userspace 
*  should know which request is sent.
*/
//enum instruction {READ_INSTR = 30, WRITE_INSTR}; // 30 and 31


/* Returns how many bytes has been used to create a tlv type. */

int32_t tlv_add_instruction(struct TLV_holder* holders, int32_t number);
int32_t tlv_add_integer(struct TLV_holder *tlvs, int integer);
int32_t tlv_add_string(struct TLV_holder *tlvs, const char* string);
int32_t add_raw_tlv(struct TLV_holder *tlvs, const unsigned char type, const int16_t size, const void* ptr);
int32_t serialize_tlv(struct TLV_holder *src,  unsigned char* dest, int *byte_counter);
int32_t deserialize_tlv(struct TLV_holder *dest, unsigned char* src,  int nr_of_bytes);
int32_t free_tlv(struct TLV_holder *tlvs); 
int32_t print_tlv(struct TLV_holder *tlvs);


#endif //TLV_H
