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


/* Variables for handling scket communication*/
struct sockaddr_nl src_addr, dest_addr;
int sock_fd, seqNo;
struct iovec iov;
struct msghdr msg;
/*NETLINK message pointers */

//TODO Implement nlattr to send correct shit
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
int cr_tlv_msg(unsigned char* buffer);
int recv_tvl_msg(unsigned char* buffer, int pload_len);


int main(int argc, char* argv[]) {
    
    printf("Hello, World! This is a test of simple netlink implementation\n");
    struct TLV_holder tlv_holder1, tlv_holder2;
    unsigned char buffer[MAX_PAYLOAD] = {0};
    int pload_len = 0;
    int err = 0;
    int i = 0;
    
    
    
    /* Message out, build, serialize, print for debugging and free */
    
    pload_len = cr_tlv_msg(buffer);
    if(pload_len < 0) {
        printf("Error creating tlv");
        return ERROR;    
    }
    printf("The pload_len is %d", pload_len);
    
    /* Message recieved, deserialize to holder, print value and free*/
    //err = recv_tvl_msg(buffer, pload_len);
    
    
    //TODO send in correct params for both src and dest
    
    set_src_addr();
    set_dest_addr();
    
    if(open_connection() != 0){
        fprintf(stderr, "Could not connect\n");
        return ERROR;
    }
    
    while(i < 100){
    	sleep(1); // seconds
		send_message(pload_len, buffer);
		
		if(recieve_message() != 0){
			printf("an error occured while recieving message\n");
			return ERROR;
		}
		i++;
    }
    
    close(sock_fd);
	free(nl_hdr);
    
    return SUCCESS;
}

void set_src_addr(){

    memset(&src_addr, 0, sizeof(struct sockaddr_nl));
    src_addr.nl_family = AF_NETLINK;

    /*If pid is set before bind, application has the responsibility to add 
    *  unique pid to each connection. Identifies a socket, not a process. 
    */
    src_addr.nl_pid = getpid();

    //src_addr.nl_groups = 0-32 , default is 0, each family has 23 grops.
    //src_addr.nl_pad = 0; , used for zeroing

}

void set_dest_addr(){
    memset(&dest_addr, 0, sizeof(struct sockaddr_nl));
    dest_addr.nl_family = AF_NETLINK;
    dest_addr.nl_pid = 0;
    dest_addr.nl_groups = 0;
}

int cr_tlv_msg(unsigned char* buffer){
    
    printf("\n\n------------------> cr_tlv_msg <--------------------- \n");
    struct TLV_holder tlv_holder1;
    int pload_len = -1;
    
    memset(&tlv_holder1, 0 , sizeof(tlv_holder1));    
    
    tlv_add_instruction(&tlv_holder1, 1);
    tlv_add_string(&tlv_holder1, "hello kernel, tlv mess here..");
    print_tlv(&tlv_holder1);
    serialize_tlv(&tlv_holder1, buffer, &pload_len);
    
    free_tlv(&tlv_holder1);
    
    return pload_len;
}


int recv_tvl_msg(unsigned char* buffer, int pload_len) {

    struct TLV_holder tlv_reciever;
        
    memset(&tlv_reciever, 0 , sizeof(tlv_reciever));
    deserialize_tlv(&tlv_reciever, buffer, pload_len);
    
    print_tlv(&tlv_reciever);
    
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
	
	
	//TODO try to fix sizes of msg and setup depending on created tlv buffer.
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

	printf("copying data to nl_hdr with pid: %d\n", PID);
    //TODO Use tlv to prepare a message And fix to correct size, as buffer.
    //strcpy(NLMSG_DATA(nl_hdr), message);
    memcpy(NLMSG_DATA(nl_hdr), message, len);
    
	printf("copieddata to nl_hdrwith pid: %d\n", PID);
	
    size = sendmsg(sock_fd, &msg, 0);
    if(size < 0 ) {
        printf("recieved %zu from sendmsg", size);
        return ERROR;
    }
    
    printf("PID-%d sending with payload size:%zu\n", src_addr.nl_pid, size);
    
    return SUCCESS;

}

int recieve_message(){
    
    //memset(&msg, 0 , sizeof(struct msghdr));
	struct TLV_holder tlv_reciever;
    memset(&tlv_reciever, 0 , sizeof(tlv_reciever));
	memset(nl_hdr, 0 , sizeof(struct nlmsghdr));
	/*
    iov.iov_base = (void *)nl_hdr;
    iov.iov_len  = nl_hdr->nlmsg_len;
    msg.msg_name = (void*) &src_addr;
    msg.msg_namelen = sizeof(src_addr);
	*/
	/*recvmsg blocks api until message is recieved.*/
	printf("waiting to recieve message from kernel\n");
    recvmsg(sock_fd, &msg, 0);
	
    if(nl_hdr->nlmsg_pid != 0){
        printf("msg from unknown source, it has: %d \n", nl_hdr->nlmsg_pid);
    }

    //TODO Parse with deserialize to get the data. Create a parse_incoming.
	/*Deserialize, print then free tlv holder */
    //deserialize_tlv(&tlv_reciever, NLMSG_DATA(nl_hdr), nl_hdr->nlmsg_len);
    deserialize_tlv(&tlv_reciever, NLMSG_DATA(nl_hdr), 7);
    print_tlv(&tlv_reciever);
    free_tlv(&tlv_reciever);
    
    printf("Message recieved %s\n With len:%d\n", (char*)NLMSG_DATA(nl_hdr), nl_hdr->nlmsg_len );
	
    close(sock_fd);
    
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
