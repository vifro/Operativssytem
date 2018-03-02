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

#ifndef NETLINK_USER 
#define NETLINK_USER 31
#endif

#ifndef MAX_PAYLOAD 
#define MAX_PAYLOAD 1024
#endif


struct sock *nl_sock = NULL;

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
		
	strcpy(NLMSG_DATA(nl_hdr), "Hello user, this is the kernel");
	
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

    /*
     * static DEFINE_MUTEX(nl_cfg_mutex);
     *  Check
     *  mutex_lock(nl_mutex_lock);
     * and
     * mutex_unlock(nl_mutex_unlock);
     * */

    //int err;
    int err;
    pr_info("Entering %s \n", __FUNCTION__);

    nl_hdr=(struct nlmsghdr*)skb->data;
    pid = nl_hdr->nlmsg_pid;
	seq = nl_hdr->nlmsg_seq;
	
	pr_info("Netlink recieved message with payload: %s \n",(char*)NLMSG_DATA(nl_hdr));
	
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
