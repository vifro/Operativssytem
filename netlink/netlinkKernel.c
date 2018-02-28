#include <linux/module.h>
#include <linux/kernel.h>
#include <net/sock.h>
#include <linux/netlink.h>
#include <asm/types.h>
#include <linux/string.h>

#define NETLINK_USER 31

struct sock *nl_sock = NULL;

static void nl_recv_msg(struct sk_buff *skb){

	struct nlmsghdr *nl_hdr;
	u32 pid;
	//struct sk_buff *skb_out = NULL;
	//int err;
	int err;
	pr_info("Entering %s \n", __FUNCTION__);
	
	nl_hdr=(struct nlmsghdr*)skb->data;
	pr_info("Netlink recieved message with payload: %s \n",NLMSG_DATA(nl_hdr));
	
	memset(skb, 0, sizeof(skb));
	skb = nlmsg_new(NLMSG_ALIGN(nl_hdr->nlmsg_len + 1), GFP_KERNEL);
	
	pid = nl_hdr->nlmsg_pid;
	pr_info("this is the pid from nlmsg %d", pid);
	NETLINK_CB(skb).portid = 0; /* from kernel */ 
	NETLINK_CB(skb).dst_group = 0; //unicast, default is 0 when initialised in userspace.
	NETLINK_CB(skb).portid = pid;
	/*
	skb_out = nlmsg_new(NLMSG_ALIGN(msg_size + nl_hdr->nlmsg_len), 0);
	
	
	nl_hdr = nlmsg_put(skb_out, 0, 0, NLMSG_DONE, msg_size + 1, 0);
	NETLINK_CB(skb_out).dst_group = 0; //not in multicast group
	strcpy(nlmsg_data(nl_hdr), msg);
	*/
	pr_info("Sendning unicast message");
	// send message 
	err = nlmsg_unicast(nl_sock, skb, pid);
	
	if(err < 0 ) {
		pr_info("Error while trying to send message over socket");
	}
	pr_info("message sent over socket");
		
}

/** 
*
*
*
*
*
*/
static int  nlmodule_init(void) {	
	

	struct netlink_kernel_cfg cfg = {
		.groups = 1,
		.input = nl_recv_msg,
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

static void __exit nlmodule_exit(void) {
	pr_info("exiting the nl module");
	netlink_kernel_release(nl_sock);
}

module_init(nlmodule_init);
module_exit(nlmodule_exit);

MODULE_LICENSE("GPL");
