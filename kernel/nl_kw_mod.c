#include "myheader.h"
#include "TLV/tlv.h"
#include <linux/module.h>
#include <linux/kernel.h>
#include <net/sock.h>
#include <linux/netlink.h>
#include <net/netlink.h>
#include <asm/types.h>
#include <linux/string.h>
#include <linux/skbuff.h>


/*
* To avoid possible overlaps:
* Remember to check that netlink bus is
* available and not used by other subsystems.
*/

#define INT8_SIZE   1
#define INT16_SIZE  2
#define INT32_SIZE  4

#ifndef NETLINK_USER 
#define NETLINK_USER 31
#endif

#ifndef MAX_PAYLOAD 
#define MAX_PAYLOAD 512
#endif

unsigned char *ptr;
struct sock *nl_sock = NULL;



/*
* Send back to user, given sequence number and the pid of process to be reached.
*
*/
int nl_send_msg(u32 rec_pid , int seq, int status) {

	struct sk_buff *skb;
	struct nlmsghdr *nl_hdr;
    unsigned char buffer[MAX_PAYLOAD] = {0}; //buffer used to construct message
	int err = 0;                        // err
    int pload_length = 0;               //length of tlv payload in bytes
    
	
    if(rec_pid == 0) {
        pr_err("Dont send to kernel or recieve from kernel!");
        return -1;
    }
       
	/* Allocate a new netlink message */
	skb = nlmsg_new(NLMSG_SPACE(MAX_PAYLOAD), GFP_KERNEL);
	
	if(!skb){
		pr_err("vailed to allocate skb");
		return -1;
	}
    
    /* create payload and recieve length */
    memset(buffer, 0, sizeof(buffer));
    pload_length = create_tlv_message(status, buffer); 
    if(pload_length <= 0) {
        pr_err("No message to send");
        return -1;
    }    
	
    /* Fill the header with data */
	nl_hdr = (struct nlmsghdr*)skb->data;
	nl_hdr->nlmsg_len = NLMSG_LENGTH(MAX_PAYLOAD);
	nl_hdr->nlmsg_pid = 0; 
	nl_hdr->nlmsg_flags = 0;
	
	/* Add a netlink msg to an sk_bff */
	nl_hdr = nlmsg_put(skb, 0, seq, NLMSG_DONE, nl_hdr->nlmsg_len, 0);
	
	NETLINK_CB(skb).dst_group = 0; /* Unicast */
	NETLINK_CB(skb).portid = 0; /* from kernel */	
	
	memcpy(NLMSG_DATA(nl_hdr), buffer, pload_length);
	
	err =  nlmsg_unicast(nl_sock, skb, rec_pid);
	if(err < 0) {
		pr_err("Failed to send data\n");
		return -1;
	}

	pr_info("Sent message to pid: %d with len: %d \n", rec_pid, pload_length);
	return 0;
}


/*
* Callback function that 
*
*/
static void nl_recv_callback(struct sk_buff *skb){

    struct nlmsghdr *nl_hdr;
    u32 pid;
    int seq;
    int buf_len;
    int err;
    
    unsigned char buffer[MAX_PAYLOAD]; // FIX the size

    pr_info("Entering %s \n", __FUNCTION__);
    
    nl_hdr=(struct nlmsghdr*)skb->data;
    pid = nl_hdr->nlmsg_pid;
	seq = nl_hdr->nlmsg_seq;
	
	pr_info("Netlink recieved message with payload: %s \n",
            (unsigned char*)NLMSG_DATA(nl_hdr));
	   
    /* Extract the buffer from payload */
    buf_len = NLMSG_PAYLOAD(nl_hdr, 0);
    
    memset(buffer, 0 , sizeof(buffer));
    memcpy(buffer, NLMSG_DATA(nl_hdr), buf_len);
    
    /* Parse the payload */
	err = parse_tlv_message(seq, pid, buffer, buf_len );  
	if(err < 0)
		pr_info("nl_send_msg failed");

    pr_info("message sent over socket");
	
}


/** 
* module init:
*			Set callback function to the one declared in this file.
*			Create a connection  
*/
static int __init nlmodule_init(void) {	
	

	struct netlink_kernel_cfg cfg = {
		.groups = 1,
		.input = nl_recv_callback,
	};
	
	printk("Entering: %s\n", __FUNCTION__);
	nl_sock = netlink_kernel_create(&init_net, NETLINK_USER, &cfg);

	if(!nl_sock) {
		pr_err("Error creating netlink socket");
		return -10;
	}
	pr_info("Successfully created netlink socket.");
	return 0;
}

/*
* exit module:
* 			Release socket.
*
*/
static void __exit nlmodule_exit(void) {	
	netlink_kernel_release(nl_sock);
	pr_info("exiting the nl module");
}

module_init(nlmodule_init);
module_exit(nlmodule_exit);

MODULE_LICENSE("GPL");
