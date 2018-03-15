/*
 *
 *
 * Author: Victor From <c13vfm@cs.umu.se>
 * 
 *  
 *
 */

#include <stdio.h>
#include <sys/socket.h>
#include "TLV/tlv.h"
#include <linux/netlink.h>
#include <string.h>
#include <zconf.h>
#include <stdlib.h>
#include <unistd.h>
#include <linux/genetlink.h>


#define SUCCESS			0
#define ERROR			-1
#define MAX_PAYLOAD     4096
#define NETLINK_USER    31
#define READ_INSTR		0
#define WRITE_INSTR 	1


/* Variables for handling scket communication*/
struct sockaddr_nl src_addr, dest_addr;
int sock_fd, seqNo;
struct iovec iov;
struct msghdr msg;
/*NETLINK message pointers */

struct nlmsghdr *nl_hdr = NULL;

/*Process pid*/
pid_t PID;
int seqNo = 1337;


void set_src_addr(void);
void set_dest_addr(void);
int conf_msg(int option, char* message, int key, unsigned char* buffer);
int send_message(int payload_length,unsigned char* mess);
int recieve_message(void);
int open_connection(void);
int cr_tlv_msg(unsigned char* buffer, char* string, int number);
int recv_tlv_msg(unsigned char* buffer, int pload_len);


int main(int argc, char* argv[]) {
    
    printf("Hello, World! This is a test of simple netlink implementation\n");
    struct TLV_holder tlv_holder1, tlv_holder2;
    unsigned char buffer[MAX_PAYLOAD] = {0};
    int pload_len = 0;
    int err = 0;
    int i = 0;
    
    set_src_addr();
    set_dest_addr();
    
    if(open_connection() != 0){
        fprintf(stderr, "Could not connect\n");
        return ERROR;
    }
    
    while(i < 10){
    	sleep(1); // seconds
    	
        /* Message out, build, serialize, print for debugging and free */
        pload_len = cr_tlv_msg(buffer, "Name Namesson", 19900909);
        
        if(pload_len < 0) {
            printf("Error creating tlv");
            return ERROR;    
        }
                
        
		if(send_message(pload_len, buffer) != 0){
				
		}
		
		if(recieve_message() != 0){
			printf("an error occured while recieving message\n");
			return ERROR;
		}
        memset(buffer, 0, sizeof(buffer));
        free(nl_hdr);
		i++;
    }
    
    close(sock_fd);
	 
    return SUCCESS;
}

void set_src_addr(){

    memset(&src_addr, 0, sizeof(struct sockaddr_nl));
    src_addr.nl_family = AF_NETLINK;

    /*If pid is set before bind, application has the responsibility to add 
    *  unique pid to each connection. Identifies a socket, not a process. 
    */
    src_addr.nl_pid = getpid();


}

void set_dest_addr(){
    memset(&dest_addr, 0, sizeof(struct sockaddr_nl));
    dest_addr.nl_family = AF_NETLINK;
    dest_addr.nl_pid = 0;
    dest_addr.nl_groups = 0;
}

int cr_tlv_msg(unsigned char* buffer, char* string, int number){
    
    struct TLV_holder tlv_holder1;
    int pload_len = -1;
    
    memset(&tlv_holder1, 0 , sizeof(tlv_holder1));
    
    tlv_add_instruction(&tlv_holder1, WRITE_INSTR);
    tlv_add_string(&tlv_holder1, string);
    tlv_add_integer(&tlv_holder1, number);
    
    serialize_tlv(&tlv_holder1, buffer, &pload_len);
    free_tlv(&tlv_holder1);
    
    return pload_len;
}


int recv_tlv_msg(unsigned char* buffer, int pload_len) {

    struct TLV_holder tlv_reciever;
        
    memset(&tlv_reciever, 0 , sizeof(tlv_reciever));
    deserialize_tlv(&tlv_reciever, buffer, pload_len);
    
    //print_tlv(&tlv_reciever);
    
    free_tlv(&tlv_reciever);
    
    return SUCCESS;
}

int32_t conf_msg(int option, char* message, int key, unsigned char* buffer) {
    

	//TODO FIX inparameters, set upp correct messages and return serialized.
	return tlv_success;
}

int send_message(int len, unsigned char *message){
    ssize_t size;
	//unsigned char buffer[MAX_PAYLOAD];
    //struct iovec iov = {nl_hdr, nl_hdr->nlmsg_len};
    //struct msghdr msg =  { &dest_addr, sizeof(dest_addr), &iov, 1, NULL, 0, 0 };
	PID = getpid();
	
	
	
    nl_hdr = (struct nlmsghdr*)malloc(NLMSG_SPACE(MAX_PAYLOAD));
    memset(nl_hdr, 0 , sizeof(struct nlmsghdr));
    nl_hdr->nlmsg_len = NLMSG_LENGTH(len); // length of payload without NLMSG_ALIGN
    nl_hdr->nlmsg_pid = PID;
    nl_hdr->nlmsg_flags = 0;
    nl_hdr->nlmsg_seq = seqNo++;

    iov.iov_base = (void*)nl_hdr;
    iov.iov_len = nl_hdr->nlmsg_len;
    msg.msg_name = (void *)&dest_addr;
    msg.msg_namelen = sizeof(dest_addr);
    msg.msg_iov = &iov;
    msg.msg_iovlen = 1; /* number of iov structs. */ 

    //TODO Use tlv to prepare a message And fix to correct size, as buffer.
    //strcpy(NLMSG_DATA(nl_hdr), message);
    memcpy(NLMSG_DATA(nl_hdr), message, len);
	
    size = sendmsg(sock_fd, &msg, 0);
    if(size < (ssize_t)0 ) {
        printf("recieved %zu from sendmsg", size);
        return ERROR;
    }
    printf("\nPID-%d sending with sequence number:%d\n", src_addr.nl_pid, seqNo);
    
    return SUCCESS;

}

int recieve_message(){
	memset(nl_hdr, 0 , sizeof(struct nlmsghdr));
	
	printf("waiting to recieve message from kernel\n");
    recvmsg(sock_fd, &msg, 0);
	
    if(nl_hdr->nlmsg_pid != 0){
        printf("msg from unknown source, it has: %d \n", nl_hdr->nlmsg_pid);
    }
    printf("rec mess with seqnr: %d\n\n", seqNo);
    recv_tlv_msg(NLMSG_DATA(nl_hdr), 7);
        
    return SUCCESS;
}

int open_connection(){
    sock_fd = socket(AF_NETLINK, SOCK_RAW, NETLINK_USER);

    if(sock_fd < 0){
        fprintf(stderr, "problem connection to socket\n");
        return ERROR;
    }

    if(bind(sock_fd, (struct sockaddr*)&src_addr, sizeof(src_addr)) != 0){
        fprintf(stderr, "Error binding socket\n");
        return ERROR;
    }
    printf("successfully binding socket!\n");

    return SUCCESS;
}
