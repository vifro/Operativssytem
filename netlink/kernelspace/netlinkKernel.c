#include "myheader.h"
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
#define MAX_PAYLOAD 1024
#endif

unsigned char *ptr;
struct sock *nl_sock = NULL;

struct TLV {
	    int8_t type;   // max types is 255
	    int16_t len;   // not a bigger size then 65535
	    int32_t *data; // pntr to data
};
struct TLV tlv_test;    


/*
* Send back to user, given sequence number and the pid of process to be reached.
*
*/
int nl_send_msg(u32 rec_pid , int seq) {

	struct sk_buff *skb = NULL;
	struct nlmsghdr *nl_hdr;
	int err;
	

    
	/* Allocate a new netlink message */
	skb = nlmsg_new(NLMSG_SPACE(MAX_PAYLOAD), GFP_KERNEL);
	
	if(!skb){
		pr_info("vailed to allocate skb");
		return -1;
	}
	
	nl_hdr = (struct nlmsghdr*)skb->data;
	nl_hdr->nlmsg_len = NLMSG_SPACE(MAX_PAYLOAD);
	nl_hdr->nlmsg_pid = 0; 
	nl_hdr->nlmsg_flags = 0;
	
	/* Add a netlink msg to an sk_bff */
	nl_hdr = nlmsg_put(skb, 0, rec_pid, NLMSG_DONE, nl_hdr->nlmsg_len, 0);
	
	NETLINK_CB(skb).dst_group = 0; /* Unicast */
	NETLINK_CB(skb).portid = 0; /* from kernel */	
	
	pr_info("pid recieved: %d \n and seq is: %d\n" , rec_pid, seq);
		
	strcpy(NLMSG_DATA(nl_hdr), "change this");
	
	err =  nlmsg_unicast(nl_sock, skb, rec_pid);
	if(err < 0) {
		pr_info("Failed to send data\n");
		return -1;
	}
    

	pr_info("Sent message: %s \n", (char*)NLMSG_DATA(nl_hdr));
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
    int byte_counter;
    int err;
    int i;
    int temp;
    
    unsigned char buffer[512]; // FIX the size

    
    pr_info("Entering %s \n", __FUNCTION__);
    
    nl_hdr=(struct nlmsghdr*)skb->data;
    pid = nl_hdr->nlmsg_pid;
	seq = nl_hdr->nlmsg_seq;
	
	pr_info("Netlink recieved message with payload: %s \n",(unsigned char*)NLMSG_DATA(nl_hdr));
	   
    
    
    /* -----------------------------------------------------------------------*/
    /* -----------------------------------------------------------------------*/
    /* This is a test for parsing some data    */
    
    /* Type -length -value struct
    */
    memset(buffer, 0 , sizeof(buffer));
    memcpy(buffer, NLMSG_DATA(nl_hdr), NLMSG_PAYLOAD(nl_hdr, 0));
    
    
    
    //ptr = NLMSG_DATA(nl_hdr);
    
    byte_counter = 0;
    
    
        pr_info("byte counter is %d, and tot bytes is %d \n", byte_counter, NLMSG_PAYLOAD(nl_hdr, 0));     
        
        tlv_test.type = buffer[byte_counter];
        byte_counter += INT8_SIZE;
        pr_info("byte counter: %d \n", byte_counter);
        
        memcpy(&tlv_test.len, &buffer[byte_counter], INT16_SIZE);
        byte_counter += INT16_SIZE;
        pr_info("byte counter is %d\n", byte_counter);
        
        if(tlv_test.len != 0) {
            
            tlv_test.data = kmalloc(tlv_test.len + 1, GFP_ATOMIC);
            memset(tlv_test.data ,0, tlv_test.len + 1);
            memcpy(tlv_test.data, &buffer[byte_counter], tlv_test.len);              
            byte_counter += tlv_test.len + 1;
            pr_info("byte counter is %d\n", byte_counter);
            
            
            
            
        } else {
            pr_info("len is zero, nothing to parse.....\n");        
        }
        
    if(tlv_test.data == NULL) {
        pr_info("tlv.data is NULL");
    }             
    i = 0;
       
    pr_info("\ntlv.type: %d\n", tlv_test.type);
    pr_info("tlv.len: %d\n", tlv_test.len);
    /*    
    memcpy(&temp, tlv_test.data, sizeof(int32_t));
    pr_info("data is: %d\n", temp);
    */
    pr_info("data: %s \n", (char*)tlv_test.data);
   
    /* -----------------------------------------------------------------------*/
    /* -----------------------------------------------------------------------*/         
    
    
    
	err = do_something(pid, seq); 
	
	if(err < 0)
		pr_info("nl_send_msg failed");
	
	
    pr_info("message sent over socket");
	
}



/** 
* module init:
*			Set callback function to the one declared in this file.
*			Create a connection  
*/
static int  nlmodule_init(void) {	
	

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
