#ifndef _MYHEADER_H
#define _MYHEADER_H

#include "TLV/tlv.h"


int create_tlv_message(int status, unsigned char* buffer);
int write_to_storage(struct TLV_holder received, pid_t t, int seqNo);
int read_from_storage(struct TLV_holder received, pid_t t, int seqNo);

int parse_tlv_message( int seq , int pid, unsigned char *buffer, int buf_len);
int nl_send_msg(unsigned int pid, int seqnr, char *databuf, int msglen); 


#endif
