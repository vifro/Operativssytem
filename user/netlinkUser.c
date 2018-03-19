/*
 * Creates, sends and receives messages over netlink.
 * Messages is supposed to be on the type TLV and will be sent in packages of 
 * three. First is to declare instruction, second is key value, and the third is
 * the value to be stored.
 *
 * Author: Victor From <c13vfm@cs.umu.se>
 * 
 * To see how the messages are pu together and parsed, check the TLV/tlv.h
 * and tlv.c for implementation.
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
#define MAX_PAYLOAD     4096	//Page size
#define NETLINK_USER    31
#define READ_INSTR		0
#define WRITE_INSTR 	1


/* Variables for handling scket communication*/
struct sockaddr_nl src_addr, dest_addr;
int sock_fd, seqNo;

/* Message  */
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
int cr_tlv_msg(unsigned char* buffer, char* string, char * number);
int recv_tlv_msg(unsigned char* buffer, int pload_len);
int loop_message(char *keyvalue);


/* 
 * Takes either two arguments or nothing. The first is a char* describing a key
 * and the second is a char / integer describing the value to be stored.
 * No arguments results in default values.
 */
int main(int argc, char *argv[]) {
    
    printf("Hello, World! This is a test of simple netlink implementation\n");
    char *temp;
    
    set_src_addr();
    set_dest_addr();
    
    if(open_connection() != 0){
        fprintf(stderr, "Could not connect\n");
        return ERROR;
    }
    
    if(argc == 3){
    	
    	printf("argc is 3\n");
    	temp = argv[1];
    	seqNo = atoi(argv[2]);
    	printf("%s - %d", temp, seqNo);
    	loop_message(temp);
    } else if (argc == 1){
    	printf("no arguments\n");
    	temp = "Name Namesson";
    	loop_message(temp);
    } else {
    	printf("Wrong argument count, shoud be: \
    							./application [name] [sequencenumber], or nothing\n");
    	exit(EXIT_FAILURE);
    }
    
    
    
    close(sock_fd);
	 
    return SUCCESS;
} 

/*
 * Create several messages to be sent over sockets to userspace. Creates unique 
 * key values for each message.
 * 
 */
int loop_message(char *keyvalue){
	struct TLV_holder tlv_holder1, tlv_holder2;
    unsigned char buffer[MAX_PAYLOAD];
    char *temp_string;
    char temp_string1[256];
    int pload_len = 0;
    int err = 0;
    int i = 0;

	while(i < 10){
    	sleep(1); // seconds
    	
    	
    	sprintf(temp_string1, "%s-%d", keyvalue, seqNo);
    	temp_string = temp_string1;
        /* Message out, build, serialize, print for debugging and free */
        printf("this is tempstring: %s\n", temp_string);
        pload_len = cr_tlv_msg(buffer, temp_string, "19900909");
        
        if(pload_len < 0) {
            printf("Error creating tlv");
            return ERROR;    
        }
                
        
		if(send_message(pload_len, buffer) != 0){
				exit(-1);
		}
		
		if(recieve_message() != 0){
			printf("an error occured while recieving message\n");
			return ERROR;
		}
        memset(buffer, 0, sizeof(buffer));
        free(nl_hdr);
		i++;
    }
}


/*
 * Sets family for netlink connection and sets pid to the process.
 * If pid is set before bind, application has responsibility to add a 
 * unique pid to each connection. 
 */
void set_src_addr(){

    memset(&src_addr, 0, sizeof(struct sockaddr_nl));
    src_addr.nl_family = AF_NETLINK;

    /* Identifies a socket, not a process.  */    
    src_addr.nl_pid = getpid();


}


/*
 * Sets destination address. 
 * pid = 0 for kernel.
 */
void set_dest_addr(){
    memset(&dest_addr, 0, sizeof(struct sockaddr_nl));
    dest_addr.nl_family = AF_NETLINK;
    dest_addr.nl_pid = 0;
    dest_addr.nl_groups = 0;
}

int cr_tlv_msg(unsigned char* buffer, char* key, char* value)
{
    struct TLV_holder tlv_holder1;
    int pload_len = -1;
    
    memset(&tlv_holder1, 0 , sizeof(tlv_holder1));
    
    tlv_add_instruction(&tlv_holder1, WRITE_INSTR);
    tlv_add_string(&tlv_holder1, key);
    tlv_add_string(&tlv_holder1, value);

    //printf("print tlv holder\n");
    //print_tlv(&tlv_holder1);
    
    serialize_tlv(&tlv_holder1, buffer, &pload_len);
    free_tlv(&tlv_holder1);
    
    return pload_len;
}

/*
 * Receives buffer to deserialises a TLV message. 
 */
int recv_tlv_msg(unsigned char* buffer, int pload_len) {

    struct TLV_holder tlv_reciever;
        
    memset(&tlv_reciever, 0 , sizeof(tlv_reciever));
    deserialize_tlv(&tlv_reciever, buffer, pload_len);
    free_tlv(&tlv_reciever);
    
    return SUCCESS;
}

/*
 * Sets parameters in netlink message headers.
 * 
 */
int send_message(int len, unsigned char *message){
    ssize_t size;
	PID = getpid();
	
	/* Adding information to header */
    nl_hdr = (struct nlmsghdr*)malloc(NLMSG_SPACE(MAX_PAYLOAD));
    memset(nl_hdr, 0 , sizeof(struct nlmsghdr));
    nl_hdr->nlmsg_len = NLMSG_LENGTH(len);      // Length payload w/out padding.
    nl_hdr->nlmsg_pid = PID; 			        // process pid
    nl_hdr->nlmsg_flags = 0;
    nl_hdr->nlmsg_seq = seqNo++;				// Keep track of respons
	
	/* Point the headers infomation to the msghdr and iov */
    iov.iov_base = (void*)nl_hdr;				
    iov.iov_len = nl_hdr->nlmsg_len;
    msg.msg_name = (void *)&dest_addr;			
    msg.msg_namelen = sizeof(dest_addr);
    msg.msg_iov = &iov;
    msg.msg_iovlen = 1; 						// number of iov structs. 


   	/* Copy the message to payload */
    memcpy(NLMSG_DATA(nl_hdr), message, len);

	/* Send message to kernel*/	
    size = sendmsg(sock_fd, &msg, 0);
    if(size < 0 ) {
        printf("recieved %zu from sendmsg\n", size);
        return ERROR;
    }
    printf("\nPID-%d sending with sequence number:%d\n", src_addr.nl_pid, seqNo);
    
    return SUCCESS;

}

/* QUOTE: netlink.h
   However, reliable transmissions from kernel to user are impossible in
   any case.  The kernel can't send a  netlink  message  if  the  socket
   buffer  is  full:  the message will be dropped and the kernel and the
   user-space process will no longer have the same view of kernel state.
   It  is  up  to  the  application to detect when this happens (via the
   ENOBUFS error returned by recvmsg(2)) and resynchronize.
*/

/*
 * Recieve message from and parse kernel. 
 */
int recieve_message(){
	memset(nl_hdr, 0 , sizeof(struct nlmsghdr));
	
	printf("waiting to recieve message from kernel\n");
    recvmsg(sock_fd, &msg, 0);
	
    if(nl_hdr->nlmsg_pid != 0){
        printf("msg from unknown source, it has: %d \n", nl_hdr->nlmsg_pid);
    }
    //TODO fix parameters. 
    printf("rec mess with seqnr: %d\n\n", seqNo);
    recv_tlv_msg(NLMSG_DATA(nl_hdr), 7);
        
    return SUCCESS;
}

/*
 * Establish connection. 
 */
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
