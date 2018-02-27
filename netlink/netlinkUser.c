#include <stdio.h>
#include <stdlib.h>
#include <asm/types.h>
#include <sys/socket.h>
#include <linux/netlink.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>

#define MAX_PAYLOAD		1024
#define NETLINK_USER	31
#define SUCCESS			1
#define ERROR			-1

struct sockaddr_nl s_addr, d_addr;
struct nlmsghdr *nl_hdr = NULL;
struct iovec iov;
int fd;

/**
*	msghdr from socket.h takes 7 parameters,
*	
*	void			__user *msg_name;		ptr to socekt adress
*	int				msg_namelen;			size of socke		
*	struct iovec	__user *msg_iov;		scatter/gather array
*	__kernel_size_t	msg_iovlen;				elements in msg_iov
*	void			__user  msg_control;	anccillary data
*   __kernel_size_t msg_controllen;			ancillary data buffer length
*	unsigned int    msg_flags;				flags on recieved message
*/
struct msghdr msg; 


int main () {
	/* source socket setup and bind , netlink.h manpage */
	fd = socket(AF_NETLINK, SOCK_RAW, NETLINK_USER);
	
	/* Check if socket recieved a file descriptor */
	if(fd < 0) {
		fprintf(stderr, "socket did not recieve fd");
		return ERROR;		
	}
	
	memset(&s_addr, 0, sizeof(s_addr));
	s_addr.nl_family = AF_NETLINK;
	s_addr.nl_pid = getpid(); /* process own pid*/
	
	bind(fd, (struct sockaddr *) &s_addr, sizeof(s_addr));

	/* setup netlink header, compute payload with hdr and msg later*/
	
	nl_hdr = (struct nlmsghdr *)malloc(NLMSG_SPACE(MAX_PAYLOAD)); 
	memset(nl_hdr, 0, NLMSG_SPACE(MAX_PAYLOAD));
	nl_hdr->nlmsg_len = NLMSG_SPACE(MAX_PAYLOAD);
	nl_hdr->nlmsg_pid = 0;
	nl_hdr->nlmsg_flags = 0;
	//nl_hdr->nlmsg_seq = ++sequence_number;

	/*Add the message to payload (data) */
	strcpy(NLMSG_DATA(nl_hdr), "User socket message");
	
	/*Request ack from kernel*/	
	//nl_hdr->nlmsg_flags |= NLM_F_ACK;
	
	//msg = {&s_addr, sizeof(s_addr), &iov, 1, NULL, 0, 0 };
	

	
	/* adress socket bind*/
	memset(&d_addr, 0, sizeof(d_addr));
	d_addr.nl_family = AF_NETLINK;
	d_addr.nl_pid = 0; /*Linux kernel*/
	d_addr.nl_groups = 0; /* unicast, also supports mulitcast*/
	
	iov.iov_base = (void*)nl_hdr;
	iov.iov_len = nl_hdr->nlmsg_len;
	msg.msg_name = (void *)&d_addr;
	msg.msg_namelen = sizeof(d_addr);
	msg.msg_iov = &iov;
	msg.msg_iovlen = 1;
	
	printf("Sending message: %s \n", (char*)NLMSG_DATA(nl_hdr));
	/*Sending message to kernel */
	sendmsg(fd,&msg, 0);
	
	printf("Waiting for response.. \n");
	
	/* Read the response from kernel */
	recvmsg(fd, &msg, 0);
	printf("Recieved message payload: %s \n", (char*)NLMSG_DATA(nl_hdr));
	
	/* Close socket connection */
	free(nl_hdr);
	close(fd);	
	
	return SUCCESS;
}
