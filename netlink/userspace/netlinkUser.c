#include <stdio.h>
#include <sys/socket.h>
#include <linux/netlink.h>
#include <string.h>
#include <zconf.h>
#include <stdlib.h>


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
struct nlmsghdr *nl_hdr = NULL;

/*Process pid*/
pid_t PID;
int seqNo = 1337;

void set_src_addr(void);
void set_dest_addr(void);
void send_message(char*);
int recieve_message(void);
int open_connection(void);


int main(int argc, char* argv[]) {

    printf("Hello, World! This is a test of simple netlink implementation\n");
    

    //TODO send in correct params for both src and dest
    set_src_addr();
    set_dest_addr();

    if(open_connection() != 0){
        fprintf(stderr, "Could not connect\n");
        return ERROR;
    }
    send_message("Hello kernel, this is from user-space\n" );
    return SUCCESS;
}

void set_src_addr(){

    memset(&src_addr, 0, sizeof(struct sockaddr_nl));
    src_addr.nl_family = AF_NETLINK;

    /*If pid is set before bind, application has the responsibility to add unique pid to each connection
     * identifies a socket, not a process. */
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

void send_message(char *message){
	
    //struct iovec iov = {nl_hdr, nl_hdr->nlmsg_len};
    //struct msghdr msg =  { &dest_addr, sizeof(dest_addr), &iov, 1, NULL, 0, 0 };
	PID = getpid();
	
    nl_hdr = (struct nlmsghdr*)malloc(NLMSG_SPACE(MAX_PAYLOAD));
    memset(nl_hdr, 0 , sizeof(struct nlmsghdr));
    nl_hdr->nlmsg_len = NLMSG_LENGTH(NLMSG_SPACE(MAX_PAYLOAD));
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
    strcpy(NLMSG_DATA(nl_hdr), message);
	printf("copieddata to nl_hdrwith pid: %d\n", PID);
	
    sendmsg(sock_fd, &msg, 0);
    printf("PID-%d sending with payload:\n%s\n", src_addr.nl_pid,(char*)NLMSG_DATA(nl_hdr));

    if(recieve_message() != 0){
    	printf("an error occured while recieving message\n");
    	return;
    
    }
	free(nl_hdr);
}

int recieve_message(){
    
	
    //memset(&msg, 0 , sizeof(struct msghdr));
	
	
    iov.iov_base = (void *)nl_hdr;
    iov.iov_len  = nl_hdr->nlmsg_len;
    msg.msg_name = (void*) &src_addr;
    msg.msg_namelen = sizeof(src_addr);

	/*recvmsg blocks api until message is recieved.*/
	printf("waiting to recieve message from kernel\n");
    recvmsg(sock_fd, &msg, 0);
	
    if(nl_hdr->nlmsg_pid != 0){
        printf("msg from unknown source, it has: %d \n", nl_hdr->nlmsg_pid);
    }
    printf("Message recieved %s\n With len:%d\n", (char*)NLMSG_DATA(nl_hdr), nl_hdr->nlmsg_len);

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
