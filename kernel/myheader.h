#ifndef _MYHEADER_H
#define _MYHEADER_H

int write_to_storage(void);
int read_from_storage(void);
int check_instr(int rec_pid, int seq);
int parse_message( int seq , int pid, unsigned char* buffer, int buf_len);
int nl_send_msg(unsigned int, int); 
#endif
